#include "feature_modification_geometry.h"

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <ChFi3d_FilletShape.hxx>
#include <GeomAbs_Shape.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <algorithm>
#include <cmath>

namespace FeatureModificationGeometry {

namespace {

bool fail(std::string* errorMessage, const char* message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool collectEdgeFaces(const TopoDS_Shape& shape,
                      const TopoDS_Edge& edge,
                      TopoDS_Face& outF1,
                      TopoDS_Face& outF2)
{
    int found = 0;
    outF1 = TopoDS_Face();
    outF2 = TopoDS_Face();
    for (TopExp_Explorer exF(shape, TopAbs_FACE); exF.More(); exF.Next()) {
        const TopoDS_Face f = TopoDS::Face(exF.Current());
        for (TopExp_Explorer exE(f, TopAbs_EDGE); exE.More(); exE.Next()) {
            const TopoDS_Edge e = TopoDS::Edge(exE.Current());
            if (!e.IsSame(edge)) {
                continue;
            }
            if (found == 0) {
                outF1 = f;
            } else if (found == 1) {
                outF2 = f;
                return true;
            }
            ++found;
            break;
        }
    }
    return false;
}

void configureFilletProfile(BRepFilletAPI_MakeFillet& fillet,
                            bool isG2,
                            double rho,
                            double rhoN,
                            int profileIndex)
{
    if (!isG2) {
        fillet.SetFilletShape(ChFi3d_Rational);
        fillet.SetContinuity(GeomAbs_C1, 1.0e-3);
        return;
    }

    switch (profileIndex) {
    case 0:
        fillet.SetFilletShape(rho < 1.0 ? ChFi3d_Polynomial : ChFi3d_QuasiAngular);
        fillet.SetContinuity(GeomAbs_C2, 7.0e-4 + (1.0 - rhoN) * 7.0e-4);
        fillet.SetParams(1.0e-2, 1.0e-3, 1.0e-5,
                         2.0e-4 + (1.0 - rhoN) * 6.0e-4,
                         1.0e-5,
                         2.5e-4 + (1.0 - rhoN) * 7.0e-4);
        break;
    case 1:
        fillet.SetFilletShape(rho < 1.0 ? ChFi3d_QuasiAngular : ChFi3d_Polynomial);
        fillet.SetContinuity(GeomAbs_C2, 1.0e-3 + (1.0 - rhoN) * 1.1e-3);
        fillet.SetParams(1.0e-2, 1.0e-3, 1.0e-5,
                         4.0e-4 + (1.0 - rhoN) * 9.0e-4,
                         1.0e-5,
                         5.0e-4 + (1.0 - rhoN) * 1.1e-3);
        break;
    default:
        fillet.SetFilletShape(ChFi3d_Rational);
        fillet.SetContinuity(GeomAbs_C2, 1.8e-3 + (1.0 - rhoN) * 2.2e-3);
        fillet.SetParams(1.0e-2, 1.0e-3, 1.0e-5,
                         9.0e-4 + (1.0 - rhoN) * 2.2e-3,
                         1.0e-5,
                         1.0e-3 + (1.0 - rhoN) * 2.4e-3);
        break;
    }
}

} // namespace

bool buildFilletShape(const TopoDS_Shape& targetShape,
                      const QList<TopoDS_Edge>& edges,
                      double radius,
                      bool isG2,
                      double rho,
                      TopoDS_Shape& outShape,
                      double* usedRadius,
                      std::string* errorMessage)
{
    outShape = TopoDS_Shape();
    if (usedRadius) {
        *usedRadius = radius;
    }
    if (targetShape.IsNull() || edges.isEmpty() || radius <= 0.0) {
        return fail(errorMessage, "Fillet: invalid input");
    }

    try {
        const double rhoClamped = std::clamp(rho, 0.05, 1.95);
        const double rhoN = std::clamp((rhoClamped - 0.05) / 1.90, 0.0, 1.0);

        BRepFilletAPI_MakeFillet fillet(targetShape);
        auto tryBuild = [&](double currentRadius, int profileIndex) -> bool {
            fillet.Reset();
            configureFilletProfile(fillet, isG2, rhoClamped, rhoN, profileIndex);

            bool added = false;
            for (const TopoDS_Edge& edge : edges) {
                if (edge.IsNull()) {
                    continue;
                }
                fillet.Add(currentRadius, edge);
                added = true;
            }
            if (!added) {
                return false;
            }

            fillet.Build();
            return fillet.IsDone() && !fillet.Shape().IsNull();
        };

        bool ok = false;
        double currentRadius = radius;
        const int maxProfiles = isG2 ? 3 : 1;
        for (int profile = 0; profile < maxProfiles && !ok; ++profile) {
            for (int attempt = 0; attempt < 6; ++attempt) {
                currentRadius = radius * std::pow(0.5, attempt);
                if (currentRadius <= 1e-9) {
                    break;
                }
                if (tryBuild(currentRadius, profile)) {
                    ok = true;
                    break;
                }
            }
        }

        if (!ok) {
            return fail(errorMessage, isG2 ? "Fillet: G2 build failed" : "Fillet: build failed");
        }

        outShape = fillet.Shape();
        if (usedRadius) {
            *usedRadius = currentRadius;
        }
        return !outShape.IsNull();
    } catch (const Standard_Failure& e) {
        return fail(errorMessage, e.GetMessageString());
    } catch (const std::exception& e) {
        return fail(errorMessage, e.what());
    } catch (...) {
        return fail(errorMessage, "Fillet: unexpected failure");
    }
}

bool buildChamferShape(const TopoDS_Shape& targetShape,
                       const QList<TopoDS_Edge>& edges,
                       double distance1,
                       double distance2,
                       bool twoDistances,
                       TopoDS_Shape& outShape,
                       std::string* errorMessage)
{
    outShape = TopoDS_Shape();
    if (targetShape.IsNull() || edges.isEmpty() || distance1 <= 0.0 || distance2 <= 0.0) {
        return fail(errorMessage, "Chamfer: invalid input");
    }

    try {
        BRepFilletAPI_MakeChamfer chamfer(targetShape);
        bool added = false;
        for (const TopoDS_Edge& edge : edges) {
            if (edge.IsNull()) {
                continue;
            }
            if (twoDistances) {
                TopoDS_Face face1, face2;
                if (!collectEdgeFaces(targetShape, edge, face1, face2) || face1.IsNull()) {
                    return fail(errorMessage, "Chamfer: edge face lookup failed");
                }
                chamfer.Add(distance1, distance2, edge, face1);
            } else {
                chamfer.Add(distance1, edge);
            }
            added = true;
        }

        if (!added) {
            return fail(errorMessage, "Chamfer: no valid edges");
        }

        chamfer.Build();
        if (!chamfer.IsDone() || chamfer.Shape().IsNull()) {
            return fail(errorMessage, "Chamfer: build failed");
        }

        outShape = chamfer.Shape();
        return !outShape.IsNull();
    } catch (const Standard_Failure& e) {
        return fail(errorMessage, e.GetMessageString());
    } catch (const std::exception& e) {
        return fail(errorMessage, e.what());
    } catch (...) {
        return fail(errorMessage, "Chamfer: unexpected failure");
    }
}

bool buildHollowShape(const TopoDS_Shape& targetShape,
                      const TopTools_ListOfShape& facesToRemove,
                      double thickness,
                      TopoDS_Shape& outShape,
                      std::string* errorMessage)
{
    outShape = TopoDS_Shape();
    if (targetShape.IsNull() || facesToRemove.IsEmpty() || thickness <= 0.0) {
        return fail(errorMessage, "Hollow: invalid input");
    }

    try {
        BRepOffsetAPI_MakeThickSolid hollowMaker;
        hollowMaker.MakeThickSolidByJoin(targetShape, facesToRemove, thickness, 1e-3);
        if (!hollowMaker.IsDone() || hollowMaker.Shape().IsNull()) {
            return fail(errorMessage, "Hollow: build failed");
        }

        outShape = hollowMaker.Shape();
        return !outShape.IsNull();
    } catch (const Standard_Failure& e) {
        return fail(errorMessage, e.GetMessageString());
    } catch (const std::exception& e) {
        return fail(errorMessage, e.what());
    } catch (...) {
        return fail(errorMessage, "Hollow: unexpected failure");
    }
}

} // namespace FeatureModificationGeometry
