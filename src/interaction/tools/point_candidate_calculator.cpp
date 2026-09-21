#include "point_candidate_calculator.h"

#include "geometry/sketch/sketch_geometry.h"
#include "point_snap_manager.h"

#include <algorithm>
#include <cmath>

#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Precision.hxx>
#include <Standard_Real.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>

namespace {

constexpr double kTwoPi = 6.28318530717958647692;

bool parameterInPeriodicRange(double parameter,
                              double first,
                              double last,
                              double period,
                              double tolerance)
{
    if (!std::isfinite(parameter) || !std::isfinite(first) || !std::isfinite(last)
        || period <= Precision::Confusion()) {
        return false;
    }

    const double lo = std::min(first, last);
    const double hi = std::max(first, last);
    const double span = hi - lo;
    if (span <= Precision::Confusion()) {
        return false;
    }
    if (span >= period - tolerance) {
        return true;
    }

    const double base = parameter + std::floor((lo - parameter) / period) * period;
    for (int i = -1; i <= 2; ++i) {
        const double shifted = base + static_cast<double>(i) * period;
        if (shifted >= lo - tolerance && shifted <= hi + tolerance) {
            return true;
        }
    }
    return false;
}

bool circularPointLiesOnEdge(const TopoDS_Edge& edge,
                             const Handle(Geom_Curve)& curve,
                             Standard_Real first,
                             Standard_Real last,
                             const Handle(Geom_Circle)& circle,
                             const gp_Pnt& point)
{
    if (edge.IsNull() || curve.IsNull() || circle.IsNull()) {
        return false;
    }

    try {
        GeomAPI_ProjectPointOnCurve projection(point, curve);
        if (projection.NbPoints() < 1) {
            return false;
        }

        const Standard_Real parameter = projection.LowerDistanceParameter();
        const gp_Pnt projected = curve->Value(parameter);
        const double spatialTolerance = std::max(1.0e-6, circle->Radius() * 1.0e-6);
        if (projected.Distance(point) > spatialTolerance) {
            return false;
        }

        return parameterInPeriodicRange(
            static_cast<double>(parameter),
            static_cast<double>(first),
            static_cast<double>(last),
            kTwoPi,
            std::max(Precision::Angular() * 100.0, 1.0e-7));
    } catch (...) {
        return false;
    }
}

bool edgeLengthMidpoint(const TopoDS_Edge& edge,
                        const Handle(Geom_Curve)& curve,
                        Standard_Real first,
                        Standard_Real last,
                        gp_Pnt& outPoint)
{
    gp_Vec tangent;
    if (SketchGeometry::edgePointAndTangentAtPosition(
            edge, 50.0, true, outPoint, tangent, nullptr)) {
        return true;
    }
    if (curve.IsNull()) {
        return false;
    }
    outPoint = curve->Value((first + last) * 0.5);
    return true;
}

bool isClosedCircleEdge(const Handle(Geom_Circle)& circle,
                        Standard_Real first,
                        Standard_Real last)
{
    if (circle.IsNull()) {
        return false;
    }
    return std::abs(std::abs(static_cast<double>(last - first)) - kTwoPi)
        <= std::max(Precision::Angular() * 100.0, 1.0e-7);
}

PointCandidate makeCandidate(PointSnapType type,
                             const gp_Pnt& point,
                             const TopoDS_Edge& edge,
                             double parameter,
                             bool hasParameter)
{
    PointCandidate candidate;
    candidate.label = PointSnapManager::labelForType(type);
    candidate.point = point;
    candidate.snapType = type;
    candidate.sourceShape = edge;
    candidate.sourceEdge = edge;
    candidate.parameter = parameter;
    candidate.hasParameter = hasParameter;
    return candidate;
}

} // namespace

PointCandidateList PointCandidateCalculator::edgeCandidates(const TopoDS_Edge& edge,
                                                            const PointSnapMode& mode)
{
    PointCandidateList candidates;
    if (edge.IsNull()) {
        return candidates;
    }

    Standard_Real first = 0.0;
    Standard_Real last = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull() || !std::isfinite(static_cast<double>(first))
        || !std::isfinite(static_cast<double>(last))) {
        return candidates;
    }

    const Handle(Geom_Circle) circle = SketchGeometry::sketchCircleBasis(curve);
    const bool closedCircle = isClosedCircleEdge(circle, first, last);

    if (mode.endpoint && !closedCircle) {
        candidates.append(makeCandidate(
            PointSnapType::Endpoint, curve->Value(first), edge, first, true));
        candidates.append(makeCandidate(
            PointSnapType::Endpoint, curve->Value(last), edge, last, true));
    }

    if (mode.midpoint) {
        gp_Pnt midpoint;
        if (edgeLengthMidpoint(edge, curve, first, last, midpoint)) {
            candidates.append(makeCandidate(
                PointSnapType::Midpoint, midpoint, edge,
                (static_cast<double>(first) + static_cast<double>(last)) * 0.5,
                true));
        }
    }

    if (mode.arcMidpoint && !circle.IsNull()) {
        const Standard_Real mid = (first + last) * 0.5;
        candidates.append(makeCandidate(
            PointSnapType::ArcMidpoint, curve->Value(mid), edge, mid, true));
    }

    if (circle.IsNull()) {
        return candidates;
    }

    const gp_Pnt center = circle->Location();
    if (mode.center) {
        candidates.append(makeCandidate(
            PointSnapType::Center, center, edge, 0.0, false));
    }

    if (!mode.quadrant) {
        return candidates;
    }

    const gp_Ax2 axis = circle->Position();
    const gp_Dir xDirection = axis.XDirection();
    const gp_Dir yDirection = axis.YDirection();
    const double radius = circle->Radius();
    const gp_Pnt quadrantPoints[] = {
        gp_Pnt(center.X() + xDirection.X() * radius,
               center.Y() + xDirection.Y() * radius,
               center.Z() + xDirection.Z() * radius),
        gp_Pnt(center.X() - xDirection.X() * radius,
               center.Y() - xDirection.Y() * radius,
               center.Z() - xDirection.Z() * radius),
        gp_Pnt(center.X() + yDirection.X() * radius,
               center.Y() + yDirection.Y() * radius,
               center.Z() + yDirection.Z() * radius),
        gp_Pnt(center.X() - yDirection.X() * radius,
               center.Y() - yDirection.Y() * radius,
               center.Z() - yDirection.Z() * radius),
    };

    for (const gp_Pnt& point : quadrantPoints) {
        if (circularPointLiesOnEdge(edge, curve, first, last, circle, point)) {
            candidates.append(makeCandidate(
                PointSnapType::Quadrant, point, edge, 0.0, false));
        }
    }

    return candidates;
}
