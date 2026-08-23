// 拉伸/旋转：自动矢量、实时预览、起止手柄与布尔预览
#include "widget.h"
#include "extrusiondialog.h"
#include "handle_geometry.h"
#include "revolvedialog.h"

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QSet>
#include <QStatusBar>
#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Surface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkFeatureEdges.h>
#include <vtkLineSource.h>
#include <vtkMapper.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkVersionMacros.h>
#include <vtkPropPicker.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTubeFilter.h>

namespace {

constexpr double kPreviewR = 1.0;
constexpr double kPreviewG = 0.92;
constexpr double kPreviewB = 0.45;
constexpr double kPreviewOpacity = 0.45;
constexpr int kHandleSelectToDragThresholdPx2 = 16; // 4px^2：按下后小幅抖动不进入拖拽
// 拉伸/旋转交互手柄尺寸（箭头长度仅乘 overlayWorldScale，屏幕像素大小恒定）
constexpr double kExtrudeHandleSphereR = 0.07;
constexpr double kExtrudeHandleArrowLen = 0.55;
constexpr double kRevolveHandleSphereR = 0.09;
constexpr double kRevolveHandleCenterR = 0.07;
constexpr double kRevolveHandleArrowLen = 0.45;

constexpr auto kExtrusionHandleId = "extrusion_distance";
constexpr auto kRevolveHandleId = "revolve_angle";
constexpr auto kCtrlStartSphere = "start_sphere";
constexpr auto kCtrlEndArrow = "end_arrow";
constexpr double kArcBlueR = 0.45;
constexpr double kArcBlueG = 0.75;
constexpr double kArcBlueB = 1.0;
// 特征操作中的“透视”显示（半透明实体 + 橙色轮廓）
constexpr double kGhostOpacity = 0.38;
constexpr double kGhostEdgeR = 1.0;
constexpr double kGhostEdgeG = 0.55;
constexpr double kGhostEdgeB = 0.12;
constexpr double kGhostEdgeWidth = 2.8;

void configureTranslucentSolidMapper(vtkMapper* mapper, double factor, double units)
{
    if (!mapper) return;
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(factor, units);
}

void configureTranslucentSolidActor(vtkActor* actor, double opacity)
{
    if (!actor) return;
    vtkProperty* prop = actor->GetProperty();
    prop->SetOpacity(opacity);
    // 必须关闭背面剔除：片体/薄壳/空心体在部分视角下否则会“整面消失”
    prop->SetBackfaceCulling(false);
    prop->SetFrontfaceCulling(false);
    prop->SetEdgeVisibility(0);
    prop->SetRepresentationToSurface();
#if VTK_MAJOR_VERSION >= 9
    actor->ForceTranslucentOn();
#endif
    configureTranslucentSolidMapper(actor->GetMapper(), 1.0, 1.0);
}

/** 将 atan2 的 [-180,180] 读数解包为相对 prev 连续的角度（可跨过 ±180，支持到 ±360） */
double unwrapAngleNear(double prevDeg, double rawDeg)
{
    double prevNorm = prevDeg;
    while (prevNorm > 180.0) prevNorm -= 360.0;
    while (prevNorm < -180.0) prevNorm += 360.0;
    double delta = rawDeg - prevNorm;
    while (delta > 180.0) delta -= 360.0;
    while (delta < -180.0) delta += 360.0;
    return prevDeg + delta;
}

void clampSweepDeg(double startA, double& endA)
{
    double sweep = endA - startA;
    if (sweep > 360.0) endA = startA + 360.0;
    else if (sweep < -360.0) endA = startA - 360.0;
}

/** 拖起始角时钳制 start，保持 end 不动（两手柄独立） */
void clampStartAgainstEnd(double& startA, double endA)
{
    double sweep = endA - startA;
    if (sweep > 360.0) startA = endA - 360.0;
    else if (sweep < -360.0) startA = endA + 360.0;
}

HandleStateStyle extrusionCtrlStyle(const char* controlId, ControlState state)
{
    return HandleGeom::styleForControl(QString::fromLatin1(kExtrusionHandleId),
                                       QString::fromLatin1(controlId),
                                       state);
}

HandleStateStyle revolveCtrlStyle(const char* controlId, ControlState state)
{
    return HandleGeom::styleForControl(QString::fromLatin1(kRevolveHandleId),
                                       QString::fromLatin1(controlId),
                                       state);
}

double applyDistanceCr(const QString& handleId, const QString& controlId, double raw)
{
    if (const ControlSpec* c = HandleGeom::findControlSpec(handleId, controlId)) {
        if (c->cr) return c->cr(raw);
    }
    return raw;
}

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

/** 从开放边推断所在平面法向：两条非平行边叉积；单圆/椭圆取轴；否则用三点拟合 */
bool inferPlaneNormalFromEdges(const QList<TopoDS_Edge>& edges, gp_Dir& outDir, gp_Pnt& outOrigin)
{
    if (edges.isEmpty()) return false;

    QList<gp_Vec> tangents;
    QList<gp_Pnt> samples;
    tangents.reserve(edges.size());
    samples.reserve(edges.size() * 3);

    for (const TopoDS_Edge& e : edges) {
        if (e.IsNull()) continue;

        // 整圆/椭圆（含 TrimmedCurve 基底）：自身即确定平面
        if (edges.size() == 1) {
            Standard_Real f = 0.0, l = 0.0;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(e, f, l);
            if (!curve.IsNull()) {
                if (Handle(Geom_TrimmedCurve) trimmed = Handle(Geom_TrimmedCurve)::DownCast(curve))
                    curve = trimmed->BasisCurve();
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

    // 两条及以上非平行切向：叉积即为平面法向
    for (int i = 0; i < tangents.size(); ++i) {
        for (int j = i + 1; j < tangents.size(); ++j) {
            gp_Vec n = tangents[i].Crossed(tangents[j]);
            if (n.Magnitude() > Precision::Confusion()) {
                outDir = gp_Dir(n);
                if (!samples.isEmpty()) {
                    double sx = 0, sy = 0, sz = 0;
                    for (const gp_Pnt& pt : samples) {
                        sx += pt.X(); sy += pt.Y(); sz += pt.Z();
                    }
                    const double inv = 1.0 / samples.size();
                    outOrigin = gp_Pnt(sx * inv, sy * inv, sz * inv);
                }
                return true;
            }
        }
    }

    // 切向都平行时：用不共线三点拟合平面
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

bool rayPlaneHit(vtkRenderer* renderer, int x, int y, const gp_Pnt& planeOrigin, const gp_Dir& planeNormal,
                 gp_Pnt& outHit)
{
    if (!renderer) return false;
    renderer->SetDisplayPoint(x, y, 0.0);
    renderer->DisplayToWorld();
    double nearPt[4];
    renderer->GetWorldPoint(nearPt);
    renderer->SetDisplayPoint(x, y, 1.0);
    renderer->DisplayToWorld();
    double farPt[4];
    renderer->GetWorldPoint(farPt);
    if (std::abs(nearPt[3]) < 1e-12 || std::abs(farPt[3]) < 1e-12) return false;
    gp_Pnt p0(nearPt[0] / nearPt[3], nearPt[1] / nearPt[3], nearPt[2] / nearPt[3]);
    gp_Pnt p1(farPt[0] / farPt[3], farPt[1] / farPt[3], farPt[2] / farPt[3]);
    gp_Vec ray(p0, p1);
    if (ray.Magnitude() < 1e-12) return false;
    gp_Dir rayDir(ray);
    const double denom = planeNormal.Dot(rayDir);
    if (std::abs(denom) < 1e-12) return false;
    const double t = gp_Vec(p0, planeOrigin).Dot(planeNormal) / denom;
    outHit = p0.Translated(gp_Vec(rayDir) * t);
    return true;
}

} // namespace

bool Widget::inferDirectionFromExtrusionSelection(gp_Dir& outDir, gp_Pnt* outOrigin) const
{
    outDir = gp_Dir(0, 0, 1);
    if (outOrigin) *outOrigin = gp_Pnt(0, 0, 0);
    if (extrusionSelectedFaces.isEmpty()) return false;

    gp_Pnt centerFallback;
    const bool hasCenter = computeExtrusionProfileCenter(centerFallback);
    if (hasCenter && outOrigin) *outOrigin = centerFallback;

    try {
        for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
            if (sel.shapeType == TopAbs_FACE) {
                TopoDS_Face face = sel.getFace();
                if (face.IsNull()) continue;
                gp_Pnt p;
                if (faceNormal(face, outDir, p)) {
                    if (outOrigin) *outOrigin = p;
                    return true;
                }
            } else if (sel.shapeType == TopAbs_WIRE) {
                TopoDS_Wire wire = sel.getWire();
                if (wire.IsNull() || !BRep_Tool::IsClosed(wire)) continue;
                BRepBuilderAPI_MakeFace faceMaker(wire, Standard_True);
                if (!faceMaker.IsDone()) continue;
                gp_Pnt p;
                if (faceNormal(faceMaker.Face(), outDir, p)) {
                    if (outOrigin) *outOrigin = p;
                    return true;
                }
            }
        }

        // 收集已选边（含开放边）；优先拼封闭线框，否则由边切向叉积推断平面法向
        QList<TopoDS_Edge> edges;
        for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
            if (sel.shapeType == TopAbs_EDGE && !sel.getEdge().IsNull()) {
                edges.append(sel.getEdge());
            } else if (sel.shapeType == TopAbs_WIRE && !sel.getWire().IsNull()) {
                for (TopExp_Explorer ex(sel.getWire(), TopAbs_EDGE); ex.More(); ex.Next()) {
                    edges.append(TopoDS::Edge(ex.Current()));
                }
            }
        }
        if (edges.size() >= 1) {
            BRepBuilderAPI_MakeWire mw;
            bool addOk = true;
            for (const TopoDS_Edge& e : edges) {
                try {
                    mw.Add(e);
                } catch (Standard_Failure&) {
                    addOk = false;
                    break;
                }
            }
            if (addOk && mw.IsDone()) {
                TopoDS_Wire wire = mw.Wire();
                if (!wire.IsNull() && BRep_Tool::IsClosed(wire)) {
                    BRepBuilderAPI_MakeFace faceMaker(wire, Standard_True);
                    if (faceMaker.IsDone()) {
                        gp_Pnt p;
                        if (faceNormal(faceMaker.Face(), outDir, p)) {
                            if (outOrigin) *outOrigin = p;
                            return true;
                        }
                    }
                }
            }

            // 未闭合（如两条相交棱边）：用边所在平面的法向作为自动矢量
            gp_Pnt origin;
            if (inferPlaneNormalFromEdges(edges, outDir, origin)) {
                if (outOrigin) *outOrigin = hasCenter ? centerFallback : origin;
                return true;
            }
        }
    } catch (Standard_Failure&) {
        outDir = gp_Dir(0, 0, 1);
    } catch (...) {
        outDir = gp_Dir(0, 0, 1);
    }

    if (hasCenter && outOrigin) *outOrigin = centerFallback;
    return true;
}

void Widget::applyAutoVectorFromSelection()
{
    const bool autoMode =
        (extrusionDialog && extrusionDialog->isAutoVectorMode())
        || (revolveDialog && revolveDialog->isAutoVectorMode());
    if (!autoMode) return;

    try {
        gp_Dir dir;
        gp_Pnt origin;
        if (!inferDirectionFromExtrusionSelection(dir, &origin)) {
            return;
        }
        customVectorDir_ = dir;
        hasCustomVectorDir_ = true;
        vectorDialogBaseDir_ = dir;
        hasVectorDialogBaseDir_ = true;

        // 起止操作柄已表达轴向：不叠矢量预览箭头（避免与起始球/中心球重叠）
        updateExtrusionHandles();
        updateRevolveHandles();
        if (extrusionDialog) refreshExtrusionLivePreview();
        if (revolveDialog) refreshRevolveLivePreview();
    } catch (Standard_Failure&) {
    } catch (...) {
    }
}

bool Widget::computeExtrusionProfileCenter(gp_Pnt& outCenter) const
{
    Bnd_Box box;
    bool any = false;
    for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
        if (sel.shape.IsNull()) continue;
        BRepBndLib::Add(sel.shape, box);
        any = true;
    }
    if (!any || box.IsVoid()) return false;
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    outCenter = gp_Pnt(0.5 * (xmin + xmax), 0.5 * (ymin + ymax), 0.5 * (zmin + zmax));
    return true;
}

namespace {

TopoDS_Shape coalesceBooleanTool(const TopoDS_Shape& tool)
{
    if (tool.IsNull()) return tool;
    if (tool.ShapeType() != TopAbs_COMPOUND && tool.ShapeType() != TopAbs_COMPSOLID) {
        return tool;
    }
    TopoDS_Shape fused;
    bool has = false;
    for (TopExp_Explorer ex(tool, TopAbs_SOLID); ex.More(); ex.Next()) {
        const TopoDS_Shape s = ex.Current();
        if (!has) {
            fused = s;
            has = true;
            continue;
        }
        BRepAlgoAPI_Fuse fuse(fused, s);
        fuse.Build();
        if (!fuse.IsDone() || fuse.Shape().IsNull()) {
            return tool;
        }
        fused = fuse.Shape();
    }
    if (has) return fused;
    // 无实体时再尝试面（片体布尔）
    for (TopExp_Explorer ex(tool, TopAbs_FACE); ex.More(); ex.Next()) {
        const TopoDS_Shape s = ex.Current();
        if (!has) {
            fused = s;
            has = true;
            continue;
        }
        BRepAlgoAPI_Fuse fuse(fused, s);
        fuse.Build();
        if (!fuse.IsDone() || fuse.Shape().IsNull()) {
            return tool;
        }
        fused = fuse.Shape();
    }
    return has ? fused : tool;
}

bool shapeHasVolumeOrFace(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) return false;
    for (TopExp_Explorer ex(shape, TopAbs_SOLID); ex.More(); ex.Next()) {
        return true;
    }
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        return true;
    }
    return false;
}

} // namespace

