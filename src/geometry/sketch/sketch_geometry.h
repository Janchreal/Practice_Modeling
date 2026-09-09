#ifndef SKETCH_GEOMETRY_H
#define SKETCH_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <string>

namespace SketchGeometry {

Handle(Geom_Curve) sketchBasisCurve(Handle(Geom_Curve) curve);
Handle(Geom_Circle) sketchCircleBasis(Handle(Geom_Curve) curve);
bool sketchEdgesEquivalent(const TopoDS_Edge& a, const TopoDS_Edge& b);
int findSketchEdgeIndex(const QList<TopoDS_Shape>& geometries, const TopoDS_Edge& targetEdge);
double distancePointToEdge(const gp_Pnt& point, const TopoDS_Edge& edge);
bool edgeMidPoint(const TopoDS_Edge& edge, gp_Pnt& outPoint);
bool circularEdgeMidPoint(const TopoDS_Edge& edge, gp_Pnt& outPoint);
QList<gp_Pnt> edgeSnapCandidates(const TopoDS_Edge& edge, bool includeMidpoint, bool includeCenter, bool includeQuadrants);
/**
 * Build a planar profile from sketch edges without changing the source shapes.
 *
 * The returned shape is a FACE for one contour, or a compound of planar faces
 * for multiple independent contours.  Open, non-planar, invalid, or
 * non-face-buildable contours are rejected with a short diagnostic.
 */
bool buildPlanarProfile(const TopoDS_Shape& sketchShape,
                        const gp_Pln& sketchPlane,
                        TopoDS_Shape& outProfile,
                        std::string* errorMessage = nullptr);
int preferredEdgeSampleCount(const TopoDS_Edge& edge, int lineCount = 2, int curvedCount = 48, int fallbackCount = 32);
QList<gp_Pnt> sampleEdgePoints(const TopoDS_Edge& edge, int sampleCount);
bool projectPointOntoEdge(const gp_Pnt& point, const TopoDS_Edge& edge, gp_Pnt& outPoint, double* outParam = nullptr);
bool edgeIntersectionPoint(const TopoDS_Edge& a, const TopoDS_Edge& b, gp_Pnt& outPoint, double tolerance = 1.0e-3);
bool edgePointAndTangentAtPosition(const TopoDS_Edge& edge, double positionValue, bool positionIsPercent, gp_Pnt& outPoint, gp_Vec& outTangent, double* outTotalLength = nullptr);

} // namespace SketchGeometry

#endif // SKETCH_GEOMETRY_H
