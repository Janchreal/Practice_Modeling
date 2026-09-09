#include "sketch_geometry.h"

#include <QtGlobal>

#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Circle.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <gp_Ax2.hxx>
#include <TopoDS.hxx>

#include <limits>

namespace SketchGeometry {

Handle(Geom_Curve) sketchBasisCurve(Handle(Geom_Curve) curve)
{
    while (!curve.IsNull() && curve->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        curve = Handle(Geom_TrimmedCurve)::DownCast(curve)->BasisCurve();
    }
    return curve;
}

Handle(Geom_Circle) sketchCircleBasis(Handle(Geom_Curve) curve)
{
    return Handle(Geom_Circle)::DownCast(sketchBasisCurve(curve));
}

bool sketchEdgesEquivalent(const TopoDS_Edge& a, const TopoDS_Edge& b)
{
    if (a.IsNull() || b.IsNull()) return false;
    if (a.IsSame(b)) return true;

    Standard_Real f1 = 0.0, l1 = 0.0, f2 = 0.0, l2 = 0.0;
    Handle(Geom_Curve) c1 = BRep_Tool::Curve(a, f1, l1);
    Handle(Geom_Curve) c2 = BRep_Tool::Curve(b, f2, l2);
    if (c1.IsNull() || c2.IsNull()) return false;

    const gp_Pnt p1a = c1->Value(f1);
    const gp_Pnt p1b = c1->Value(l1);
    const gp_Pnt p2a = c2->Value(f2);
    const gp_Pnt p2b = c2->Value(l2);
    const double tol = 1.0e-6;
    return (p1a.Distance(p2a) <= tol && p1b.Distance(p2b) <= tol)
        || (p1a.Distance(p2b) <= tol && p1b.Distance(p2a) <= tol);
}

int findSketchEdgeIndex(const QList<TopoDS_Shape>& geometries, const TopoDS_Edge& targetEdge)
{
    for (int i = 0; i < geometries.size(); ++i) {
        const TopoDS_Shape& sh = geometries[i];
        if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
        if (sketchEdgesEquivalent(TopoDS::Edge(sh), targetEdge)) {
            return i;
        }
    }
    return -1;
}

double distancePointToEdge(const gp_Pnt& point, const TopoDS_Edge& edge)
{
    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return std::numeric_limits<double>::max();

    const double lo = qMin(f, l);
    const double hi = qMax(f, l);
    GeomAPI_ProjectPointOnCurve proj(point, curve);
    if (proj.NbPoints() < 1) return std::numeric_limits<double>::max();

    double t = proj.LowerDistanceParameter();
    t = qBound(lo, t, hi);
    return point.Distance(curve->Value(t));
}

bool edgeMidPoint(const TopoDS_Edge& edge, gp_Pnt& outPoint)
{
    if (edge.IsNull()) return false;
    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return false;
    outPoint = curve->Value(0.5 * (f + l));
    return true;
}

bool circularEdgeMidPoint(const TopoDS_Edge& edge, gp_Pnt& outPoint)
{
    if (edge.IsNull()) return false;

    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return false;

    Handle(Geom_Curve) basis = curve;
    if (basis->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        basis = Handle(Geom_TrimmedCurve)::DownCast(basis)->BasisCurve();
    }
    if (Handle(Geom_Circle)::DownCast(basis).IsNull()) return false;

    outPoint = curve->Value(0.5 * (f + l));
    return true;
}

QList<gp_Pnt> edgeSnapCandidates(const TopoDS_Edge& edge, bool includeMidpoint, bool includeCenter, bool includeQuadrants)
{
    QList<gp_Pnt> points;
    if (edge.IsNull()) return points;

    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return points;

    Handle(Geom_Curve) basis = curve;
    if (basis->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        basis = Handle(Geom_TrimmedCurve)::DownCast(basis)->BasisCurve();
    }
    Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(basis);
    if (circ.IsNull()) {
        if (includeMidpoint) {
            points.append(curve->Value(0.5 * (f + l)));
        }
        return points;
    }

    if (includeMidpoint) {
        points.append(curve->Value(0.5 * (f + l)));
    }
    if (includeCenter) {
        points.append(circ->Location());
    }
    if (includeQuadrants) {
        const gp_Pnt c = circ->Location();
        const gp_Ax2 ax = circ->Position();
        const gp_Dir xd = ax.XDirection();
        const gp_Dir yd = ax.YDirection();
        const double r = circ->Radius();
        points.append(gp_Pnt(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r));
        points.append(gp_Pnt(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r));
        points.append(gp_Pnt(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r));
        points.append(gp_Pnt(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r));
    }
    return points;
}

int preferredEdgeSampleCount(const TopoDS_Edge& edge, int lineCount, int curvedCount, int fallbackCount)
{
    int sampleCount = lineCount;
    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) {
        sampleCount = fallbackCount;
    } else {
        try {
            GeomAdaptor_Curve adaptor(curve, f, l);
            if (adaptor.GetType() != GeomAbs_Line) {
                sampleCount = curvedCount;
            }
        } catch (...) {
            sampleCount = fallbackCount;
        }
    }
    return qMax(2, sampleCount);
}