bool Widget::applyDialogBooleanToShape(int boolMode, int boolTargetIndex,
                                       const TopoDS_Shape& featureShape,
                                       TopoDS_Shape& outShape) const
{
    if (featureShape.IsNull()) return false;
    if (boolMode < 0) {
        outShape = featureShape;
        return true;
    }
    // 已选布尔类型但尚未选目标体：不算失败，保留特征本身（预览用）
    if (boolTargetIndex < 0 || boolTargetIndex >= historyList.size()) {
        outShape = featureShape;
        return true;
    }
    TopoDS_Shape targetRaw = const_cast<Widget*>(this)->getShapeFromHistory(boolTargetIndex);
    if (targetRaw.IsNull()) {
        return false;
    }

    // 复制后再布尔，避免“从目标体自身取面拉伸再切回自身”时共享拓扑导致失败/结果异常
    TopoDS_Shape target = BRepBuilderAPI_Copy(targetRaw).Shape();
    TopoDS_Shape tool = BRepBuilderAPI_Copy(coalesceBooleanTool(featureShape)).Shape();
    if (target.IsNull() || tool.IsNull()) return false;

    try {
        constexpr double kFuzzy = 1.0e-4;
        TopoDS_Shape result;
        switch (boolMode) {
        case 0: { // 合并 Fuse(target, tool)
            BRepAlgoAPI_Fuse op(target, tool);
            op.SetFuzzyValue(kFuzzy);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) return false;
            result = op.Shape();
            break;
        }
        case 1: { // 求交 Common
            BRepAlgoAPI_Common op(target, tool);
            op.SetFuzzyValue(kFuzzy);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) return false;
            result = op.Shape();
            break;
        }
        case 2: { // 减去 Cut(target, tool)
            BRepAlgoAPI_Cut op(target, tool);
            op.SetFuzzyValue(kFuzzy);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) return false;
            result = op.Shape();
            // 减去后体积应明显小于目标体；若几乎不变，说明工具体未切入实体
            {
                GProp_GProps propsTarget, propsResult;
                BRepGProp::VolumeProperties(target, propsTarget);
                BRepGProp::VolumeProperties(result, propsResult);
                const double vT = propsTarget.Mass();
                const double vR = propsResult.Mass();
                if (vT > Precision::Confusion()
                    && (vT - vR) < std::max(1.0e-6 * vT, Precision::Confusion() * 10.0)) {
                    return false;
                }
            }
            break;
        }
        default:
            return false;
        }

        if (!shapeHasVolumeOrFace(result)) {
            return false;
        }

        ShapeFix_Shape fixer(result);
        fixer.Perform();
        if (!fixer.Shape().IsNull()) {
            result = fixer.Shape();
        }
        ShapeUpgrade_UnifySameDomain unify(result, true, true, true);
        unify.Build();
        if (!unify.Shape().IsNull()) {
            result = unify.Shape();
        }
        outShape = result;
        return !outShape.IsNull();
    } catch (Standard_Failure&) {
        return false;
    } catch (...) {
        return false;
    }
}

bool Widget::buildExtrusionPreviewShape(ExtrusionDialog* dialog, TopoDS_Shape& outShape,
                                        bool softBooleanFallback) const
{
    if (!dialog || extrusionSelectedFaces.isEmpty()) return false;

    const double startD = dialog->getStartDistance();
    const double endD = dialog->getEndDistance();
    const double len = endD - startD;
    if (std::abs(len) < 1e-9) return false;

    gp_Dir dir = getExtrusionDirection(dialog);
    gp_Trsf startTr;
    startTr.SetTranslation(gp_Vec(dir) * startD);
    gp_Vec prismVec(dir);
    prismVec.Scale(len);

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    bool any = false;

    auto addPrism = [&](const TopoDS_Shape& profile) {
        if (profile.IsNull()) return;
        try {
            BRepBuilderAPI_Transform mover(profile, startTr, true);
            BRepPrimAPI_MakePrism prism(mover.Shape(), prismVec);
            if (!prism.IsDone()) return;
            builder.Add(compound, prism.Shape());
            any = true;
        } catch (Standard_Failure&) {
        } catch (...) {
        }
    };

    QList<TopoDS_Edge> edges;
    for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
        if (sel.shapeType == TopAbs_FACE) {
            addPrism(sel.getFace());
        } else if (sel.shapeType == TopAbs_WIRE) {
            addPrism(sel.getWire());
        } else if (sel.shapeType == TopAbs_EDGE) {
            edges.append(sel.getEdge());
        }
    }
    if (!edges.isEmpty()) {
        try {
            BRepBuilderAPI_MakeWire mw;
            bool addOk = true;
            for (const TopoDS_Edge& e : edges) {
                if (e.IsNull()) continue;
                try {
                    mw.Add(e);
                } catch (Standard_Failure&) {
                    addOk = false;
                    break;
                }
            }
            if (addOk && mw.IsDone()) {
                TopoDS_Wire wire = mw.Wire();
                if (!wire.IsNull() && BRep_Tool::IsClosed(wire)) {
                    ShapeFix_Wire fixer;
                    fixer.Load(wire);
                    fixer.FixReorder();
                    fixer.FixConnected();
                    fixer.Perform();
                    wire = fixer.Wire();
                    BRepBuilderAPI_MakeFace fm(wire, Standard_True);
                    if (fm.IsDone()) {
                        addPrism(fm.Face());
                    } else {
                        addPrism(wire);
                    }
                } else {
                    // 开放线框：逐边拉伸，避免 ShapeFix_FixClosed / MakeFace 崩
                    for (const TopoDS_Edge& e : edges) {
                        if (!e.IsNull()) addPrism(e);
                    }
                }
            } else {
                for (const TopoDS_Edge& e : edges) {
                    if (!e.IsNull()) addPrism(e);
                }
            }
        } catch (Standard_Failure&) {
            for (const TopoDS_Edge& e : edges) {
                if (!e.IsNull()) addPrism(e);
            }
        } catch (...) {
            for (const TopoDS_Edge& e : edges) {
                if (!e.IsNull()) addPrism(e);
            }
        }
    }
    if (!any) return false;

    TopoDS_Shape feature = compound;
    if (applyDialogBooleanToShape(dialog->booleanMode(), dialog->booleanTargetIndex(),
                                  feature, outShape) && !outShape.IsNull()) {
        return true;
    }
    // 预览软回退：布尔失败时仍显示拉伸体，避免手柄拖动时预览消失
    if (softBooleanFallback) {
        outShape = feature;
        return !outShape.IsNull();
    }
    return false;
}

