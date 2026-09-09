#include "extrusion_geometry.h"
#include "boolean_ops.h"

#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepGProp.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <ShapeFix_Wire.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>

#include <stdexcept>
#include <vector>

#include <cmath>

namespace ExtrusionGeometry {

TopoDS_Shape extrudeResolvedProfile(const TopoDS_Shape& profile,
                                    const gp_Dir& direction,
                                    double length,
                                    double startOffset,
                                    bool makeSheetBody)
{
    try {
        if (profile.IsNull() || std::abs(length) < Precision::Confusion()) {
            return TopoDS_Shape();
        }

        gp_Vec extrudeVec(direction);
        extrudeVec.Scale(length);

        TopoDS_Shape shapeToExtrude = profile;
        if (profile.ShapeType() == TopAbs_FACE && makeSheetBody) {
            const TopoDS_Wire outer = BRepTools::OuterWire(TopoDS::Face(profile));
            if (!outer.IsNull()) {
                shapeToExtrude = outer;
            }
        } else if (profile.ShapeType() == TopAbs_WIRE && !makeSheetBody) {
            ShapeFix_Wire fixer;
            fixer.Load(TopoDS::Wire(profile));
            fixer.FixReorder();
            fixer.FixConnected();
            fixer.FixClosed();
            BRepBuilderAPI_MakeFace faceMaker(fixer.Wire());
            if (faceMaker.IsDone()) {
                shapeToExtrude = faceMaker.Face();
            }
        }

        if (std::abs(startOffset) > Precision::Confusion()) {
            gp_Trsf startShift;
            startShift.SetTranslation(gp_Vec(direction) * startOffset);
            BRepBuilderAPI_Transform mover(shapeToExtrude, startShift, true);
            shapeToExtrude = mover.Shape();
        }

        BRepPrimAPI_MakePrism prism(shapeToExtrude, extrudeVec);
        if (!prism.IsDone()) {
            return TopoDS_Shape();
        }
        return prism.Shape();
    } catch (Standard_Failure&) {
        return TopoDS_Shape();
    } catch (...) {
        return TopoDS_Shape();
    }
}

bool buildExtrusionCompound(const QList<TopoDS_Shape>& profiles,
                            const gp_Dir& direction,
                            double startDistance,
                            double endDistance,
                            bool makeSheetBody,
                            TopoDS_Compound& outCompound)
{
    outCompound = TopoDS_Compound();
    const double length = endDistance - startDistance;
    if (profiles.isEmpty() || std::abs(length) < Precision::Confusion()) {
        return false;
    }

    BRep_Builder builder;
    builder.MakeCompound(outCompound);

    bool any = false;
    for (const TopoDS_Shape& profile : profiles) {
        const TopoDS_Shape extruded = extrudeResolvedProfile(
            profile, direction, length, startDistance, makeSheetBody);
        if (extruded.IsNull()) {
            continue;
        }
        builder.Add(outCompound, extruded);
        any = true;
    }
    return any;
}

namespace {

TopoDS_Shape makeFaceOrWireFromWire(const TopoDS_Wire& sourceWire)
{
    if (sourceWire.IsNull()) {
        return TopoDS_Shape();
    }

    ShapeFix_Wire fixer;
    fixer.Load(sourceWire);
    fixer.FixReorder();
    fixer.FixConnected();
    fixer.FixClosed();
    const TopoDS_Wire fixedWire = fixer.Wire();

    BRepBuilderAPI_MakeFace faceMaker(fixedWire);
    if (faceMaker.IsDone()) {
        return faceMaker.Face();
    }
    return fixedWire;
}

TopoDS_Shape buildWireFromEdges(const QList<TopoDS_Edge>& edges)
{
    if (edges.isEmpty()) {
        return TopoDS_Shape();
    }

    BRepBuilderAPI_MakeWire wireMaker;
    bool hasEdge = false;
    for (const TopoDS_Edge& edge : edges) {
        if (edge.IsNull()) {
            continue;
        }
        wireMaker.Add(edge);
        hasEdge = true;
    }
    if (!hasEdge || !wireMaker.IsDone()) {
        return TopoDS_Shape();
    }
    return wireMaker.Wire();
}

} // namespace

TopoDS_Shape prepareSolidExtrusionProfile(const TopoDS_Shape& sourceShape)
{
    if (sourceShape.IsNull()) {
        return TopoDS_Shape();
    }

    if (sourceShape.ShapeType() == TopAbs_FACE) {
        return sourceShape;
    }

    if (sourceShape.ShapeType() == TopAbs_WIRE) {
        return makeFaceOrWireFromWire(TopoDS::Wire(sourceShape));
    }

    QList<TopoDS_Edge> edges;
    for (TopExp_Explorer ex(sourceShape, TopAbs_EDGE); ex.More(); ex.Next()) {
        edges.append(TopoDS::Edge(ex.Current()));
    }
    const TopoDS_Shape wire = buildWireFromEdges(edges);
    if (wire.IsNull()) {
        return sourceShape;
    }
    if (wire.ShapeType() == TopAbs_WIRE) {
        return makeFaceOrWireFromWire(TopoDS::Wire(wire));
    }
    return wire;
}

TopoDS_Shape prepareSolidExtrusionProfile(const QList<TopoDS_Edge>& edges)
{
    const TopoDS_Shape wire = buildWireFromEdges(edges);
    if (wire.IsNull()) {
        return TopoDS_Shape();
    }
    if (wire.ShapeType() == TopAbs_WIRE) {
        return makeFaceOrWireFromWire(TopoDS::Wire(wire));
    }
    return wire;
}

namespace {

bool failDraft(std::string* errorMessage, const char* message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

void createTaperedPrismOffset(const TopoDS_Wire& sourceWire,
                              const gp_Vec& translation,
                              double offset,
                              bool isSecond,
                              TopoDS_Wire& result)
{
    (void)isSecond;

    gp_Trsf tempTransform;
    tempTransform.SetTranslation(translation);
    TopLoc_Location loc(tempTransform);
    TopoDS_Wire movedSourceWire = TopoDS::Wire(sourceWire.Moved(loc));

    TopoDS_Shape offsetShape;
    if (fabs(offset) > Precision::Confusion()) {
        BRepOffsetAPI_MakeOffset offsetMaker;
        offsetMaker.AddWire(movedSourceWire);
        try {
            offsetMaker.Perform(offset);
            if (offsetMaker.IsDone()) {
                offsetShape = offsetMaker.Shape();
            } else {
                throw std::runtime_error("Extrusion: Offset could not be created");
            }
        }
        catch (const Standard_Failure& e) {
            throw std::runtime_error(e.GetMessageString());
        }
    } else {
        offsetShape = movedSourceWire;
    }

    if (offsetShape.IsNull()) {
        throw std::runtime_error("Extrusion: end face of tapered extrusion is empty");
    }

    TopAbs_ShapeEnum type = offsetShape.ShapeType();
    if (type == TopAbs_WIRE) {
        result = TopoDS::Wire(offsetShape);
    } else if (type == TopAbs_EDGE) {
        BRepBuilderAPI_MakeWire mkWire2(TopoDS::Edge(offsetShape));
        result = mkWire2.Wire();
    } else {
        result = TopoDS_Wire();
        throw std::runtime_error("Extrusion: type of extrusion end face is not supported");
    }
}

void checkInnerWires(std::vector<bool>& isInnerWire,
                     const gp_Dir& direction,
                     std::vector<bool>& checklist,
                     bool forInner,
                     const std::vector<TopoDS_Shape>& prisms)
{
    size_t numCheckWiresInitial = 0;
    for (auto checks : checklist) {
        if (checks) {
            ++numCheckWiresInitial;
        }
    }
    GProp_GProps tempProperties;
    Standard_Real momentOfInertiaInitial;
    Standard_Real momentOfInertiaFinal;
    size_t numCheckWires = 0;
    std::vector<bool>::iterator isInnerWireIterator = isInnerWire.begin();
    std::vector<bool>::iterator toCheckIterator = checklist.begin();
    std::vector<bool> toDisable(checklist.size(), false);
    int outer = -1;

    for (auto itOuter = prisms.begin(); itOuter != prisms.end(); ++itOuter) {
        ++outer;
        if (!*toCheckIterator) {
            ++isInnerWireIterator;
            ++toCheckIterator;
            continue;
        }
        auto toCheckIteratorInner = checklist.begin();
        bool saveIsInnerWireIterator = *isInnerWireIterator;
        for (auto itInner = prisms.begin(); itInner != prisms.end(); ++itInner) {
            if (itOuter == itInner || !*toCheckIteratorInner) {
                ++toCheckIteratorInner;
                continue;
            }
            BRepGProp::VolumeProperties(*itInner, tempProperties);
            momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            TopoDS_Shape cutShape;
            if (!BooleanOps::executeOccBoolean(*itInner, *itOuter, 2, cutShape)) {
                throw std::runtime_error("Extrusion: Cut out failed");
            }
            BRepGProp::VolumeProperties(cutShape, tempProperties);
            momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            if ((momentOfInertiaInitial != momentOfInertiaFinal)
                && (momentOfInertiaFinal > Precision::Confusion())) {
                *isInnerWireIterator = !forInner;
                ++numCheckWires;
                *toCheckIterator = true;
                break;
            }
            ++toCheckIteratorInner;
        }
        if (saveIsInnerWireIterator == *isInnerWireIterator) {
            toDisable[outer] = true;
        }
        ++isInnerWireIterator;
        ++toCheckIterator;
    }

    size_t i = 0;
    for (auto disable : toDisable) {
        if (disable) {
            checklist[i] = false;
        }
        ++i;
    }

    if (numCheckWires == isInnerWire.size()) {
        isInnerWire[0] = false;
        checklist[0] = false;
        --numCheckWires;
    }

    if (numCheckWiresInitial == numCheckWires) {
        i = 0;
        for (auto checks : checklist) {
            if (checks) {
                isInnerWire[i] = false;
                checklist[i] = false;
                --numCheckWires;
            }
            ++i;
        }
    }

    if (numCheckWires > 1) {
        checkInnerWires(isInnerWire, direction, checklist, !forInner, prisms);
    }
}

} // namespace

bool buildTaperedExtrusionDrafts(const TopoDS_Shape& sourceShape,
                                 const gp_Dir& direction,
                                 double lengthFwd,
                                 double lengthRev,
                                 bool solid,
                                 double taperAngleFwd,
                                 double taperAngleRev,
                                 std::list<TopoDS_Shape>& drafts,
                                 std::string* errorMessage)
{
    drafts.clear();
    try {
        BRepBuilderAPI_Copy copyMaker(sourceShape);
        if (!copyMaker.IsDone()) {
            return failDraft(errorMessage, "Cannot copy shape");
        }
        TopoDS_Shape myShape = copyMaker.Shape();
        if (myShape.IsNull()) {
            return failDraft(errorMessage, "Cannot extrude empty shape");
        }

        double distanceFwd = tan(taperAngleFwd) * lengthFwd;
        double distanceRev = tan(taperAngleRev) * lengthRev;
        gp_Vec vecFwd = gp_Vec(direction) * lengthFwd;
        gp_Vec vecRev = gp_Vec(direction.Reversed()) * lengthRev;

        bool bFwd = fabs(lengthFwd) > Precision::Confusion();
        bool bRev = fabs(lengthRev) > Precision::Confusion();
        bool bMid = !bFwd || !bRev || -1.0 * taperAngleFwd != taperAngleRev;

        std::vector<std::vector<TopoDS_Shape>> wiresections;
        auto addWiresToWireSections =
            [&myShape](std::vector<std::vector<TopoDS_Shape>>& wiresections) -> size_t {
            TopExp_Explorer ex;
            size_t i = 0;
            for (ex.Init(myShape, TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
                wiresections.emplace_back();
                wiresections[i].push_back(TopoDS::Wire(ex.Current()));
            }
            return i;
        };

        if (myShape.IsNull()) {
            return failDraft(errorMessage, "Not a valid shape");
        }

        size_t numWires = addWiresToWireSections(wiresections);
        if (numWires == 0) {
            return failDraft(errorMessage, "Extrusion: Input must not only consist if a vertex");
        }

        TopoDS_Wire offsetWire;
        std::vector<std::vector<TopoDS_Shape>> extrusionSections(wiresections.size(), std::vector<TopoDS_Shape>());
        size_t rows = 0;
        int numEdges = 0;

        std::vector<TopoDS_Shape> resultPrisms;
        TopoDS_Shape singlePrism;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(singleWire));
                auto tempFace = mkFace.Shape();
                BRepPrimAPI_MakePrism mkPrism(tempFace, vecFwd);
                if (!mkPrism.IsDone()) {
                    return failDraft(errorMessage, "Extrusion: Generating prism failed");
                }
                singlePrism = mkPrism.Shape();
                resultPrisms.push_back(singlePrism);
            }
        }

