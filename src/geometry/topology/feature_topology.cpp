#include "feature_topology.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_Curve.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <cmath>

namespace {

void fillEdgeSignature(const TopoDS_Edge& edge, SubShapeRef& ref)
{
    if (edge.IsNull()) {
        return;
    }

    BRepAdaptor_Curve curve(edge);
    const Standard_Real first = curve.FirstParameter();
    const Standard_Real last = curve.LastParameter();
    const Standard_Real mid = (first + last) * 0.5;
    gp_Pnt pMid;
    curve.D0(mid, pMid);

    ref.signatureLength = GCPnts_AbscissaPoint::Length(curve, first, last);
    ref.signatureMidX = pMid.X();
    ref.signatureMidY = pMid.Y();
    ref.signatureMidZ = pMid.Z();
}

int countEdges(const TopoDS_Shape& shape)
{
    int count = 0;
    if (shape.IsNull()) {
        return count;
    }
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        ++count;
    }
    return count;
}

void fillFaceSignature(const TopoDS_Shape& shape, SubShapeRef& ref)
{
    if (shape.IsNull()) {
        return;
    }

    ref.signatureEdgeCount = countEdges(shape);

    try {
        GProp_GProps surfaceProps;
        BRepGProp::SurfaceProperties(shape, surfaceProps);
        ref.signatureArea = surfaceProps.Mass();
        const gp_Pnt center = surfaceProps.CentreOfMass();
        ref.signatureMidX = center.X();
        ref.signatureMidY = center.Y();
        ref.signatureMidZ = center.Z();
    } catch (...) {
    }

    try {
        GProp_GProps linearProps;
        BRepGProp::LinearProperties(shape, linearProps);
        ref.signatureLength = linearProps.Mass();
    } catch (...) {
    }
}

TopoDS_Shape findEdgeBySignature(const TopoDS_Shape& parent, const SubShapeRef& ref)
{
    if (parent.IsNull()) {
        return TopoDS_Shape();
    }

    const double tolerance = Precision::Confusion() * 100.0;
    double bestScore = 1.0e100;
    TopoDS_Edge bestEdge;

    for (TopExp_Explorer explorer(parent, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        SubShapeRef candidate;
        candidate.parentIndex = ref.parentIndex;
        candidate.shapeType = TopAbs_EDGE;
        fillEdgeSignature(edge, candidate);

        const double dx = candidate.signatureMidX - ref.signatureMidX;
        const double dy = candidate.signatureMidY - ref.signatureMidY;
        const double dz = candidate.signatureMidZ - ref.signatureMidZ;
        const double midDist2 = dx * dx + dy * dy + dz * dz;
        const double lenDiff = std::abs(candidate.signatureLength - ref.signatureLength);
        const double score = midDist2 + lenDiff * lenDiff;

        if (score < bestScore) {
            bestScore = score;
            bestEdge = edge;
        }
    }

    if (bestEdge.IsNull() || bestScore > tolerance * tolerance) {
        return TopoDS_Shape();
    }
    return bestEdge;
}

} // namespace

SubShapeRef makeSubShapeRef(int parentIndex,
                            const TopoDS_Shape& parentShape,
                            const TopoDS_Shape& subShape,
                            TopAbs_ShapeEnum shapeType,
                            std::int64_t subShapeId)
{
    SubShapeRef ref;
    ref.parentIndex = parentIndex;
    ref.shapeType = shapeType;
    ref.subShapeId = subShapeId;

    if (parentShape.IsNull() || subShape.IsNull()) {
        return ref;
    }

    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(parentShape, shapeType, shapeMap);
    ref.persistentShapeIndex = shapeMap.FindIndex(subShape);

    if (shapeType == TopAbs_EDGE) {
        fillEdgeSignature(TopoDS::Edge(subShape), ref);
        ref.signatureEdgeCount = 1;
    } else if (shapeType == TopAbs_FACE || shapeType == TopAbs_WIRE) {
        fillFaceSignature(subShape, ref);
    }

    return ref;
}

SubShapeRef makeSketchContourRef(int parentIndex,
                                 int contourIndex,
                                 const TopoDS_Shape& contourProfile,
                                 std::int64_t subShapeId)
{
    SubShapeRef ref;
    ref.parentIndex = parentIndex;
    ref.shapeType = TopAbs_FACE;
    ref.semanticKind = static_cast<int>(SubShapeSemanticKind::SketchContour);
    ref.sketchContourIndex = contourIndex;
    ref.persistentShapeIndex = contourIndex >= 0 ? contourIndex + 1 : -1;
    ref.subShapeId = subShapeId;
    fillFaceSignature(contourProfile, ref);
    return ref;
}

TopoDS_Shape resolveSubShapeRef(const TopoDS_Shape& parentShape, const SubShapeRef& ref)
{
    if (parentShape.IsNull()) {
        return TopoDS_Shape();
    }

    if (ref.persistentShapeIndex > 0) {
        TopTools_IndexedMapOfShape shapeMap;
        TopExp::MapShapes(parentShape, ref.shapeType, shapeMap);
        if (ref.persistentShapeIndex <= shapeMap.Extent()) {
            const TopoDS_Shape resolved = shapeMap(ref.persistentShapeIndex);
            if (!resolved.IsNull()) {
                return resolved;
            }
        }
    }

    if (ref.shapeType == TopAbs_EDGE) {
        return findEdgeBySignature(parentShape, ref);
    }

    return TopoDS_Shape();
}