bool Widget::buildRevolvePreviewShape(revolvedialog* dialog, TopoDS_Shape& outShape,
                                      bool softBooleanFallback) const
{
    if (!dialog || extrusionSelectedFaces.isEmpty()) return false;
    gp_Ax1 axis = getRevolutionAxis(dialog);
    const double startA = dialog->getStartAngle();
    const double endA = dialog->getEndAngle();
    double sweep = endA - startA;
    while (sweep > 360.0) sweep -= 360.0;
    while (sweep < -360.0) sweep += 360.0;
    if (std::abs(sweep) < 1e-6) return false;

    // 将剖面绕轴先旋转 startA，再扫掠 sweep
    gp_Trsf startRot;
    startRot.SetRotation(axis, startA * M_PI / 180.0);
    const double sweepRad = sweep * M_PI / 180.0;

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    bool any = false;

    auto addRevol = [&](const TopoDS_Shape& profile) {
        if (profile.IsNull()) return;
        try {
            BRepBuilderAPI_Transform mover(profile, startRot, true);
            BRepPrimAPI_MakeRevol revol(mover.Shape(), axis, sweepRad, true);
            if (!revol.IsDone()) return;
            builder.Add(compound, revol.Shape());
            any = true;
        } catch (Standard_Failure&) {
        } catch (...) {
        }
    };

    for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
        if (sel.shapeType == TopAbs_FACE) addRevol(sel.getFace());
        else if (sel.shapeType == TopAbs_WIRE) addRevol(sel.getWire());
        else if (sel.shapeType == TopAbs_EDGE) addRevol(sel.getEdge());
    }
    if (!any) return false;

    TopoDS_Shape feature = compound;
    if (applyDialogBooleanToShape(dialog->booleanMode(), dialog->booleanTargetIndex(),
                                  feature, outShape) && !outShape.IsNull()) {
        return true;
    }
    if (softBooleanFallback) {
        outShape = feature;
        return !outShape.IsNull();
    }
    return false;
}

void Widget::applyFeatureGhostStyleToModel(int index)
{
    if (!renderer || index < 0 || index >= historyList.size()) return;
    ModelingHistory& rec = historyList[index];
    if (!rec.actor || rec.actor->GetVisibility() == 0) return;
    // 草图线框保持不透明，避免“看不见”
    if (rec.type == SKETCH) return;

    rec.actor->GetProperty()->SetColor(rec.color.redF(), rec.color.greenF(), rec.color.blueF());
    configureTranslucentSolidActor(rec.actor, kGhostOpacity);
    // 幽灵体略向后偏移，让预览/轮廓优先，减轻共面闪烁
    configureTranslucentSolidMapper(rec.actor->GetMapper(), 2.0, 2.0);

    // 轮廓走 IVtk 框架管线；幽灵色由 styleModelBoundaryOutline 统一设置
    if (!rec.outlineActor) {
        ensureModelBoundaryOutline(index);
    } else {
        styleModelBoundaryOutline(index, index == currentSelectedIndex);
    }
}

void Widget::setFeatureOperationGhostMode(bool on, int onlyModelIndex)
{
    if (!renderer) {
        featureOperationGhostMode_ = on;
        return;
    }

    auto restoreModelOpaque = [&](int i) {
        ModelingHistory& rec = historyList[i];
        if (rec.actor && rec.actor->GetVisibility() != 0) {
            rec.actor->GetProperty()->SetColor(
                rec.color.redF(), rec.color.greenF(), rec.color.blueF());
            rec.actor->GetProperty()->SetOpacity(1.0);
            rec.actor->GetProperty()->SetEdgeVisibility(0);
            rec.actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
            rec.actor->GetProperty()->SetLineWidth(1.4);
            rec.actor->GetProperty()->SetBackfaceCulling(false);
#if VTK_MAJOR_VERSION >= 9
            rec.actor->ForceTranslucentOff();
#endif
        }
        // 退出幽灵模式后恢复黑色轮廓（勿永久删掉）
        if (rec.outlineActor) {
            styleModelBoundaryOutline(i, i == currentSelectedIndex);
        } else {
            ensureModelBoundaryOutline(i);
        }
    };

    if (!on) {
        featureOperationGhostMode_ = false;
        // 退出幽灵模式后关闭深度剥离，恢复轮廓线正常遮挡
        renderer->SetUseDepthPeeling(0);
        for (int i = 0; i < historyList.size(); ++i) {
            restoreModelOpaque(i);
        }
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        return;
    }

    featureOperationGhostMode_ = true;
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->SetMultiSamples(0);
        vtkWidget->renderWindow()->SetAlphaBitPlanes(1);
    }
    // 半透明幽灵体才需要深度剥离；会削弱线轮廓遮挡，因此仅在此模式开启
    renderer->SetUseDepthPeeling(1);
    renderer->SetMaximumNumberOfPeels(100);
    renderer->SetOcclusionRatio(0.0);

    // 先全部恢复，再按范围套幽灵（支持“仅目标体”）
    for (int i = 0; i < historyList.size(); ++i) {
        restoreModelOpaque(i);
    }
    if (onlyModelIndex >= 0) {
        if (onlyModelIndex < historyList.size()) {
            applyFeatureGhostStyleToModel(onlyModelIndex);
        }
    } else {
        for (int i = 0; i < historyList.size(); ++i) {
            applyFeatureGhostStyleToModel(i);
        }
    }
    refreshCameraClippingRange();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::showFeatureLivePreviewShape(const TopoDS_Shape& shape)
{
    if (!renderer || !vtkWidget || shape.IsNull()) {
        clearFeatureLivePreview();
        return;
    }
    try {
        BRepMesh_IncrementalMesh mesh(shape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(900000010);
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        if (!meshPolyData || meshPolyData->GetNumberOfPoints() == 0) {
            clearFeatureLivePreview();
            return;
        }

        vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
        pd->DeepCopy(meshPolyData);
        pd->SetLines(nullptr);
        pd->SetVerts(nullptr);

        clearFeatureLivePreview();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(pd);
        previewActor = vtkSmartPointer<vtkActor>::New();
        previewActor->SetMapper(mapper);
        previewActor->GetProperty()->SetColor(kPreviewR, kPreviewG, kPreviewB);
        configureTranslucentSolidActor(previewActor, kPreviewOpacity);
        configureTranslucentSolidMapper(mapper, -2.0, -2.0);
        previewActor->SetPickable(false);
        addAppearanceActor(previewActor);
        refreshCameraClippingRange();
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    } catch (...) {
        clearFeatureLivePreview();
    }
}

void Widget::clearFeatureLivePreview()
{
    if (previewActor && renderer) {
        removeSceneActor(previewActor);
        previewActor = nullptr;
    }
}

void Widget::showFeatureResultPreviewShape(const TopoDS_Shape& shape, const QColor& color)
{
    if (!renderer || !vtkWidget || shape.IsNull()) {
        clearFeatureLivePreview();
        return;
    }
    try {
        BRepMesh_IncrementalMesh mesh(shape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(900000011);
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        if (!meshPolyData || meshPolyData->GetNumberOfPoints() == 0) {
            clearFeatureLivePreview();
            return;
        }

        vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
        pd->DeepCopy(meshPolyData);
        pd->SetLines(nullptr);
        pd->SetVerts(nullptr);

        clearFeatureLivePreview();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(pd);
        previewActor = vtkSmartPointer<vtkActor>::New();
        previewActor->SetMapper(mapper);
        vtkProperty* prop = previewActor->GetProperty();
        prop->SetColor(color.redF(), color.greenF(), color.blueF());
        prop->SetOpacity(1.0);
        applySolidActorMaterial(prop);
        prop->SetBackfaceCulling(false);
        prop->SetFrontfaceCulling(false);
#if VTK_MAJOR_VERSION >= 9
        previewActor->ForceTranslucentOff();
#endif
        previewActor->SetPickable(false);
        addAppearanceActor(previewActor);
        refreshCameraClippingRange();
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    } catch (...) {
        clearFeatureLivePreview();
    }
}

void Widget::hideModelsForResultPreview()
{
    featureResultPreviewHiddenModelIndices_.clear();
    QSet<int> toHide;
    for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
        if (sel.modelIndex >= 0) {
            toHide.insert(sel.modelIndex);
        }
    }
    if (extrusionDialog) {
        const int boolTarget = extrusionDialog->booleanTargetIndex();
        if (extrusionDialog->booleanMode() >= 0 && boolTarget >= 0) {
            toHide.insert(boolTarget);
        }
    }
    if (revolveDialog) {
        const int boolTarget = revolveDialog->booleanTargetIndex();
        if (revolveDialog->booleanMode() >= 0 && boolTarget >= 0) {
            toHide.insert(boolTarget);
        }
    }
    for (int idx : toHide) {
        if (idx < 0 || idx >= historyList.size()) continue;
        ModelingHistory& rec = historyList[idx];
        if (rec.actor) {
            rec.actor->SetVisibility(0);
            featureResultPreviewHiddenModelIndices_.append(idx);
        }
        if (rec.outlineActor) {
            rec.outlineActor->SetVisibility(0);
        }
    }
}

void Widget::restoreModelsAfterResultPreview()
{
    for (int idx : featureResultPreviewHiddenModelIndices_) {
        if (idx < 0 || idx >= historyList.size()) continue;
        ModelingHistory& rec = historyList[idx];
        if (rec.actor) {
            rec.actor->SetVisibility(1);
        }
        if (rec.outlineActor) {
            rec.outlineActor->SetVisibility(1);
        }
    }
    featureResultPreviewHiddenModelIndices_.clear();
}

void Widget::enterFeatureResultPreview(bool isExtrusion)
{
    TopoDS_Shape shape;
    const bool ok = isExtrusion
        ? (extrusionDialog && buildExtrusionPreviewShape(extrusionDialog, shape, false))
        : (revolveDialog && buildRevolvePreviewShape(revolveDialog, shape, false));
    if (!ok || shape.IsNull()) {
        if (statusBar()) {
            statusBar()->showMessage(
                isExtrusion ? tr("未选择几何体或参数无效，无法预览结果")
                            : tr("未选择几何体、角度无效或布尔失败，无法预览结果"),
                2500);
        }
        return;
    }

    setFeatureOperationGhostMode(false);
    hideModelsForResultPreview();
    clearExtrusionSelectionHighlight();
    clearExtrusionFaceHighlight();
    clearExtrusionHandles();
    clearRevolveHandles();
    clearVectorDialogArrowPreview();
    showFeatureResultPreviewShape(shape, QColor(255, 140, 0));
    if (!previewActor) {
        restoreModelsAfterResultPreview();
        if (extrusionDialog || revolveDialog) {
            setFeatureOperationGhostMode(true);
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("预览结果生成失败"), 2500);
        }
        if (isExtrusion) {
            updateExtrusionHandles();
            refreshExtrusionLivePreview();
        } else {
            updateRevolveHandles();
            refreshRevolveLivePreview();
        }
        return;
    }

    featureResultPreviewActive_ = true;
    if (isExtrusion && extrusionDialog) {
        extrusionDialog->setResultPreviewActive(true);
    } else if (!isExtrusion && revolveDialog) {
        revolveDialog->setResultPreviewActive(true);
    }
    if (statusBar()) {
        statusBar()->showMessage(tr("结果预览中：点击「取消预览结果」可继续编辑"), 4000);
    }
}

