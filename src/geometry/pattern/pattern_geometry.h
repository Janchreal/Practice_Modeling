#ifndef PATTERN_GEOMETRY_H
#define PATTERN_GEOMETRY_H

#include "domain/features/patternfeaturetypes.h"
#include "platformmath.h"

#include <QList>

#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

namespace PatternGeometry {

gp_Dir perpendicularDirection(const gp_Dir& axis, const gp_Dir& hint);
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
                               double polygonSpanDegrees = 360.0,
                               PolygonSpacingMode polygonSpacing = PolygonSpacingMode::CountPerSide,
                               int polygonAlongEdgeCount = 2,
                               double polygonAlongEdgePitch = 10.0);
gp_Trsf makeCircularInstanceTransform(const gp_Pnt& origin,
                                      const gp_Dir& axisDir,
                                      double angularPitchDeg,
                                      int indexI,
                                      bool useRadial,
                                      const gp_Dir& radialDir,
                                      double radialPitch,
                                      int indexJ);
gp_Trsf makeRotatedRadialTransform(const gp_Pnt& origin,
                                   const gp_Dir& axisDir,
                                   double angleRad,
                                   bool useRadial,
                                   const gp_Dir& radialDir,
                                   double radialPitch,
                                   int radialIndex);
double polygonPatternRadius(const gp_Pnt& origin,
                            const gp_Dir& axisDir,
                            const TopoDS_Shape& shape);
int polygonAlongEdgeInstanceCount(double edgeArcLength,
                                  PolygonSpacingMode spacing,
                                  int countPerSide,
                                  double edgePitch);

} // namespace PatternGeometry

#endif // PATTERN_GEOMETRY_H