QList<gp_Pnt> sampleEdgePoints(const TopoDS_Edge& edge, int sampleCount)
{
    QList<gp_Pnt> points;
    if (edge.IsNull() || sampleCount < 2) return points;

    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return points;

    points.reserve(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        const double t = (sampleCount == 1)
            ? f
            : (f + (l - f) * static_cast<double>(i) / static_cast<double>(sampleCount - 1));
        points.append(curve->Value(t));
    }
    return points;
}

bool projectPointOntoEdge(const gp_Pnt& point, const TopoDS_Edge& edge, gp_Pnt& outPoint, double* outParam)
{
    if (edge.IsNull()) return false;

    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return false;

    GeomAPI_ProjectPointOnCurve proj(point, curve);
    if (proj.NbPoints() < 1) return false;

    double t = proj.LowerDistanceParameter();
    const double lo = qMin(f, l);
    const double hi = qMax(f, l);
    t = qBound(lo, t, hi);
    outPoint = curve->Value(t);
    if (outParam) {
        *outParam = t;
    }
    return true;
}

bool edgeIntersectionPoint(const TopoDS_Edge& a, const TopoDS_Edge& b, gp_Pnt& outPoint, double tolerance)
{
    if (a.IsNull() || b.IsNull()) return false;

    Standard_Real fa = 0.0, la = 0.0, fb = 0.0, lb = 0.0;
    Handle(Geom_Curve) ca = BRep_Tool::Curve(a, fa, la);
    Handle(Geom_Curve) cb = BRep_Tool::Curve(b, fb, lb);
    if (ca.IsNull() || cb.IsNull()) return false;

    try {
        GeomAPI_ExtremaCurveCurve extrema(ca, cb);
        if (extrema.NbExtrema() < 1) return false;
        gp_Pnt p1, p2;
        extrema.NearestPoints(p1, p2);
        if (p1.Distance(p2) > tolerance) return false;
        outPoint = gp_Pnt((p1.X() + p2.X()) * 0.5, (p1.Y() + p2.Y()) * 0.5, (p1.Z() + p2.Z()) * 0.5);
        return true;
    } catch (...) {
        return false;
    }
}

bool edgePointAndTangentAtPosition(const TopoDS_Edge& edge, double positionValue, bool positionIsPercent, gp_Pnt& outPoint, gp_Vec& outTangent, double* outTotalLength)
{
    if (edge.IsNull()) return false;

    Standard_Real firstParam = 0.0;
    Standard_Real lastParam = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, firstParam, lastParam);
    if (curve.IsNull()) return false;

    Standard_Real evalParam = 0.5 * (firstParam + lastParam);
    double totalLen = 0.0;
    try {
        GeomAdaptor_Curve adaptor(curve, firstParam, lastParam);
        const Standard_Real measuredLen = GCPnts_AbscissaPoint::Length(adaptor, firstParam, lastParam);
        totalLen = static_cast<double>(measuredLen);

        Standard_Real s = 0.0;
        if (positionIsPercent) {
            const Standard_Real pct = qBound<Standard_Real>(0.0, static_cast<Standard_Real>(positionValue), 100.0);
            s = measuredLen * (pct / 100.0);
        } else {
            s = qBound<Standard_Real>(0.0, static_cast<Standard_Real>(positionValue), measuredLen);
        }

        GCPnts_AbscissaPoint ab(adaptor, s, firstParam);
        if (ab.IsDone()) {
            evalParam = ab.Parameter();
        }
    } catch (...) {
    }

    gp_Pnt p;
    gp_Vec d1;
    curve->D1(evalParam, p, d1);
    if (d1.Magnitude() <= Precision::Confusion()) return false;

    outPoint = p;
    outTangent = d1;
    if (outTotalLength) {
        *outTotalLength = totalLen;
    }
    return true;
}

} // namespace SketchGeometry