void Widget::leaveFeatureResultPreview(bool isExtrusion)
{
    featureResultPreviewActive_ = false;
    restoreModelsAfterResultPreview();
    if (isExtrusion) {
        if (extrusionDialog && extrusionDialog->isResultPreviewActive()) {
            extrusionDialog->setResultPreviewActive(false);
        }
        if (extrusionDialog) {
            setFeatureOperationGhostMode(true);
        }
        updateExtrusionHandles();
        refreshExtrusionLivePreview();
    } else {
        if (revolveDialog && revolveDialog->isResultPreviewActive()) {
            revolveDialog->setResultPreviewActive(false);
        }
        if (revolveDialog) {
            setFeatureOperationGhostMode(true);
        }
        updateRevolveHandles();
        refreshRevolveLivePreview();
    }
    if (statusBar()) {
        statusBar()->showMessage(tr("已取消结果预览，可继续调整参数"), 2500);
    }
}

void Widget::cleanupExtrudeRevolveDialogSession()
{
    // 使已排队的选中后延迟刷新失效，避免关闭后重新画出红边/手柄
    ++extrudeRevolveSelectionEpoch_;

    featureResultPreviewActive_ = false;
    restoreModelsAfterResultPreview();
    clearFeatureLivePreview();
    clearExtrusionHandles();
    clearRevolveHandles();
    clearVectorDialogArrowPreview();
    clearSelectedPoint();

    extrusionSelectedFaces.clear();
    extrusionSelectedIndices.clear();
    hasHoveredFace = false;
    extrusionHandleHover_ = ExtrusionHandlePart::None;
    extrusionHandleSelected_ = ExtrusionHandlePart::None;
    extrusionHandleDrag_ = ExtrusionHandlePart::None;
    extrusionHandleDragging_ = false;
    revolveHandleHover_ = RevolveHandlePart::None;
    revolveHandleSelected_ = RevolveHandlePart::None;
    revolveHandleDrag_ = RevolveHandlePart::None;
    revolveHandleDragging_ = false;

    // 先退出幽灵模式，再清高亮：否则 clear 会按幽灵样式“恢复”留下橙轮廓
    setFeatureOperationGhostMode(false);
    clearExtrusionSelectionHighlight();
    clearExtrusionFaceHighlight();

    if (originSnapSelectionActive_) {
        originSnapSelectionActive_ = false;
        pendingOriginDialogKind_ = OriginDialogKind::None;
        pendingOriginSnapKind_ = -1;
        clearSnapSettings();
    }

    currentSelectionMode = None;
    setFeatureGizmoActorsPickable(true);

    if (vtkWidget && vtkWidget->renderWindow() && vtkWidget->renderWindow()->GetInteractor()) {
        vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::refreshExtrusionLivePreview()
{
    if (!extrusionDialog || !renderer || !vtkWidget) return;
    if (featureResultPreviewActive_) return;
    try {
        TopoDS_Shape shape;
        if (!buildExtrusionPreviewShape(extrusionDialog, shape, true) || shape.IsNull()) {
            clearFeatureLivePreview();
            if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            return;
        }

        // 使用与 displayOccShape 相同的 IVtk 网格管线，避免 OccConverter 对薄片/扫掠面偶发崩溃
        BRepMesh_IncrementalMesh mesh(shape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(900000001);
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        if (!meshPolyData || meshPolyData->GetNumberOfPoints() == 0) {
            clearFeatureLivePreview();
            return;
        }

        vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
        pd->DeepCopy(meshPolyData);
        pd->SetLines(nullptr);
        pd->SetVerts(nullptr);

        clearFeatureLivePreview();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(pd);
        previewActor = vtkSmartPointer<vtkActor>::New();
        previewActor->SetMapper(mapper);
        previewActor->GetProperty()->SetColor(kPreviewR, kPreviewG, kPreviewB);
        configureTranslucentSolidActor(previewActor, kPreviewOpacity);
        // 预览相对幽灵体更靠前，避免与截面共面时对角线闪烁
        configureTranslucentSolidMapper(mapper, -2.0, -2.0);
        previewActor->SetPickable(false);
        addAppearanceActor(previewActor);
        // 预览在覆盖层：必须把其包围盒并入裁切范围，否则斜视/拉长时会被近远平面切掉
        refreshCameraClippingRange();
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    } catch (Standard_Failure&) {
        clearFeatureLivePreview();
    } catch (...) {
        clearFeatureLivePreview();
    }
}

void Widget::refreshRevolveLivePreview()
{
    if (!revolveDialog || !renderer || !vtkWidget) return;
    if (featureResultPreviewActive_) return;
    try {
        TopoDS_Shape shape;
        if (!buildRevolvePreviewShape(revolveDialog, shape, true) || shape.IsNull()) {
            clearFeatureLivePreview();
            if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            return;
        }

        BRepMesh_IncrementalMesh mesh(shape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(900000002);
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        if (!meshPolyData || meshPolyData->GetNumberOfPoints() == 0) {
            clearFeatureLivePreview();
            return;
        }

        vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
        pd->DeepCopy(meshPolyData);
        pd->SetLines(nullptr);
        pd->SetVerts(nullptr);

        clearFeatureLivePreview();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(pd);
        previewActor = vtkSmartPointer<vtkActor>::New();
        previewActor->SetMapper(mapper);
        previewActor->GetProperty()->SetColor(kPreviewR, kPreviewG, kPreviewB);
        configureTranslucentSolidActor(previewActor, kPreviewOpacity);
        configureTranslucentSolidMapper(mapper, -2.0, -2.0);
        previewActor->SetPickable(false);
        addAppearanceActor(previewActor);
        refreshCameraClippingRange();
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    } catch (Standard_Failure&) {
        clearFeatureLivePreview();
    } catch (...) {
        clearFeatureLivePreview();
    }
}

void Widget::clearExtrusionHandles()
{
    auto remove = [&](vtkSmartPointer<vtkActor>& a) {
        if (a) removeSceneActor(a);
        a = nullptr;
    };
    remove(extrusionHandleLineActor_);
    remove(extrusionHandleSphereActor_);
    remove(extrusionHandleArrowActor_);
    // 不在此处清 hover/selected/dragging：update 会 rebuild，清掉会导致悬浮色永远刷不出来
}

void Widget::setFeatureGizmoActorsPickable(bool pickable)
{
    if (extrusionHandleSphereActor_) extrusionHandleSphereActor_->SetPickable(pickable ? 1 : 0);
    if (extrusionHandleArrowActor_) extrusionHandleArrowActor_->SetPickable(pickable ? 1 : 0);
    if (extrusionHandleLineActor_) extrusionHandleLineActor_->SetPickable(0);
    if (revolveHandleSphereActor_) revolveHandleSphereActor_->SetPickable(pickable ? 1 : 0);
    if (revolveHandleArrowActor_) revolveHandleArrowActor_->SetPickable(pickable ? 1 : 0);
    if (revolveHandleArcActor_) revolveHandleArcActor_->SetPickable(0);
    if (revolveHandleStartLineActor_) revolveHandleStartLineActor_->SetPickable(0);
    if (revolveHandleEndLineActor_) revolveHandleEndLineActor_->SetPickable(0);
    if (revolveHandleCenterActor_) revolveHandleCenterActor_->SetPickable(0);
    if (previewActor) previewActor->SetPickable(0);
    if (vectorDialogArrowActor_) vectorDialogArrowActor_->SetPickable(0);
    if (vectorDialogHoverShapeActor_) vectorDialogHoverShapeActor_->SetPickable(0);
    if (vectorDialogHoverOutlineActor_) vectorDialogHoverOutlineActor_->SetPickable(0);
}

void Widget::updateExtrusionHandles()
{
    if (!extrusionDialog || !renderer || !vtkWidget) {
        clearExtrusionHandles();
        extrusionHandleDragging_ = false;
        extrusionHandleDrag_ = ExtrusionHandlePart::None;
        extrusionHandleHover_ = ExtrusionHandlePart::None;
        extrusionHandleSelected_ = ExtrusionHandlePart::None;
        return;
    }
    if (featureResultPreviewActive_) {
        clearExtrusionHandles();
        return;
    }
    if (extrusionSelectedFaces.isEmpty()) {
        clearExtrusionHandles();
        extrusionHandleDragging_ = false;
        extrusionHandleDrag_ = ExtrusionHandlePart::None;
        extrusionHandleHover_ = ExtrusionHandlePart::None;
        extrusionHandleSelected_ = ExtrusionHandlePart::None;
        return;
    }

    // 距离操作柄已表达拉伸轴：隐藏矢量预览箭头，避免与起始球重叠成“球+大圆锥”
    if (vectorDialogArrowActor_) {
        vectorDialogArrowActor_->SetVisibility(false);
    }

    gp_Pnt center;
    if (!computeExtrusionProfileCenter(center)) {
        clearExtrusionHandles();
        return;
    }

    const gp_Dir dir = getExtrusionDirection(extrusionDialog);
    const double startD = extrusionDialog->getStartDistance();
    const double endD = extrusionDialog->getEndDistance();
    const gp_Pnt pStart = center.Translated(gp_Vec(dir) * startD);
    const gp_Pnt pEnd = center.Translated(gp_Vec(dir) * endD);

    // 拖拽中：只更新位置，不销毁 Actor（否则指针失效且 dragging 会被打乱）
    if (extrusionHandleDragging_
        && extrusionHandleSphereActor_ && extrusionHandleArrowActor_ && extrusionHandleLineActor_) {
        extrusionHandleSphereActor_->SetPosition(pStart.X(), pStart.Y(), pStart.Z());

        const bool sphereActive = (extrusionHandleDrag_ == ExtrusionHandlePart::StartSphere);
        const bool arrowActive = (extrusionHandleDrag_ == ExtrusionHandlePart::EndArrow);
        const HandleStateStyle sphereStyle = extrusionCtrlStyle(
            kCtrlStartSphere, sphereActive ? ControlState::Drag : ControlState::Default);
        const HandleStateStyle arrowStyle = extrusionCtrlStyle(
            kCtrlEndArrow, arrowActive ? ControlState::Drag : ControlState::Default);
        const HandleStateStyle lineStyle = (sphereActive || arrowActive)
            ? extrusionCtrlStyle(kCtrlEndArrow, ControlState::Drag)
            : extrusionCtrlStyle(kCtrlEndArrow, ControlState::Default);

        HandleGeom::applyStateStyle(extrusionHandleSphereActor_, sphereStyle,
                                    overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z()),
                                    /*applyActorScale=*/true);
        HandleGeom::applyStateStyle(extrusionHandleArrowActor_->GetProperty(), arrowStyle);
        HandleGeom::applyStateStyle(extrusionHandleLineActor_->GetProperty(), lineStyle);

        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(pStart.X(), pStart.Y(), pStart.Z());
        line->SetPoint2(pEnd.X(), pEnd.Y(), pEnd.Z());
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(extrusionHandleLineActor_->GetMapper())) {
            mapper->SetInputConnection(line->GetOutputPort());
            mapper->Modified();
        }

        const double arrowLen = kExtrudeHandleArrowLen
            * arrowStyle.scaleFactor
            * overlayWorldScaleAt(pEnd.X(), pEnd.Y(), pEnd.Z());
        auto tf = HandleGeom::makeOrientedArrow(
            ControlShape::ArrowWithShaft, pEnd, dir, arrowLen,
            HandleGeom::defaultShaftArrowParams());
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(extrusionHandleArrowActor_->GetMapper())) {
            mapper->SetInputConnection(tf->GetOutputPort());
            mapper->Modified();
        }
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }

    // 悬浮/选择态：若 Actor 已存在，只刷新样式与几何，避免重建把状态冲掉
    if (!extrusionHandleDragging_
        && extrusionHandleSphereActor_ && extrusionHandleArrowActor_ && extrusionHandleLineActor_) {
        extrusionHandleSphereActor_->SetPosition(pStart.X(), pStart.Y(), pStart.Z());

        const bool sphereSelected = extrusionHandleSelected_ == ExtrusionHandlePart::StartSphere;
        const bool sphereHover = extrusionHandleSelected_ == ExtrusionHandlePart::None
            && extrusionHandleHover_ == ExtrusionHandlePart::StartSphere;
        const bool arrowSelected = extrusionHandleSelected_ == ExtrusionHandlePart::EndArrow;
        const bool arrowHover = extrusionHandleSelected_ == ExtrusionHandlePart::None
            && extrusionHandleHover_ == ExtrusionHandlePart::EndArrow;

        const ControlState sphereSt = HandleGeom::resolveControlState(false, sphereSelected, sphereHover);
        const ControlState arrowSt = HandleGeom::resolveControlState(false, arrowSelected, arrowHover);
        const ControlState lineSt = HandleGeom::resolveControlState(
            false,
            extrusionHandleSelected_ != ExtrusionHandlePart::None,
            extrusionHandleHover_ != ExtrusionHandlePart::None);
        const HandleStateStyle sphereStyle = extrusionCtrlStyle(kCtrlStartSphere, sphereSt);
        const HandleStateStyle arrowStyle = extrusionCtrlStyle(kCtrlEndArrow, arrowSt);
        const HandleStateStyle lineStyle = extrusionCtrlStyle(kCtrlEndArrow, lineSt);

        HandleGeom::applyStateStyle(extrusionHandleSphereActor_, sphereStyle,
                                    overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z()), true);
        HandleGeom::applyStateStyle(extrusionHandleLineActor_->GetProperty(), lineStyle);

        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(pStart.X(), pStart.Y(), pStart.Z());
        line->SetPoint2(pEnd.X(), pEnd.Y(), pEnd.Z());
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(extrusionHandleLineActor_->GetMapper())) {
            mapper->SetInputConnection(line->GetOutputPort());
            mapper->Modified();
        }

        const double arrowLen = kExtrudeHandleArrowLen
            * arrowStyle.scaleFactor
            * overlayWorldScaleAt(pEnd.X(), pEnd.Y(), pEnd.Z());
        auto tf = HandleGeom::makeOrientedArrow(
            ControlShape::ArrowWithShaft, pEnd, dir, arrowLen,
            HandleGeom::defaultShaftArrowParams());
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(extrusionHandleArrowActor_->GetMapper())) {
            mapper->SetInputConnection(tf->GetOutputPort());
            mapper->Modified();
        }
        HandleGeom::applyStateStyle(extrusionHandleArrowActor_->GetProperty(), arrowStyle);
        extrusionHandleSphereActor_->SetPickable(true);
        extrusionHandleArrowActor_->SetPickable(true);
        if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }

    const bool keepDragging = extrusionHandleDragging_;
    const ExtrusionHandlePart keepPart = extrusionHandleDrag_;
    const ExtrusionHandlePart keepHover = extrusionHandleHover_;
    const ExtrusionHandlePart keepSelected = extrusionHandleSelected_;
    clearExtrusionHandles();
    extrusionHandleDragging_ = keepDragging;
    extrusionHandleDrag_ = keepPart;
    extrusionHandleHover_ = keepHover;
    extrusionHandleSelected_ = keepSelected;

    // 轨迹线（导引角色：跟随任一控制柄的交互态）
    {
        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(pStart.X(), pStart.Y(), pStart.Z());
        line->SetPoint2(pEnd.X(), pEnd.Y(), pEnd.Z());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(line->GetOutputPort());
        extrusionHandleLineActor_ = vtkSmartPointer<vtkActor>::New();
        extrusionHandleLineActor_->SetMapper(mapper);
        const ControlState lineState = HandleGeom::resolveControlState(
            extrusionHandleDragging_,
            extrusionHandleSelected_ != ExtrusionHandlePart::None,
            extrusionHandleHover_ != ExtrusionHandlePart::None);
        HandleGeom::applyStateStyle(extrusionHandleLineActor_->GetProperty(),
                                    extrusionCtrlStyle(kCtrlEndArrow, lineState));
        extrusionHandleLineActor_->SetPickable(false);
        addReferenceActor(extrusionHandleLineActor_);
    }
    // 起始球（球形控制柄）
    {
        auto sph = HandleGeom::makeSphereSource(
            HandleGeom::defaultHandleSphereParams(kExtrudeHandleSphereR));
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sph->GetOutputPort());
        extrusionHandleSphereActor_ = vtkSmartPointer<vtkActor>::New();
        extrusionHandleSphereActor_->SetMapper(mapper);
        extrusionHandleSphereActor_->SetPosition(pStart.X(), pStart.Y(), pStart.Z());
        const bool sphereDragging = extrusionHandleDragging_ && extrusionHandleDrag_ == ExtrusionHandlePart::StartSphere;
        const bool sphereSelected = !extrusionHandleDragging_ && extrusionHandleSelected_ == ExtrusionHandlePart::StartSphere;
        const bool sphereHover = !extrusionHandleDragging_ && extrusionHandleSelected_ == ExtrusionHandlePart::None
            && extrusionHandleHover_ == ExtrusionHandlePart::StartSphere;
        const ControlState st = HandleGeom::resolveControlState(sphereDragging, sphereSelected, sphereHover);
        HandleGeom::applyStateStyle(extrusionHandleSphereActor_,
                                    extrusionCtrlStyle(kCtrlStartSphere, st),
                                    overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z()),
                                    true);
        extrusionHandleSphereActor_->SetPickable(true);
        addReferenceActor(extrusionHandleSphereActor_);
    }
    // 终止箭头（含箭柄，固定屏幕尺寸）
    {
        const bool arrowDragging = extrusionHandleDragging_ && extrusionHandleDrag_ == ExtrusionHandlePart::EndArrow;
        const bool arrowSelected = !extrusionHandleDragging_ && extrusionHandleSelected_ == ExtrusionHandlePart::EndArrow;
        const bool arrowHover = !extrusionHandleDragging_ && extrusionHandleSelected_ == ExtrusionHandlePart::None
            && extrusionHandleHover_ == ExtrusionHandlePart::EndArrow;
        const ControlState st = HandleGeom::resolveControlState(arrowDragging, arrowSelected, arrowHover);
        const HandleStateStyle arrowStyle = extrusionCtrlStyle(kCtrlEndArrow, st);
        const double arrowLen = kExtrudeHandleArrowLen
            * arrowStyle.scaleFactor
            * overlayWorldScaleAt(pEnd.X(), pEnd.Y(), pEnd.Z());
        auto tf = HandleGeom::makeOrientedArrow(
            ControlShape::ArrowWithShaft, pEnd, dir, arrowLen,
            HandleGeom::defaultShaftArrowParams());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());
        extrusionHandleArrowActor_ = vtkSmartPointer<vtkActor>::New();
        extrusionHandleArrowActor_->SetMapper(mapper);
        HandleGeom::applyStateStyle(extrusionHandleArrowActor_->GetProperty(), arrowStyle);
        extrusionHandleArrowActor_->SetPickable(true);
        addReferenceActor(extrusionHandleArrowActor_);
    }

    if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::clearRevolveHandles()
{
    auto remove = [&](vtkSmartPointer<vtkActor>& a) {
        if (a) removeSceneActor(a);
        a = nullptr;
    };
    remove(revolveHandleArcActor_);
    remove(revolveHandleStartLineActor_);
    remove(revolveHandleEndLineActor_);
    remove(revolveHandleSphereActor_);
    remove(revolveHandleArrowActor_);
    remove(revolveHandleCenterActor_);
    // 不在此处清 hover/selected/dragging（同拉伸手柄）
}