        std::vector<bool> isInnerWire(resultPrisms.size(), false);
        std::vector<bool> checklist(resultPrisms.size(), true);
        checkInnerWires(isInnerWire, direction, checklist, false, resultPrisms);

        int numInnerWires = 0;
        for (auto isInner : isInnerWire) {
            if (isInner) {
                ++numInnerWires;
            }
        }

        if (bRev) {
            rows = 0;
            for (auto& wireVector : wiresections) {
                for (auto& singleWire : wireVector) {
                    numEdges = 0;
                    TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                    while (xp.More()) {
                        ++numEdges;
                        xp.Next();
                    }
                    if (!isInnerWire[rows]) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                    } else if (numEdges > 1) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, -distanceRev, true, offsetWire);
                    } else {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                    }
                    if (offsetWire.IsNull()) {
                        return failDraft(errorMessage, "Extrusion: end face of tapered extrusion is empty");
                    }
                    extrusionSections[rows].push_back(offsetWire);
                }
                ++rows;
            }
        }

        if (bMid) {
            rows = 0;
            for (auto& wireVector : wiresections) {
                for (auto& singleWire : wireVector) {
                    extrusionSections[rows].push_back(singleWire);
                }
                ++rows;
            }
        }

        if (bFwd) {
            rows = 0;
            for (auto& wireVector : wiresections) {
                for (auto& singleWire : wireVector) {
                    numEdges = 0;
                    TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                    while (xp.More()) {
                        ++numEdges;
                        xp.Next();
                    }
                    if (!isInnerWire[rows]) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
                    } else if (numEdges > 1) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, -distanceFwd, false, offsetWire);
                    } else {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
                    }
                    if (offsetWire.IsNull()) {
                        return failDraft(errorMessage, "Extrusion: end face of tapered extrusion is empty");
                    }
                    extrusionSections[rows].push_back(offsetWire);
                }
                ++rows;
            }
        }

        std::vector<TopoDS_Shape> shells;
        for (auto& wires : extrusionSections) {
            BRepOffsetAPI_ThruSections mkTS(solid, Standard_True, Precision::Confusion());
            for (auto& singleWire : wires) {
                if (singleWire.ShapeType() == TopAbs_VERTEX) {
                    mkTS.AddVertex(TopoDS::Vertex(singleWire));
                } else {
                    mkTS.AddWire(TopoDS::Wire(singleWire));
                }
            }
            mkTS.Build();
            if (!mkTS.IsDone()) {
                return failDraft(errorMessage, "Extrusion: Loft could not be built");
            }
            shells.push_back(mkTS.Shape());
        }

        if (solid) {
            if (numInnerWires > 0) {
                GProp_GProps tempProperties;
                Standard_Real momentOfInertiaInitial;
                Standard_Real momentOfInertiaFinal;
                auto isInnerWireIterator = isInnerWire.begin();
                std::vector<bool>::iterator isInnerWireIteratorLoop;
                for (auto itOuter = shells.begin(); itOuter != shells.end(); ++itOuter) {
                    if (*isInnerWireIterator) {
                        ++isInnerWireIterator;
                        continue;
                    }
                    isInnerWireIteratorLoop = isInnerWire.begin();
                    for (auto itInner = shells.begin(); itInner != shells.end(); ++itInner) {
                        if (itOuter == itInner || !*isInnerWireIteratorLoop) {
                            ++isInnerWireIteratorLoop;
                            continue;
                        }
                        BRepGProp::VolumeProperties(*itOuter, tempProperties);
                        momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
                        TopoDS_Shape cutShape;
                        if (!BooleanOps::executeOccBoolean(*itOuter, *itInner, 2, cutShape)) {
                            return failDraft(errorMessage, "Extrusion: Final cut out failed");
                        }
                        BRepGProp::VolumeProperties(cutShape, tempProperties);
                        momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
                        if ((momentOfInertiaInitial != momentOfInertiaFinal)
                            && (momentOfInertiaFinal > Precision::Confusion())) {
                            *itOuter = cutShape;
                        }
                        ++isInnerWireIteratorLoop;
                    }
                    drafts.push_back(*itOuter);
                    ++isInnerWireIterator;
                }
            } else {
                for (const auto& shell : shells) {
                    drafts.push_back(shell);
                }
            }
        } else {
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            for (TopoDS_Shape& s : shells) {
                sewer.Add(s);
            }
            sewer.Perform();
            drafts.push_back(sewer.SewedShape());
        }
    } catch (const Standard_Failure& e) {
        return failDraft(errorMessage, e.GetMessageString());
    } catch (const std::exception& e) {
        return failDraft(errorMessage, e.what());
    } catch (...) {
        return failDraft(errorMessage, "Extrusion: A fatal error occurred when making the loft");
    }

    return true;
}

} // namespace ExtrusionGeometry
