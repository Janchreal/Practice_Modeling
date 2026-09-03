#ifndef SELECTION_GEOMETRY_H
#define SELECTION_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace SelectionGeometry {

bool faceNormal(const TopoDS_Face& face, gp_Dir& outDir, gp_Pnt& outPnt);
bool inferPlaneNormalFromEdges(const QList<TopoDS_Edge>& edges, gp_Dir& outDir, gp_Pnt& outOrigin);
bool singleWireFromShape(const TopoDS_Shape& shape, TopoDS_Wire& outWire);
TopoDS_Wire wireContainingEdge(const TopoDS_Shape& shape, const TopoDS_Edge& edge);
QList<TopoDS_Edge> edgesOfShape(const TopoDS_Shape& shape);

} // namespace SelectionGeometry

#endif // SELECTION_GEOMETRY_H