void Widget::updateRevolveHandles()
{
    if (!revolveDialog || !renderer || !vtkWidget) {
        clearRevolveHandles();
        revolveHandleDragging_ = false;
        revolveHandleDrag_ = RevolveHandlePart::None;
        revolveHandleHover_ = RevolveHandlePart::None;
        revolveHandleSelected_ = RevolveHandlePart::None;
        return;
    }
    if (featureResultPreviewActive_) {
        clearRevolveHandles();
        return;
    }
    if (extrusionSelectedFaces.isEmpty()) {
        clearRevolveHandles();
        revolveHandleDragging_ = false;
        revolveHandleDrag_ = RevolveHandlePart::None;
        revolveHandleHover_ = RevolveHandlePart::None;
        revolveHandleSelected_ = RevolveHandlePart::None;
        return;
    }

    gp_Pnt center;
    if (!computeExtrusionProfileCenter(center)) {
        clearRevolveHandles();
        return;
    }

    const bool keepDragging = revolveHandleDragging_;
    const RevolveHandlePart keepPart = revolveHandleDrag_;
    const RevolveHandlePart keepHover = revolveHandleHover_;
    const RevolveHandlePart keepSelected = revolveHandleSelected_;
    // 拖拽中全量重建前先保留状态（本函数末尾会恢复）
    clearRevolveHandles();
    revolveHandleDragging_ = keepDragging;
    revolveHandleDrag_ = keepPart;
    revolveHandleHover_ = keepHover;
    revolveHandleSelected_ = keepSelected;

    const gp_Ax1 axis = getRevolutionAxis(revolveDialog);
    const gp_Pnt O = axis.Location();
    const gp_Dir az = axis.Direction();

    // 半径：剖面中心到轴的距离，下限保护
    gp_Vec oc(O, center);
    const double axial = oc.Dot(gp_Vec(az));
    gp_Pnt foot = O.Translated(gp_Vec(az) * axial);
    gp_Vec rvec(foot, center);
    double radius = rvec.Magnitude();
    if (radius < 1e-3) radius = 2.0;

    // 保证径向与轴向正交，避免 gp_Ax2 / Crossed 抛 ConstructionError
    gp_Vec rOrtho = rvec - gp_Vec(az) * rvec.Dot(gp_Vec(az));
    gp_Dir rdir(1, 0, 0);
    if (rOrtho.Magnitude() > 1e-8) {
        rdir = gp_Dir(rOrtho);
    } else {
        gp_Vec vx = gp_Vec(az).Crossed(gp_Vec(0, 0, 1));
        if (vx.Magnitude() < 1e-8) vx = gp_Vec(az).Crossed(gp_Vec(1, 0, 0));
        if (vx.Magnitude() > 1e-8) rdir = gp_Dir(vx);
    }

    const double startA = revolveDialog->getStartAngle() * M_PI / 180.0;
    const double endA = revolveDialog->getEndAngle() * M_PI / 180.0;

    auto pointAtAngle = [&](double ang) -> gp_Pnt {
        gp_Trsf rot;
        rot.SetRotation(gp_Ax1(foot, az), ang);
        gp_Pnt p = foot.Translated(gp_Vec(rdir) * radius);
        p.Transform(rot);
        return p;
    };

    const gp_Pnt pStart = pointAtAngle(startA);
    const gp_Pnt pEnd = pointAtAngle(endA);

    clearRevolveHandles();

    // 圆心球
    {
        auto sph = HandleGeom::makeSphereSource(
            HandleGeom::defaultHandleSphereParams(kRevolveHandleCenterR));
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sph->GetOutputPort());
        revolveHandleCenterActor_ = vtkSmartPointer<vtkActor>::New();
        revolveHandleCenterActor_->SetMapper(mapper);
        revolveHandleCenterActor_->SetPosition(foot.X(), foot.Y(), foot.Z());
        {
            const double s = overlayWorldScaleAt(foot.X(), foot.Y(), foot.Z());
            revolveHandleCenterActor_->SetScale(s, s, s);
        }
        revolveHandleCenterActor_->GetProperty()->SetColor(1.0, 0.55, 0.15);
        revolveHandleCenterActor_->SetPickable(false);
        addReferenceActor(revolveHandleCenterActor_);
    }

    // 圆弧带
    {
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        double sweep = endA - startA;
        int segs = std::max(8, static_cast<int>(std::abs(sweep) * 180.0 / M_PI));
        segs = std::min(segs, 180);
        for (int i = 0; i <= segs; ++i) {
            const double a = startA + sweep * (static_cast<double>(i) / segs);
            gp_Pnt p = pointAtAngle(a);
            pts->InsertNextPoint(p.X(), p.Y(), p.Z());
        }
        vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
        poly->SetPoints(pts);
        vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
        for (vtkIdType i = 0; i < pts->GetNumberOfPoints() - 1; ++i) {
            vtkIdType ids[2] = { i, i + 1 };
            lines->InsertNextCell(2, ids);
        }
        poly->SetLines(lines);
        vtkSmartPointer<vtkTubeFilter> tube = vtkSmartPointer<vtkTubeFilter>::New();
        tube->SetInputData(poly);
        // 管径随屏幕缩放，避免滚轮推进后圆弧变粗
        tube->SetRadius(0.02 * overlayWorldScaleAt(foot.X(), foot.Y(), foot.Z()));
        tube->SetNumberOfSides(10);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tube->GetOutputPort());
        revolveHandleArcActor_ = vtkSmartPointer<vtkActor>::New();
        revolveHandleArcActor_->SetMapper(mapper);
        revolveHandleArcActor_->GetProperty()->SetColor(kArcBlueR, kArcBlueG, kArcBlueB);
        const bool dragging = revolveHandleDragging_;
        const bool selected = (revolveHandleSelected_ != RevolveHandlePart::None);
        const bool hovered = (revolveHandleHover_ != RevolveHandlePart::None);
        const double arcOpacity = dragging ? 0.55 : selected ? 0.45 : hovered ? 0.4 : 0.35;
        revolveHandleArcActor_->GetProperty()->SetOpacity(arcOpacity);
        revolveHandleArcActor_->SetPickable(false);
        addReferenceActor(revolveHandleArcActor_);
    }

    auto makeRadialLine = [&](const gp_Pnt& tip, vtkSmartPointer<vtkActor>& actor) {
        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(foot.X(), foot.Y(), foot.Z());
        line->SetPoint2(tip.X(), tip.Y(), tip.Z());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(line->GetOutputPort());
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(kArcBlueR, kArcBlueG, kArcBlueB);
        actor->GetProperty()->SetLineWidth(1.5);
        actor->SetPickable(false);
        addReferenceActor(actor);
    };
    makeRadialLine(pStart, revolveHandleStartLineActor_);
    makeRadialLine(pEnd, revolveHandleEndLineActor_);

    // 起始球
    {
        auto sph = HandleGeom::makeSphereSource(
            HandleGeom::defaultHandleSphereParams(kRevolveHandleSphereR));
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sph->GetOutputPort());
        revolveHandleSphereActor_ = vtkSmartPointer<vtkActor>::New();
        revolveHandleSphereActor_->SetMapper(mapper);
        revolveHandleSphereActor_->SetPosition(pStart.X(), pStart.Y(), pStart.Z());
        const bool sphereDragging = revolveHandleDragging_ && revolveHandleDrag_ == RevolveHandlePart::StartSphere;
        const bool sphereSelected = !revolveHandleDragging_ && revolveHandleSelected_ == RevolveHandlePart::StartSphere;
        const bool sphereHover = !revolveHandleDragging_ && revolveHandleSelected_ == RevolveHandlePart::None
            && revolveHandleHover_ == RevolveHandlePart::StartSphere;
        const ControlState st = HandleGeom::resolveControlState(sphereDragging, sphereSelected, sphereHover);
        HandleGeom::applyStateStyle(revolveHandleSphereActor_,
                                    revolveCtrlStyle(kCtrlStartSphere, st),
                                    overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z()),
                                    true);
        revolveHandleSphereActor_->SetPickable(true);
        addReferenceActor(revolveHandleSphereActor_);
    }
    // 终止箭头：切向指向「增大终止角」方向（右手系：轴 × 径向）
    {
        gp_Vec radial(foot, pEnd);
        radial = radial - gp_Vec(az) * radial.Dot(gp_Vec(az));
        gp_Vec tangent = gp_Vec(az).Crossed(radial);
        if (tangent.Magnitude() < 1e-9) tangent = gp_Vec(1, 0, 0);
        gp_Dir tdir(tangent);
        const bool arrowDragging = revolveHandleDragging_ && revolveHandleDrag_ == RevolveHandlePart::EndArrow;
        const bool arrowSelected = !revolveHandleDragging_ && revolveHandleSelected_ == RevolveHandlePart::EndArrow;
        const bool arrowHover = !revolveHandleDragging_ && revolveHandleSelected_ == RevolveHandlePart::None
            && revolveHandleHover_ == RevolveHandlePart::EndArrow;
        const ControlState st = HandleGeom::resolveControlState(arrowDragging, arrowSelected, arrowHover);
        const HandleStateStyle arrowStyle = revolveCtrlStyle(kCtrlEndArrow, st);
        const double arrowLen = kRevolveHandleArrowLen
            * arrowStyle.scaleFactor
            * overlayWorldScaleAt(pEnd.X(), pEnd.Y(), pEnd.Z());
        auto tf = HandleGeom::makeOrientedArrow(
            ControlShape::ArrowWithShaft, pEnd, tdir, arrowLen,
            HandleGeom::defaultShaftArrowParams());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());
        revolveHandleArrowActor_ = vtkSmartPointer<vtkActor>::New();
        revolveHandleArrowActor_->SetMapper(mapper);
        HandleGeom::applyStateStyle(revolveHandleArrowActor_->GetProperty(), arrowStyle);
        revolveHandleArrowActor_->SetPickable(true);
        addReferenceActor(revolveHandleArrowActor_);
    }

    // 轴方向已由旋转弧/起止控制柄表达；不再在中心叠矢量预览箭头（会盖住中心球）
    if (vectorDialogArrowActor_) {
        vectorDialogArrowActor_->SetVisibility(false);
    }

    if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::handleExtrusionHandleMouseDown(int x, int y)
{
    if (!renderer || !extrusionDialog || featureResultPreviewActive_) return;
    if (!extrusionHandleSphereActor_ && !extrusionHandleArrowActor_) return;

    // 仅对手柄做 PropPicker，避免被模型面挡住
    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->PickFromListOn();
    picker->InitializePickList();
    if (extrusionHandleSphereActor_) picker->AddPickList(extrusionHandleSphereActor_);
    if (extrusionHandleArrowActor_) picker->AddPickList(extrusionHandleArrowActor_);
    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* hit = picker->GetActor();
    if (hit == extrusionHandleSphereActor_.GetPointer()) {
        extrusionHandleSavedMode_ = currentSelectionMode;
        extrusionHandleSelected_ = ExtrusionHandlePart::StartSphere;
        extrusionHandleDrag_ = ExtrusionHandlePart::None;
        extrusionHandleDragging_ = false;
        extrusionHandleSelectDownX_ = x;
        extrusionHandleSelectDownY_ = y;
        currentSelectionMode = ExtrusionHandleDrag;
    } else if (hit == extrusionHandleArrowActor_.GetPointer()) {
        extrusionHandleSavedMode_ = currentSelectionMode;
        extrusionHandleSelected_ = ExtrusionHandlePart::EndArrow;
        extrusionHandleDrag_ = ExtrusionHandlePart::None;
        extrusionHandleDragging_ = false;
        extrusionHandleSelectDownX_ = x;
        extrusionHandleSelectDownY_ = y;
        currentSelectionMode = ExtrusionHandleDrag;
    } else {
        extrusionHandleDrag_ = ExtrusionHandlePart::None;
        extrusionHandleDragging_ = false;
        extrusionHandleSelected_ = ExtrusionHandlePart::None;
    }

    // 进入“选择态”并刷新一次手柄样式（不改变距离参数）
    updateExtrusionHandles();
    refreshExtrusionLivePreview();
}

