#include "pattern_geometry.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>

namespace PatternGeometry {

namespace {

constexpr double kDegToRad = M_PI / 180.0;

TopoDS_Shape buildLinearPatternShape(const QList<TopoDS_Shape>& sourceShapes,
                                     const gp_Dir& direction1,
                                     double pitch1,
                                     int count1,
                                     bool useDirection2,
                                     const gp_Dir& direction2,
                                     double pitch2,
                                     int count2)
{
    if (sourceShapes.isEmpty() || count1 < 1) {
        return TopoDS_Shape();
    }
    const int c2 = useDirection2 ? std::max(1, count2) : 1;

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (const TopoDS_Shape& src : sourceShapes) {
        if (src.IsNull()) {
            continue;
        }
        for (int i = 0; i < count1; ++i) {
            for (int j = 0; j < c2; ++j) {
                if (i == 0 && j == 0) {
                    builder.Add(compound, src);
                    continue;
                }
                gp_Trsf trsf;
                const gp_Vec offset = gp_Vec(direction1) * (i * pitch1) + gp_Vec(direction2) * (j * pitch2);
                trsf.SetTranslation(offset);
                BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                builder.Add(compound, transformer.Shape());
            }
        }
    }
    return compound;
}

TopoDS_Shape buildCircularPatternShape(const QList<TopoDS_Shape>& sourceShapes,
                                      const gp_Pnt& origin,
                                      const gp_Dir& axisDir,
                                      double angularPitchDeg,
                                      int count1,
                                      bool useDirection2,
                                      const gp_Dir& radialDir,
                                      double radialPitch,
                                      int count2)
{
    if (sourceShapes.isEmpty() || count1 < 1) {
        return TopoDS_Shape();
    }
    const int c2 = useDirection2 ? std::max(1, count2) : 1;

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (const TopoDS_Shape& src : sourceShapes) {
        if (src.IsNull()) {
            continue;
        }
        for (int i = 0; i < count1; ++i) {
            for (int j = 0; j < c2; ++j) {
                if (i == 0 && j == 0) {
                    builder.Add(compound, src);
                    continue;
                }
                const gp_Trsf trsf = PatternGeometry::makeCircularInstanceTransform(
                    origin, axisDir, angularPitchDeg, i, useDirection2, radialDir, radialPitch, j);
                BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                builder.Add(compound, transformer.Shape());
            }
        }
    }
    return compound;
}

TopoDS_Shape buildPolygonalPatternShape(const QList<TopoDS_Shape>& sourceShapes,
                                       const gp_Pnt& origin,
                                       const gp_Dir& axisDir,
                                       double spanDegrees,
                                       int sides,
                                       PolygonSpacingMode polygonSpacing,
                                       int alongEdgeCount,
                                       double alongEdgePitch,
                                       bool useRadialReplication,
                                       const gp_Dir& radialDir,
                                       double radialPitch,
                                       int count2)
{
    if (sourceShapes.isEmpty() || sides < 1) {
        return TopoDS_Shape();
    }
    const int numSides = std::max(1, sides);
    const int c2 = useRadialReplication ? std::max(1, count2) : 1;
    const double spanRad = spanDegrees * kDegToRad;
    const double sideAngleRad = spanRad / static_cast<double>(numSides);

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (const TopoDS_Shape& src : sourceShapes) {
        if (src.IsNull()) {
            continue;
        }

        const double radius = PatternGeometry::polygonPatternRadius(origin, axisDir, src);

        for (int s = 0; s < numSides; ++s) {
            const double ang0 = s * sideAngleRad;
            const double ang1 = (s + 1) * sideAngleRad;
            const double edgeArcLen = radius * (ang1 - ang0);
            const int nAlong = PatternGeometry::polygonAlongEdgeInstanceCount(
                edgeArcLen, polygonSpacing, alongEdgeCount, alongEdgePitch);

            for (int k = 0; k < nAlong; ++k) {
                const double t = (nAlong <= 1) ? 0.0 : static_cast<double>(k) / static_cast<double>(nAlong - 1);
                const double angleRad = ang0 + t * (ang1 - ang0);

                for (int j = 0; j < c2; ++j) {
                    if (s == 0 && k == 0 && j == 0) {
                        builder.Add(compound, src);
                        continue;
                    }
                    const gp_Trsf trsf = PatternGeometry::makeRotatedRadialTransform(
                        origin, axisDir, angleRad, useRadialReplication, radialDir, radialPitch, j);
                    BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                    builder.Add(compound, transformer.Shape());
                }
            }
        }
    }
    return compound;
}

} // namespace

gp_Dir perpendicularDirection(const gp_Dir& axis, const gp_Dir& hint)
{
    gp_Vec perp = gp_Vec(axis).Crossed(gp_Vec(hint));
    if (perp.Magnitude() < Precision::Angular()) {
        perp = gp_Vec(axis).Crossed(gp_Vec(1, 0, 0));
    }
    if (perp.Magnitude() < Precision::Angular()) {
        perp = gp_Vec(axis).Crossed(gp_Vec(0, 1, 0));
    }
    perp.Normalize();
    return gp_Dir(perp);
}

TopoDS_Shape buildPatternShape(const QList<TopoDS_Shape>& sourceShapes,
                               PatternLayoutType layoutType,
                               const gp_Pnt& origin,
                               const gp_Dir& direction1,
                               double pitch1,
                               int count1,
                               bool useRadialReplication,
                               const gp_Dir& direction2,
                               double pitch2,
                               int count2,
                               double polygonSpanDegrees,
                               PolygonSpacingMode polygonSpacing,
                               int polygonAlongEdgeCount,
                               double polygonAlongEdgePitch)
{
    switch (layoutType) {
    case PatternLayoutType::Circular:
        return buildCircularPatternShape(sourceShapes, origin, direction1, pitch1, count1,
                                         useRadialReplication, direction2, pitch2, count2);
    case PatternLayoutType::Polygonal:
        return buildPolygonalPatternShape(sourceShapes, origin, direction1, polygonSpanDegrees,
                                          count1, polygonSpacing, polygonAlongEdgeCount,
                                          polygonAlongEdgePitch, useRadialReplication, direction2,
                                          pitch2, count2);
    case PatternLayoutType::Linear:
    default:
        return buildLinearPatternShape(sourceShapes, direction1, pitch1, count1,
                                       useRadialReplication, direction2, pitch2, count2);
    }
}

gp_Trsf makeCircularInstanceTransform(const gp_Pnt& origin,
                                      const gp_Dir& axisDir,
                                      double angularPitchDeg,
                                      int indexI,
                                      bool useRadial,
                                      const gp_Dir& radialDir,
                                      double radialPitch,
                                      int indexJ)
{
    gp_Trsf trsf;
    if (indexI == 0 && indexJ == 0) {
        return trsf;
    }

    gp_Trsf rot;
    gp_Trsf radial;
    bool hasRot = false;
    bool hasRadial = false;

    if (indexI != 0) {
        rot.SetRotation(gp_Ax1(origin, axisDir), indexI * angularPitchDeg * kDegToRad);
        hasRot = true;
    }
    if (indexJ != 0 && useRadial) {
        radial.SetTranslation(gp_Vec(radialDir) * (indexJ * radialPitch));
        hasRadial = true;
    }

    if (hasRot && hasRadial) {
        trsf = radial;
        trsf.Multiply(rot);
    } else if (hasRot) {
        trsf = rot;
    } else if (hasRadial) {
        trsf = radial;
    }
    return trsf;
}

gp_Trsf makeRotatedRadialTransform(const gp_Pnt& origin,
                                   const gp_Dir& axisDir,
                                   double angleRad,
                                   bool useRadial,
                                   const gp_Dir& radialDir,
                                   double radialPitch,
                                   int radialIndex)
{
    gp_Trsf trsf;
    gp_Trsf rot;
    gp_Trsf radial;
    bool hasRot = false;
    bool hasRadial = false;

    if (std::abs(angleRad) > Precision::Angular()) {
        rot.SetRotation(gp_Ax1(origin, axisDir), angleRad);
        hasRot = true;
    }
    if (radialIndex != 0 && useRadial) {
        radial.SetTranslation(gp_Vec(radialDir) * (radialIndex * radialPitch));
        hasRadial = true;
    }

    if (hasRot && hasRadial) {
        trsf = radial;
        trsf.Multiply(rot);
    } else if (hasRot) {
        trsf = rot;
    } else if (hasRadial) {
        trsf = radial;
    }
    return trsf;
}

double polygonPatternRadius(const gp_Pnt& origin,
                            const gp_Dir& axisDir,
                            const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) {
        return 10.0;
    }
    Standard_Real xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const gp_Pnt center((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
    gp_Vec radial = gp_Vec(origin, center);
    radial -= gp_Vec(axisDir) * radial.Dot(gp_Vec(axisDir));
    const double r = radial.Magnitude();
    return (r > Precision::Confusion()) ? r : 10.0;
}

int polygonAlongEdgeInstanceCount(double edgeArcLength,
                                  PolygonSpacingMode spacing,
                                  int countPerSide,
                                  double edgePitch)
{
    if (spacing == PolygonSpacingMode::CountPerSide) {
        return std::max(1, countPerSide);
    }
    if (edgePitch <= Precision::Confusion()) {
        return 1;
    }
    return std::max(1, static_cast<int>(std::floor(edgeArcLength / edgePitch)) + 1);
}

} // namespace PatternGeometry
