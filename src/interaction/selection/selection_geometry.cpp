#include "selection_geometry.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Surface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>

namespace SelectionGeometry {

bool faceNormal(const TopoDS_Face& face, gp_Dir& outDir, gp_Pnt& outPnt)
{
    TopLoc_Location loc;
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face, loc);
    if (surf.IsNull()) return false;
    BRepAdaptor_Surface adaptor(face);
    const Standard_Real uMid = 0.5 * (adaptor.FirstUParameter() + adaptor.LastUParameter());
    const Standard_Real vMid = 0.5 * (adaptor.FirstVParameter() + adaptor.LastVParameter());
    gp_Pnt p;
    gp_Vec d1u, d1v;
    surf->D1(uMid, vMid, p, d1u, d1v);
    if (!loc.IsIdentity()) p.Transform(loc.Transformation());
    gp_Vec n = d1u.Crossed(d1v);
    if (n.Magnitude() <= Precision::Confusion()) return false;
    if (!loc.IsIdentity()) n.Transform(loc.Transformation());
    outDir = gp_Dir(n);
    outPnt = p;
    return true;
}

bool inferPlaneNormalFromEdges(const QList<TopoDS_Edge>& edges, gp_Dir& outDir, gp_Pnt& outOrigin)
{
    if (edges.isEmpty()) return false;

    QList<gp_Vec> tangents;
    QList<gp_Pnt> samples;
    tangents.reserve(edges.size());
    samples.reserve(edges.size() * 3);

    for (const TopoDS_Edge& e : edges) {
        if (e.IsNull()) continue;

        if (edges.size() == 1) {
            Standard_Real f = 0.0, l = 0.0;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(e, f, l);
            if (!curve.IsNull()) {
                if (Handle(Geom_TrimmedCurve) trimmed = Handle(Geom_TrimmedCurve)::DownCast(curve)) {
                    curve = trimmed->BasisCurve();
                }
                if (Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(curve)) {
                    outDir = circ->Axis().Direction();
                    outOrigin = circ->Location();
                    return true;
                }
                if (Handle(Geom_Ellipse) ell = Handle(Geom_Ellipse)::DownCast(curve)) {
                    outDir = ell->Axis().Direction();
                    outOrigin = ell->Location();
                    return true;
                }
            }
        }

        BRepAdaptor_Curve ac(e);
        const Standard_Real u1 = ac.FirstParameter();
        const Standard_Real u2 = ac.LastParameter();
        const Standard_Real um = 0.5 * (u1 + u2);
        const gp_Pnt p0 = ac.Value(u1);
        const gp_Pnt p1 = ac.Value(u2);
        const gp_Pnt pm = ac.Value(um);
        samples.append(p0);
        samples.append(pm);
        samples.append(p1);

        gp_Pnt p;
        gp_Vec d1;
        ac.D1(um, p, d1);
        if (d1.Magnitude() > Precision::Confusion()) {
            tangents.append(d1);
        } else {
            gp_Vec chord(p0, p1);
            if (chord.Magnitude() > Precision::Confusion())
                tangents.append(chord);
        }
    }

    for (int i = 0; i < tangents.size(); ++i) {
        for (int j = i + 1; j < tangents.size(); ++j) {
            gp_Vec n = tangents[i].Crossed(tangents[j]);
            if (n.Magnitude() > Precision::Confusion()) {
                outDir = gp_Dir(n);
                if (!samples.isEmpty()) {
                    double sx = 0, sy = 0, sz = 0;
                    for (const gp_Pnt& pt : samples) {
                        sx += pt.X();
                        sy += pt.Y();
                        sz += pt.Z();
                    }
                    const double inv = 1.0 / samples.size();
                    outOrigin = gp_Pnt(sx * inv, sy * inv, sz * inv);
                }
                return true;
            }
        }
    }

    if (samples.size() >= 3) {
        const gp_Pnt& a = samples[0];
        for (int i = 1; i < samples.size(); ++i) {
            gp_Vec v1(a, samples[i]);
            if (v1.Magnitude() <= Precision::Confusion()) continue;
            for (int j = i + 1; j < samples.size(); ++j) {
                gp_Vec v2(a, samples[j]);
                gp_Vec n = v1.Crossed(v2);
                if (n.Magnitude() > Precision::Confusion()) {
                    outDir = gp_Dir(n);
                    outOrigin = a;
                    return true;
                }
            }
        }
    }
    return false;
}

bool singleWireFromShape(const TopoDS_Shape& shape, TopoDS_Wire& outWire)
{
    if (shape.IsNull()) return false;
    if (shape.ShapeType() == TopAbs_WIRE) {
        outWire = TopoDS::Wire(shape);
        return !outWire.IsNull();
    }

    TopoDS_Wire candidate;
    int wireCount = 0;
    for (TopExp_Explorer ex(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        const TopoDS_Wire w = TopoDS::Wire(ex.Current());
        if (w.IsNull()) continue;
        candidate = w;
        ++wireCount;
        if (wireCount > 1) {
            return false;
        }
    }
    if (wireCount == 1 && !candidate.IsNull()) {
        outWire = candidate;
        return true;
    }
    return false;
}

TopoDS_Wire wireContainingEdge(const TopoDS_Shape& shape, const TopoDS_Edge& edge)
{
    if (shape.IsNull() || edge.IsNull()) return TopoDS_Wire();

    for (TopExp_Explorer wireExp(shape, TopAbs_WIRE); wireExp.More(); wireExp.Next()) {
        const TopoDS_Wire wire = TopoDS::Wire(wireExp.Current());
        if (wire.IsNull()) continue;
        for (TopExp_Explorer edgeExp(wire, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
            if (edgeExp.Current().IsSame(edge)) {
                return wire;
            }
        }
    }
    return TopoDS_Wire();
}

QList<TopoDS_Edge> edgesOfShape(const TopoDS_Shape& shape)
{
    QList<TopoDS_Edge> edges;
    if (shape.IsNull()) return edges;

    for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Edge edge = TopoDS::Edge(ex.Current());
        if (!edge.IsNull()) {
            edges.append(edge);
        }
    }
    return edges;
}

} // namespace SelectionGeometry