void Widget::handleExtrusionHandleMouseMove(int x, int y)
{
    if (!extrusionDialog || !renderer || featureResultPreviewActive_) return;

    if (!extrusionHandleDragging_) {
        // 1) 选择态：按下但尚未进入拖拽
        if (extrusionHandleSelected_ != ExtrusionHandlePart::None) {
            const int dx = x - extrusionHandleSelectDownX_;
            const int dy = y - extrusionHandleSelectDownY_;
            if (dx * dx + dy * dy <= kHandleSelectToDragThresholdPx2) {
                updateExtrusionHandles();
                refreshExtrusionLivePreview();
                return;
            }

            // 大于阈值：正式进入拖拽
            extrusionHandleDragging_ = true;
            extrusionHandleDrag_ = extrusionHandleSelected_;
            extrusionHandleSelected_ = ExtrusionHandlePart::None;
        } else {
            // 2) 悬浮态拾取（主渲染器 + 参考叠加层都试，避免 InteractiveOff 漏拾取）
            if (!extrusionHandleSphereActor_ && !extrusionHandleArrowActor_) return;
            auto pickHandle = [&](vtkRenderer* r) -> vtkActor* {
                if (!r) return nullptr;
                vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
                picker->PickFromListOn();
                picker->InitializePickList();
                if (extrusionHandleSphereActor_) picker->AddPickList(extrusionHandleSphereActor_);
                if (extrusionHandleArrowActor_) picker->AddPickList(extrusionHandleArrowActor_);
                picker->Pick(x, y, 0, r);
                return picker->GetActor();
            };
            vtkActor* hit = pickHandle(referenceOverlay());
            if (!hit) hit = pickHandle(renderer.GetPointer());
            const ExtrusionHandlePart hover =
                (hit == extrusionHandleSphereActor_.GetPointer()) ? ExtrusionHandlePart::StartSphere
                : (hit == extrusionHandleArrowActor_.GetPointer()) ? ExtrusionHandlePart::EndArrow
                                                                   : ExtrusionHandlePart::None;
            if (hover != extrusionHandleHover_) {
                extrusionHandleHover_ = hover;
                updateExtrusionHandles();
            }
            return;
        }
    }

    gp_Pnt center;
    if (!computeExtrusionProfileCenter(center)) return;
    const gp_Dir dir = getExtrusionDirection(extrusionDialog);

    // 必须与相机朝向平面求交，再投影到拉伸轴；若用法向=轴方向，交点投影距离恒为 ~0
    gp_Dir planeN(0, 0, 1);
    if (auto* cam = renderer->GetActiveCamera()) {
        double fp[3];
        cam->GetDirectionOfProjection(fp);
        const double mag = std::sqrt(fp[0] * fp[0] + fp[1] * fp[1] + fp[2] * fp[2]);
        if (mag > 1e-12) planeN = gp_Dir(fp[0] / mag, fp[1] / mag, fp[2] / mag);
    }
    gp_Pnt hit;
    if (!rayPlaneHit(renderer, x, y, center, planeN, hit)) return;

    const double distRaw = gp_Vec(center, hit).Dot(gp_Vec(dir));
    if (extrusionHandleDrag_ == ExtrusionHandlePart::StartSphere) {
        const double dist = applyDistanceCr(QString::fromLatin1(kExtrusionHandleId),
                                            QString::fromLatin1(kCtrlStartSphere), distRaw);
        extrusionDialog->setStartDistance(dist);
    } else if (extrusionHandleDrag_ == ExtrusionHandlePart::EndArrow) {
        const double dist = applyDistanceCr(QString::fromLatin1(kExtrusionHandleId),
                                            QString::fromLatin1(kCtrlEndArrow), distRaw);
        extrusionDialog->setEndDistance(dist);
    }
    updateExtrusionHandles();
    refreshExtrusionLivePreview();
}

void Widget::handleExtrusionHandleMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    extrusionHandleDragging_ = false;
    extrusionHandleDrag_ = ExtrusionHandlePart::None;
    extrusionHandleSelected_ = ExtrusionHandlePart::None;
    extrusionHandleSelectDownX_ = extrusionHandleSelectDownY_ = -1;
    if (currentSelectionMode == ExtrusionHandleDrag) {
        SelectionMode restore = extrusionHandleSavedMode_;
        if (restore == ExtrusionHandleDrag || restore == None) {
            if (extrusionDialog) {
                restore = (extrusionDialog->getSectionType() == QStringLiteral("曲线"))
                    ? EdgeSelection : FaceSelection;
            } else {
                restore = None;
            }
        }
        currentSelectionMode = restore;
    }

    // 离开选择/拖拽态后，刷新一次手柄样式回到默认（或悬浮由下一次 mouse move 接管）
    updateExtrusionHandles();
    refreshExtrusionLivePreview();
}

void Widget::handleRevolveHandleMouseDown(int x, int y)
{
    if (!renderer || !revolveDialog || featureResultPreviewActive_) return;
    if (!revolveHandleSphereActor_ && !revolveHandleArrowActor_) return;

    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->PickFromListOn();
    picker->InitializePickList();
    if (revolveHandleSphereActor_) picker->AddPickList(revolveHandleSphereActor_);
    if (revolveHandleArrowActor_) picker->AddPickList(revolveHandleArrowActor_);
    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* hit = picker->GetActor();
    if (hit == revolveHandleSphereActor_.GetPointer()) {
        revolveHandleSavedMode_ = currentSelectionMode;
        revolveHandleSelected_ = RevolveHandlePart::StartSphere;
        revolveHandleDrag_ = RevolveHandlePart::None;
        revolveHandleDragging_ = false;
        revolveHandleSelectDownX_ = x;
        revolveHandleSelectDownY_ = y;
        currentSelectionMode = RevolveHandleDrag;
    } else if (hit == revolveHandleArrowActor_.GetPointer()) {
        revolveHandleSavedMode_ = currentSelectionMode;
        revolveHandleSelected_ = RevolveHandlePart::EndArrow;
        revolveHandleDrag_ = RevolveHandlePart::None;
        revolveHandleDragging_ = false;
        revolveHandleSelectDownX_ = x;
        revolveHandleSelectDownY_ = y;
        currentSelectionMode = RevolveHandleDrag;
    } else {
        revolveHandleDrag_ = RevolveHandlePart::None;
        revolveHandleDragging_ = false;
        revolveHandleSelected_ = RevolveHandlePart::None;
    }

    // 进入“选择态”，先只刷新样式，不改变角度参数
    updateRevolveHandles();
    refreshRevolveLivePreview();
}

void Widget::handleRevolveHandleMouseMove(int x, int y)
{
    if (!revolveDialog || !renderer || featureResultPreviewActive_) return;

    if (!revolveHandleDragging_) {
        // 1) 选择态：按下但尚未进入拖拽
        if (revolveHandleSelected_ != RevolveHandlePart::None) {
            const int dx = x - revolveHandleSelectDownX_;
            const int dy = y - revolveHandleSelectDownY_;
            if (dx * dx + dy * dy <= kHandleSelectToDragThresholdPx2) {
                updateRevolveHandles();
                refreshRevolveLivePreview();
                return;
            }

            // 大于阈值：正式进入拖拽
            revolveHandleDragging_ = true;
            revolveHandleDrag_ = revolveHandleSelected_;
            revolveHandleSelected_ = RevolveHandlePart::None;
            revolveHandleDragGrabDeg_ =
                (revolveHandleDrag_ == RevolveHandlePart::StartSphere)
                    ? revolveDialog->getStartAngle()
                    : revolveDialog->getEndAngle();
        } else {
            // 2) 悬浮态拾取（主渲染器 + 参考叠加层）
            if (!revolveHandleSphereActor_ && !revolveHandleArrowActor_) return;
            auto pickHandle = [&](vtkRenderer* r) -> vtkActor* {
                if (!r) return nullptr;
                vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
                picker->PickFromListOn();
                picker->InitializePickList();
                if (revolveHandleSphereActor_) picker->AddPickList(revolveHandleSphereActor_);
                if (revolveHandleArrowActor_) picker->AddPickList(revolveHandleArrowActor_);
                picker->Pick(x, y, 0, r);
                return picker->GetActor();
            };
            vtkActor* hit = pickHandle(referenceOverlay());
            if (!hit) hit = pickHandle(renderer.GetPointer());
            const RevolveHandlePart hover =
                (hit == revolveHandleSphereActor_.GetPointer()) ? RevolveHandlePart::StartSphere
                : (hit == revolveHandleArrowActor_.GetPointer()) ? RevolveHandlePart::EndArrow
                                                                : RevolveHandlePart::None;
            if (hover != revolveHandleHover_) {
                revolveHandleHover_ = hover;
                updateRevolveHandles();
            }
            return;
        }
    }

    gp_Pnt profileCenter;
    if (!computeExtrusionProfileCenter(profileCenter)) return;
    const gp_Ax1 axis = getRevolutionAxis(revolveDialog);
    const gp_Pnt O = axis.Location();
    const gp_Dir az = axis.Direction();
    gp_Vec oc(O, profileCenter);
    const double axial = oc.Dot(gp_Vec(az));
    const gp_Pnt foot = O.Translated(gp_Vec(az) * axial);

    gp_Pnt hit;
    if (!rayPlaneHit(renderer, x, y, foot, az, hit)) return;

    gp_Vec v(foot, hit);
    const double along = v.Dot(gp_Vec(az));
    v = v - gp_Vec(az) * along;
    if (v.Magnitude() < 1e-9) return;

    gp_Vec rref(foot, profileCenter);
    rref = rref - gp_Vec(az) * rref.Dot(gp_Vec(az));
    if (rref.Magnitude() < 1e-9) rref = gp_Vec(1, 0, 0);
    gp_Dir d0(rref);
    gp_Dir d1(v);
    const double angRad = std::atan2(gp_Vec(d0).Crossed(gp_Vec(d1)).Dot(gp_Vec(az)), d0.Dot(d1));
    const double rawDeg = angRad * 180.0 / M_PI;

    // 相对上次角度连续解包，避免过 ±180° 时跳到负角
    double continuous = unwrapAngleNear(revolveHandleDragGrabDeg_, rawDeg);
    continuous = std::round(continuous);
    revolveHandleDragGrabDeg_ = continuous;

    double startA = revolveDialog->getStartAngle();
    double endA = revolveDialog->getEndAngle();
    if (revolveHandleDrag_ == RevolveHandlePart::StartSphere) {
        // 只改起始角，终止角固定，两手柄独立
        startA = continuous;
        clampStartAgainstEnd(startA, endA);
        revolveHandleDragGrabDeg_ = startA;
        revolveDialog->setStartAngle(startA);
    } else {
        // 只改终止角；钳制 |sweep|≤360
        endA = continuous;
        clampSweepDeg(startA, endA);
        revolveHandleDragGrabDeg_ = endA;
        revolveDialog->setEndAngle(endA);
    }
    updateRevolveHandles();
    refreshRevolveLivePreview();
}

void Widget::handleRevolveHandleMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    revolveHandleDragging_ = false;
    revolveHandleDrag_ = RevolveHandlePart::None;
    revolveHandleSelected_ = RevolveHandlePart::None;
    revolveHandleSelectDownX_ = revolveHandleSelectDownY_ = -1;
    if (currentSelectionMode == RevolveHandleDrag) {
        SelectionMode restore = revolveHandleSavedMode_;
        if (restore == RevolveHandleDrag || restore == None) {
            if (revolveDialog) {
                restore = (revolveDialog->getSectionType() == QStringLiteral("曲线"))
                    ? EdgeSelection : FaceSelection;
            } else {
                restore = None;
            }
        }
        currentSelectionMode = restore;
    }

    updateRevolveHandles();
    refreshRevolveLivePreview();
}
