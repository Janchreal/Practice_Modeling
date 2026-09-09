// Sketch plane pick and preview helpers (split from main_window.cpp)
#include "main_window.h"
#include "rendering/model/model_display_style.h"
#include "ui_main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "presentation/dialogs/sketch/sketch_mode_dialogs.h"
#include "sketch_tool_input_dialog.h"
#include "sketch_conic_dialog.h"
#include "sketch_polygon_dialog.h"
#include "sketch_ellipse_dialog.h"
#include "application/commands/sketcheditcommand.h"
#include "geometry/sketch/sketch_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QDialog>
#include <QImage>
#include <QMessageBox>
#include <QObject>
#include <QPalette>
#include <QPushButton>
#include <QSignalBlocker>
#include <QPixmap>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolButton>
#include <Qt>

#include <BRep_Builder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_Ellipse.hxx>
#include <gp_Ax2.hxx>
#include <gp_Elips.hxx>
#include <Standard_Failure.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <gce_MakeCirc.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <gp_Lin.hxx>

#include <IVtkTools_ShapePicker.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtk_Types.hxx>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCellArray.h>
#include <vtkFeatureEdges.h>
#include <vtkLineSource.h>
#include <vtkMapper.h>
#include <vtkPlaneSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
bool Widget::tryPickPointOnPlane(const gp_Pln& pln, int x, int y, gp_Pnt& outP) const
{
    if (!renderer || !vtkWidget) return false;

    const double displayX = static_cast<double>(x);
    // 注意：x/y 来自 VTK Interactor（原点在左下），这里直接使用 VTK 显示坐标
    const double displayY = static_cast<double>(y);

    double worldNear[4] = {0, 0, 0, 1};
    double worldFar[4]  = {0, 0, 1, 1};

    renderer->SetDisplayPoint(displayX, displayY, 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(worldNear);

    renderer->SetDisplayPoint(displayX, displayY, 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(worldFar);

    if (std::abs(worldNear[3]) > 1e-10) {
        worldNear[0] /= worldNear[3];
        worldNear[1] /= worldNear[3];
        worldNear[2] /= worldNear[3];
    }
    if (std::abs(worldFar[3]) > 1e-10) {
        worldFar[0] /= worldFar[3];
        worldFar[1] /= worldFar[3];
        worldFar[2] /= worldFar[3];
    }

    const gp_Pnt rayO(worldNear[0], worldNear[1], worldNear[2]);
    const gp_Pnt rayP1(worldFar[0], worldFar[1], worldFar[2]);
    gp_Vec rayV(rayO, rayP1);
    if (rayV.Magnitude() <= Precision::Confusion()) return false;
    const gp_Dir rayD(rayV);

    // 射线-平面求交：rayO + t*rayD
    const gp_Pnt o = pln.Location();
    const gp_Dir n = pln.Axis().Direction();
    const gp_Vec w(o, rayO);
    const double denom = n.X() * rayD.X() + n.Y() * rayD.Y() + n.Z() * rayD.Z();
    if (std::abs(denom) < 1e-12) return false; // 近平行

    const double num = n.X() * w.X() + n.Y() * w.Y() + n.Z() * w.Z();
    const double t = -num / denom;
    if (t < 0) return false;

    outP = gp_Pnt(rayO.X() + rayD.X() * t,
                 rayO.Y() + rayD.Y() * t,
                 rayO.Z() + rayD.Z() * t);
    return true;
}

bool Widget::tryPickFacePlaneUnderCursor(int x, int y, gp_Pln& outPlane)
{
    if (!shapePicker || !renderer) return false;

    // 确保可见模型绑定 ShapeSource
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor && renderStateFor(historyList[i]).shapeDataSource) {
            const bool visible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            renderStateFor(historyList[i]).actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, renderStateFor(historyList[i]).actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).shapeDataSource) {
            renderStateFor(historyList[i]).shapeDataSource->Modified();
            renderStateFor(historyList[i]).shapeDataSource->Update();
        }
    }

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Face);
    shapePicker->Pick(x, y, 0);

    vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
    if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return false;

    pickedActors->InitTraversal();
    vtkActor* selectedActor = nullptr;
    while (vtkActor* a = pickedActors->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) {
            selectedActor = a;
            break;
        }
    }
    if (!selectedActor) return false;

    IVtkTools_ShapeDataSource* dataSource = IVtkTools_ShapeObject::GetShapeSource(selectedActor);
    if (!dataSource) return false;

    Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
    if (shapeWrapper.IsNull()) return false;

    const IVtk_IdType shapeID = shapeWrapper->GetId();
    IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subShapeIds.IsEmpty()) return false;

    for (IVtk_ShapeIdList::Iterator it(subShapeIds); it.More(); it.Next()) {
        const TopoDS_Shape& sub = shapeWrapper->GetSubShape(it.Value());
        if (sub.ShapeType() != TopAbs_FACE) continue;
        TopoDS_Face face = TopoDS::Face(sub);

        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane) continue;
        gp_Pln pln = adapt.Plane();
        outPlane = pln;
        return true;
    }
    return false;
}

bool Widget::tryPickPlanarFaceUnderCursor(int x, int y, gp_Pln& outPlane, TopoDS_Face& outFace)
{
    if (!shapePicker || !renderer) return false;

    // 确保可见模型绑定 ShapeSource
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor && renderStateFor(historyList[i]).shapeDataSource) {
            const bool visible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            renderStateFor(historyList[i]).actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, renderStateFor(historyList[i]).actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).shapeDataSource) {
            renderStateFor(historyList[i]).shapeDataSource->Modified();
            renderStateFor(historyList[i]).shapeDataSource->Update();
        }
    }

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Face);
    shapePicker->Pick(x, y, 0);

    vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
    if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return false;

    pickedActors->InitTraversal();
    vtkActor* selectedActor = nullptr;
    while (vtkActor* a = pickedActors->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) {
            selectedActor = a;
            break;
        }
    }
    if (!selectedActor) return false;

    IVtkTools_ShapeDataSource* dataSource = IVtkTools_ShapeObject::GetShapeSource(selectedActor);
    if (!dataSource) return false;

    Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
    if (shapeWrapper.IsNull()) return false;

    const IVtk_IdType shapeID = shapeWrapper->GetId();
    IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subShapeIds.IsEmpty()) return false;

    for (IVtk_ShapeIdList::Iterator it(subShapeIds); it.More(); it.Next()) {
        const TopoDS_Shape& sub = shapeWrapper->GetSubShape(it.Value());
        if (sub.ShapeType() != TopAbs_FACE) continue;
        TopoDS_Face face = TopoDS::Face(sub);

        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane) continue;

        outPlane = adapt.Plane();
        outFace = face;
        return true;
    }
    return false;
}

void Widget::clearSketchPlaneHover()
{
    if (!renderer) return;
    if (sketchPlaneHoverActor_) {
        removeSceneActor(sketchPlaneHoverActor_);
        sketchPlaneHoverActor_ = nullptr;
    }
}

void Widget::updateSketchPlaneHover(int x, int y)
{
    if (currentSelectionMode != SketchPlaneSelection) return;
    if (!renderer || !vtkWidget) return;

    gp_Pln pln;
    TopoDS_Face face;
    if (!tryPickPlanarFaceUnderCursor(x, y, pln, face)) {
        clearSketchPlaneHover();
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }

    // 复用已有的“捕捉点高亮 actor 构建器”来高亮面
    vtkSmartPointer<vtkActor> hl = buildSnapShapeHighlightActor(face, 0.2, 0.7, 1.0, 0.35, 2.0);
    if (!hl) return;
    hl->SetPickable(false);

    // 替换旧悬浮高亮
    clearSketchPlaneHover();
    sketchPlaneHoverActor_ = hl;
    addAppearanceActor(sketchPlaneHoverActor_);
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::createOrUpdateSketchSelectedDatumPlane(const gp_Pln& pln, const TopoDS_Face& refFace)
{
    if (!renderer || !vtkWidget) return;

    // 记录旧 actor（来自成员变量与 history 记录）。某些场景下它们可能不同，
    // 导致只 RemoveActor 成员变量不足以彻底清除旧轮廓/填充。
    vtkActor* oldFillActor = sketchSelectedPlaneFillActor_.GetPointer();
    vtkActor* oldOutlineActor = sketchSelectedPlaneOutlineActor_.GetPointer();
    vtkActor* oldFillActorHist = nullptr;
    vtkActor* oldOutlineActorHist = nullptr;
    bool hasOldDatumHist = (sketchSelectedPlaneHistoryIndex_ >= 0 &&
                             sketchSelectedPlaneHistoryIndex_ < historyList.size());
    bool oldDatumVisible = true;
    if (hasOldDatumHist) {
        oldFillActorHist = renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor;
        oldOutlineActorHist = renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).outlineActor.GetPointer();
        if (renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor) {
            oldDatumVisible = (renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor->GetVisibility() != 0);
        }
    }

    // 计算 refFace 在平面局部坐标系下的 u/v 范围，用于生成略大一点的显示平面
    Bnd_Box box;
    BRepBndLib::Add(refFace, box);
    if (box.IsVoid()) return;

    Standard_Real xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    const gp_Ax3 ax = pln.Position();
    const gp_Pnt o = ax.Location();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();

    auto projUV = [&](const gp_Pnt& p, double& u, double& v) {
        gp_Vec op(o, p);
        u = op.Dot(gp_Vec(xd));
        v = op.Dot(gp_Vec(yd));
    };

    double umin = 1e100, umax = -1e100, vmin = 1e100, vmax = -1e100;
    for (int xi = 0; xi < 2; ++xi) {
        for (int yi = 0; yi < 2; ++yi) {
            for (int zi = 0; zi < 2; ++zi) {
                const gp_Pnt p(xi ? xmax : xmin, yi ? ymax : ymin, zi ? zmax : zmin);
                double u = 0, v = 0;
                projUV(p, u, v);
                umin = std::min(umin, u); umax = std::max(umax, u);
                vmin = std::min(vmin, v); vmax = std::max(vmax, v);
            }
        }
    }

    const double du = std::max(1.0, umax - umin);
    const double dv = std::max(1.0, vmax - vmin);
    const double scale = 1.15; // 略大一点

    const double hu = du * 0.5 * scale;
    const double hv = dv * 0.5 * scale;

    const gp_Pnt p00(o.X() - xd.X() * hu - yd.X() * hv,
                    o.Y() - xd.Y() * hu - yd.Y() * hv,
                    o.Z() - xd.Z() * hu - yd.Z() * hv);
    const gp_Pnt p10(o.X() + xd.X() * hu - yd.X() * hv,
                    o.Y() + xd.Y() * hu - yd.Y() * hv,
                    o.Z() + xd.Z() * hu - yd.Z() * hv);
    const gp_Pnt p01(o.X() - xd.X() * hu + yd.X() * hv,
                    o.Y() - xd.Y() * hu + yd.Y() * hv,
                    o.Z() - xd.Z() * hu + yd.Z() * hv);

    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetOrigin(p00.X(), p00.Y(), p00.Z());
    plane->SetPoint1(p10.X(), p10.Y(), p10.Z());
    plane->SetPoint2(p01.X(), p01.Y(), p01.Z());
    plane->SetResolution(1, 1);
    plane->Update();

    vtkPolyData* planePd = plane->GetOutput();
    if (!planePd) return;

    // 1) 填充半透明
    vtkSmartPointer<vtkPolyDataMapper> fillMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    fillMapper->SetInputData(planePd);
    fillMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> fillActor = vtkSmartPointer<vtkActor>::New();
    fillActor->SetMapper(fillMapper);
    fillActor->GetProperty()->SetColor(0.2, 0.8, 1.0);
    fillActor->GetProperty()->SetOpacity(0.25);
    fillActor->GetProperty()->SetLighting(false);
    fillActor->SetPickable(false);

    // 2) 轮廓实线不透明
    vtkSmartPointer<vtkFeatureEdges> fe = vtkSmartPointer<vtkFeatureEdges>::New();
    fe->SetInputData(planePd);
    fe->BoundaryEdgesOn();
    fe->FeatureEdgesOff();
    fe->ManifoldEdgesOff();
    fe->NonManifoldEdgesOff();
    fe->Update();

    vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    outlineMapper->SetInputConnection(fe->GetOutputPort());
    outlineMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
    outlineActor->SetMapper(outlineMapper);
    outlineActor->GetProperty()->SetColor(0.2, 0.8, 1.0);
    outlineActor->GetProperty()->SetOpacity(1.0);
    outlineActor->GetProperty()->SetLineWidth(2.5);
    outlineActor->GetProperty()->SetLighting(false);
    outlineActor->SetPickable(false);

    // 替换旧显示（成员变量 + history 均尝试移除，避免旧轮廓残留）
    auto removeIf = [&](vtkActor* a) {
        if (a) removeSceneActor(a);
    };
    removeIf(oldFillActor);
    removeIf(oldOutlineActor);
    removeIf(oldFillActorHist);
    removeIf(oldOutlineActorHist);

    // 保持历史可见性状态一致（避免“只剩轮廓/填充缺失”）
    fillActor->SetVisibility(oldDatumVisible ? 1 : 0);
    outlineActor->SetVisibility(oldDatumVisible ? 1 : 0);

    sketchSelectedPlaneFillActor_ = fillActor;
    sketchSelectedPlaneOutlineActor_ = outlineActor;
    addAppearanceActor(sketchSelectedPlaneFillActor_);
    addAppearanceActor(sketchSelectedPlaneOutlineActor_);

    // 作为“自动创建的基准平面”写入历史（便于特征树控制可见性）
    if (sketchSelectedPlaneHistoryIndex_ < 0 || sketchSelectedPlaneHistoryIndex_ >= historyList.size()) {
        vtkSmartPointer<vtkPolyData> polyCopy = vtkSmartPointer<vtkPolyData>::New();
        polyCopy->ShallowCopy(planePd);
        addToHistory(DATUM_PLANE, tr("基准平面(草图)"), fillActor, QColor(60, 200, 255),
                     0, 0, 0, polyCopy, TopoDS_Shape(), nullptr, nullptr);
        sketchSelectedPlaneHistoryIndex_ = historyList.size() - 1;
        renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).outlineActor = outlineActor;
        // 确保轮廓可见性与填充一致（避免树更新时 outline 还未挂到 record）
        if (renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor) {
            outlineActor->SetVisibility(renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor->GetVisibility());
        }
        renderStateFor(historyList[sketchSelectedPlaneHistoryIndex_]).actor->SetPickable(false);
    } else {
        // 更新已有历史项
        ModelingHistory& rec = historyList[sketchSelectedPlaneHistoryIndex_];
        rec.type = DATUM_PLANE;
        rec.name = tr("基准平面(草图)");
        renderStateFor(rec).actor = fillActor;
        renderStateFor(rec).outlineActor = outlineActor;
        vtkSmartPointer<vtkPolyData> polyCopy = vtkSmartPointer<vtkPolyData>::New();
        polyCopy->ShallowCopy(planePd);
        renderStateFor(rec).polyData = polyCopy;
        rec.color = QColor(60, 200, 255);
        renderStateFor(rec).actor->SetPickable(false);
        updateFeatureTree();
    }

    vtkWidget->renderWindow()->Render();
}

void Widget::ensureSketchPreviewLineActor()
{
    if (sketchPreviewLineActor_ && sketchPreviewLineSource_) return;
    if (!renderer) return;

    sketchPreviewLineSource_ = vtkSmartPointer<vtkLineSource>::New();
    sketchPreviewLineSource_->SetPoint1(0, 0, 0);
    sketchPreviewLineSource_->SetPoint2(0, 0, 0);
    sketchPreviewLineSource_->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sketchPreviewLineSource_->GetOutputPort());
    mapper->ScalarVisibilityOff();

    sketchPreviewLineActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewLineActor_->SetMapper(mapper);
    sketchPreviewLineActor_->GetProperty()->SetColor(1.0, 1.0, 0.0);
    sketchPreviewLineActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewLineActor_->GetProperty()->SetOpacity(0.9);
    sketchPreviewLineActor_->GetProperty()->SetLighting(false);
    sketchPreviewLineActor_->SetPickable(false);
    addAppearanceActor(sketchPreviewLineActor_);
}

void Widget::updateSketchPreviewLine(const gp_Pnt& p1, const gp_Pnt& p2)
{
    ensureSketchPreviewLineActor();
    if (!sketchPreviewLineSource_ || !sketchPreviewLineActor_) return;
    sketchPreviewLineSource_->SetPoint1(p1.X(), p1.Y(), p1.Z());
    sketchPreviewLineSource_->SetPoint2(p2.X(), p2.Y(), p2.Z());
    sketchPreviewLineSource_->Update();
    sketchPreviewLineActor_->SetVisibility(true);
}

void Widget::clearSketchPreviewLine()
{
    if (sketchPreviewLineActor_) sketchPreviewLineActor_->SetVisibility(false);
}

void Widget::ensureSketchPreviewRectangleActor()
{
    if (sketchPreviewRectangleActor_ && sketchPreviewRectanglePolyData_) return;
    if (!renderer) return;

    sketchPreviewRectanglePolyData_ = vtkSmartPointer<vtkPolyData>::New();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(sketchPreviewRectanglePolyData_);
    mapper->ScalarVisibilityOff();

    sketchPreviewRectangleActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewRectangleActor_->SetMapper(mapper);
    sketchPreviewRectangleActor_->GetProperty()->SetColor(1.0, 1.0, 0.0);
    sketchPreviewRectangleActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewRectangleActor_->GetProperty()->SetOpacity(0.9);
    sketchPreviewRectangleActor_->GetProperty()->SetLighting(false);
    sketchPreviewRectangleActor_->SetPickable(false);
    sketchPreviewRectangleActor_->SetVisibility(false);
    addAppearanceActor(sketchPreviewRectangleActor_);
}

void Widget::updateSketchPreviewRectangle(const gp_Pnt& p1, const gp_Pnt& p3)
{
    ensureSketchPreviewRectangleActor();
    if (!sketchPreviewRectangleActor_ || !sketchPreviewRectanglePolyData_) return;

    const gp_Ax3 ax = activeSketchPlane_.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    const gp_Vec diag(p1, p3);
    const double du = diag.Dot(gp_Vec(xd));
    const double dv = diag.Dot(gp_Vec(yd));

    const gp_Pnt p2 = p1.Translated(du * gp_Vec(xd));
    const gp_Pnt p4 = p1.Translated(dv * gp_Vec(yd));

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->InsertNextPoint(p1.X(), p1.Y(), p1.Z());
    pts->InsertNextPoint(p2.X(), p2.Y(), p2.Z());
    pts->InsertNextPoint(p3.X(), p3.Y(), p3.Z());
    pts->InsertNextPoint(p4.X(), p4.Y(), p4.Z());

    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    auto addEdge = [&](vtkIdType a, vtkIdType b) {
        lines->InsertNextCell(2);
        lines->InsertCellPoint(a);
        lines->InsertCellPoint(b);
    };
    addEdge(0, 1);
    addEdge(1, 2);
    addEdge(2, 3);
    addEdge(3, 0);

    sketchPreviewRectanglePolyData_->SetPoints(pts);
    sketchPreviewRectanglePolyData_->SetLines(lines);
    sketchPreviewRectanglePolyData_->Modified();
    sketchPreviewRectangleActor_->SetVisibility(true);
}

void Widget::clearSketchPreviewRectangle()
{
    if (sketchPreviewRectangleActor_) sketchPreviewRectangleActor_->SetVisibility(false);
}

void Widget::updateSketchPreviewRectangleGeneral(const gp_Pnt& a, const gp_Pnt& b, const gp_Pnt& c, const gp_Pnt& d)
{
    ensureSketchPreviewRectangleActor();
    if (!sketchPreviewRectangleActor_ || !sketchPreviewRectanglePolyData_) return;

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->InsertNextPoint(a.X(), a.Y(), a.Z());
    pts->InsertNextPoint(b.X(), b.Y(), b.Z());
    pts->InsertNextPoint(c.X(), c.Y(), c.Z());
    pts->InsertNextPoint(d.X(), d.Y(), d.Z());

    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    auto addEdge = [&](vtkIdType x, vtkIdType y) {
        lines->InsertNextCell(2);
        lines->InsertCellPoint(x);
        lines->InsertCellPoint(y);
    };
    addEdge(0, 1);
    addEdge(1, 2);
    addEdge(2, 3);
    addEdge(3, 0);

    sketchPreviewRectanglePolyData_->SetPoints(pts);
    sketchPreviewRectanglePolyData_->SetLines(lines);
    sketchPreviewRectanglePolyData_->Modified();
    sketchPreviewRectangleActor_->SetVisibility(true);
}

void Widget::ensureSketchPreviewCircleActor()
{
    if (sketchPreviewCircleActor_ && sketchPreviewCirclePolyData_) return;
    if (!renderer) return;

    sketchPreviewCirclePolyData_ = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(sketchPreviewCirclePolyData_);
    mapper->ScalarVisibilityOff();

    sketchPreviewCircleActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewCircleActor_->SetMapper(mapper);
    sketchPreviewCircleActor_->GetProperty()->SetColor(1.0, 1.0, 0.0);
    sketchPreviewCircleActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewCircleActor_->GetProperty()->SetOpacity(0.9);
    sketchPreviewCircleActor_->GetProperty()->SetLighting(false);
    sketchPreviewCircleActor_->SetPickable(false);
    sketchPreviewCircleActor_->SetVisibility(false);
    addAppearanceActor(sketchPreviewCircleActor_);
}

void Widget::updateSketchPreviewCircle(const gp_Pnt& center, double radius)
{
    ensureSketchPreviewCircleActor();
    if (!sketchPreviewCircleActor_ || !sketchPreviewCirclePolyData_) return;
    if (radius <= Precision::Confusion()) {
        sketchPreviewCircleActor_->SetVisibility(false);
        return;
    }

    const gp_Ax3 ax = activeSketchPlane_.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();

    const int N = 72;
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(N);
    for (int i = 0; i < N; ++i) {
        const double ang = (2.0 * M_PI * static_cast<double>(i)) / static_cast<double>(N);
        const double cs = std::cos(ang);
        const double sn = std::sin(ang);
        gp_Vec ux(xd);
        gp_Vec uy(yd);
        gp_Pnt p = center.Translated(radius * cs * ux + radius * sn * uy);
        pts->SetPoint(i, p.X(), p.Y(), p.Z());
    }

    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(N + 1);
    for (int i = 0; i < N; ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }
    polyLine->GetPointIds()->SetId(N, 0);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);

    sketchPreviewCirclePolyData_->SetPoints(pts);
    sketchPreviewCirclePolyData_->SetLines(cells);
    sketchPreviewCirclePolyData_->Modified();
    sketchPreviewCircleActor_->SetVisibility(true);
}

void Widget::clearSketchPreviewCircle()
{
    if (sketchPreviewCircleActor_) sketchPreviewCircleActor_->SetVisibility(false);
}

void Widget::ensureSketchPreviewPolygonActor()
{
    if (sketchPreviewPolygonActor_ && sketchPreviewPolygonPolyData_) return;
    if (!renderer) return;

    sketchPreviewPolygonPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(sketchPreviewPolygonPolyData_);
    mapper->ScalarVisibilityOff();

    sketchPreviewPolygonActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewPolygonActor_->SetMapper(mapper);
    sketchPreviewPolygonActor_->GetProperty()->SetColor(1.0, 0.55, 0.0);
    sketchPreviewPolygonActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewPolygonActor_->GetProperty()->SetOpacity(0.95);
    sketchPreviewPolygonActor_->GetProperty()->SetLighting(false);
    sketchPreviewPolygonActor_->SetPickable(false);
    sketchPreviewPolygonActor_->SetVisibility(false);
    addAppearanceActor(sketchPreviewPolygonActor_);
}

void Widget::updateSketchPreviewPolygon(const QList<gp_Pnt>& verts)
{
    ensureSketchPreviewPolygonActor();
    if (!sketchPreviewPolygonActor_ || !sketchPreviewPolygonPolyData_) return;
    if (verts.size() < 3) {
        sketchPreviewPolygonActor_->SetVisibility(false);
        return;
    }

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(verts.size());
    for (int i = 0; i < verts.size(); ++i) {
        const gp_Pnt& p = verts[i];
        pts->SetPoint(i, p.X(), p.Y(), p.Z());
    }

    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(verts.size() + 1);
    for (int i = 0; i < verts.size(); ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }
    polyLine->GetPointIds()->SetId(verts.size(), 0);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);

    sketchPreviewPolygonPolyData_->SetPoints(pts);
    sketchPreviewPolygonPolyData_->SetLines(cells);
    sketchPreviewPolygonPolyData_->Modified();
    sketchPreviewPolygonActor_->SetVisibility(true);
}

void Widget::clearSketchPreviewPolygon()
{
    if (sketchPreviewPolygonActor_) sketchPreviewPolygonActor_->SetVisibility(false);
}

void Widget::updateSketchPreviewPolygonGuideLine(const gp_Pnt& center, const gp_Pnt& guideEnd, bool dashed)
{
    ensureSketchPreviewLineActor();
    if (!sketchPreviewLineSource_ || !sketchPreviewLineActor_) return;
    sketchPreviewLineSource_->SetPoint1(center.X(), center.Y(), center.Z());
    sketchPreviewLineSource_->SetPoint2(guideEnd.X(), guideEnd.Y(), guideEnd.Z());
    sketchPreviewLineSource_->Update();
    sketchPreviewLineActor_->GetProperty()->SetColor(1.0, 0.55, 0.0);
    sketchPreviewLineActor_->GetProperty()->SetLineWidth(2.0);
    if (dashed) {
        sketchPreviewLineActor_->GetProperty()->SetLineStipplePattern(0xF0F0);
        sketchPreviewLineActor_->GetProperty()->SetLineStippleRepeatFactor(1);
    } else {
        sketchPreviewLineActor_->GetProperty()->SetLineStipplePattern(0xFFFF);
        sketchPreviewLineActor_->GetProperty()->SetLineStippleRepeatFactor(1);
    }
    sketchPreviewPolygonGuideDashed_ = dashed;
    sketchPreviewLineActor_->SetVisibility(true);
}

void Widget::ensureSketchPreviewArcActor()
{
    if (sketchPreviewArcActor_ && sketchPreviewArcPolyData_) return;
    if (!renderer) return;

    sketchPreviewArcPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(sketchPreviewArcPolyData_);
    mapper->ScalarVisibilityOff();

    sketchPreviewArcActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewArcActor_->SetMapper(mapper);
    sketchPreviewArcActor_->GetProperty()->SetColor(1.0, 1.0, 0.0);
    sketchPreviewArcActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewArcActor_->GetProperty()->SetOpacity(0.9);
    sketchPreviewArcActor_->GetProperty()->SetLighting(false);
    sketchPreviewArcActor_->SetPickable(false);
    sketchPreviewArcActor_->SetVisibility(false);
    addAppearanceActor(sketchPreviewArcActor_);
}

void Widget::updateSketchPreviewArc(const gp_Pnt& p1, const gp_Pnt& pm, const gp_Pnt& p3)
{
    ensureSketchPreviewArcActor();
    if (!sketchPreviewArcActor_ || !sketchPreviewArcPolyData_) return;

    try {
        GC_MakeArcOfCircle mk(p1, pm, p3);
        if (!mk.IsDone()) {
            if (sketchPreviewArcActor_) sketchPreviewArcActor_->SetVisibility(false);
            return;
        }
        Handle(Geom_TrimmedCurve) arc = mk.Value();
        if (arc.IsNull()) {
            if (sketchPreviewArcActor_) sketchPreviewArcActor_->SetVisibility(false);
            return;
        }
        const Standard_Real f = arc->FirstParameter();
        const Standard_Real l = arc->LastParameter();

        const int N = 64;
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        pts->SetNumberOfPoints(N);
        for (int i = 0; i < N; ++i) {
            const double t = f + (l - f) * (static_cast<double>(i) / static_cast<double>(N - 1));
            const gp_Pnt p = arc->Value(t);
            pts->SetPoint(i, p.X(), p.Y(), p.Z());
        }

        vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
        polyLine->GetPointIds()->SetNumberOfIds(N);
        for (int i = 0; i < N; ++i) {
            polyLine->GetPointIds()->SetId(i, i);
        }

        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        cells->InsertNextCell(polyLine);

        sketchPreviewArcPolyData_->SetPoints(pts);
        sketchPreviewArcPolyData_->SetLines(cells);
        sketchPreviewArcPolyData_->Modified();

        sketchPreviewArcActor_->SetVisibility(true);
    } catch (...) {
        sketchPreviewArcActor_->SetVisibility(false);
    }
}

void Widget::clearSketchPreviewArc()
{
    if (sketchPreviewArcActor_) sketchPreviewArcActor_->SetVisibility(false);
}

void Widget::ensureSketchPreviewConicActor()
{
    if (sketchPreviewConicActor_ && sketchPreviewConicPolyData_) return;
    if (!renderer) return;

    sketchPreviewConicPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(sketchPreviewConicPolyData_);
    mapper->ScalarVisibilityOff();

    sketchPreviewConicActor_ = vtkSmartPointer<vtkActor>::New();
    sketchPreviewConicActor_->SetMapper(mapper);
    sketchPreviewConicActor_->GetProperty()->SetColor(1.0, 1.0, 0.0);
    sketchPreviewConicActor_->GetProperty()->SetLineWidth(2.0);
    sketchPreviewConicActor_->GetProperty()->SetOpacity(0.9);
    sketchPreviewConicActor_->GetProperty()->SetLighting(false);
    sketchPreviewConicActor_->SetPickable(false);
    sketchPreviewConicActor_->SetVisibility(false);
    addAppearanceActor(sketchPreviewConicActor_);
}

void Widget::updateSketchPreviewConicBezier(const gp_Pnt& pole0, const gp_Pnt& pole1, const gp_Pnt& pole2)
{
    ensureSketchPreviewConicActor();
    if (!sketchPreviewConicActor_ || !sketchPreviewConicPolyData_) return;

    const int N = 96;
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(N);
    for (int i = 0; i < N; ++i) {
        const double u = static_cast<double>(i) / static_cast<double>(N - 1);
        const double b0 = (1.0 - u) * (1.0 - u);
        const double b1 = 2.0 * u * (1.0 - u);
        const double b2 = u * u;
        const gp_Pnt p(pole0.X() * b0 + pole1.X() * b1 + pole2.X() * b2,
                       pole0.Y() * b0 + pole1.Y() * b1 + pole2.Y() * b2,
                       pole0.Z() * b0 + pole1.Z() * b1 + pole2.Z() * b2);
        pts->SetPoint(i, p.X(), p.Y(), p.Z());
    }
    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(N);
    for (int i = 0; i < N; ++i)
        polyLine->GetPointIds()->SetId(i, i);
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);
    sketchPreviewConicPolyData_->SetPoints(pts);
    sketchPreviewConicPolyData_->SetLines(cells);
    sketchPreviewConicPolyData_->Modified();
    sketchPreviewConicActor_->SetVisibility(true);
}

void Widget::clearSketchPreviewConic()
{
    if (sketchPreviewConicActor_) sketchPreviewConicActor_->SetVisibility(false);
}

void Widget::ensureSketchConicMarkerActors()
{
    if (!renderer) return;
    static const float kRgb[3][3] = {
        {0.15f, 0.95f, 0.25f},
        {0.25f, 0.55f, 1.0f},
        {1.0f, 0.72f, 0.12f},
    };
    for (int i = 0; i < 3; ++i) {
        if (sketchConicMarkerActors_[i])
            continue;
        sketchConicMarkerSpheres_[i] = vtkSmartPointer<vtkSphereSource>::New();
        sketchConicMarkerSpheres_[i]->SetRadius(0.09);
        sketchConicMarkerSpheres_[i]->SetPhiResolution(24);
        sketchConicMarkerSpheres_[i]->SetThetaResolution(24);
        sketchConicMarkerSpheres_[i]->SetCenter(0.0, 0.0, 0.0);
        sketchConicMarkerSpheres_[i]->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sketchConicMarkerSpheres_[i]->GetOutputPort());
        mapper->ScalarVisibilityOff();

        sketchConicMarkerActors_[i] = vtkSmartPointer<vtkActor>::New();
        sketchConicMarkerActors_[i]->SetMapper(mapper);
        sketchConicMarkerActors_[i]->GetProperty()->SetColor(kRgb[i][0], kRgb[i][1], kRgb[i][2]);
        sketchConicMarkerActors_[i]->GetProperty()->SetOpacity(1.0);
        sketchConicMarkerActors_[i]->GetProperty()->SetLighting(false);
        sketchConicMarkerActors_[i]->SetPickable(false);
        sketchConicMarkerActors_[i]->SetVisibility(false);
        addReferenceActor(sketchConicMarkerActors_[i]);
    }
}

void Widget::updateSketchConicMarkers()
{
    ensureSketchConicMarkerActors();
    if (!renderer) return;

    if (!sketchConicDialog_ || !hasActiveSketch_) {
        for (int i = 0; i < 3; ++i) {
            if (sketchConicMarkerActors_[i])
                sketchConicMarkerActors_[i]->SetVisibility(false);
        }
        return;
    }

    double chord = 0.0;
    if (sketchConicHasP0_ && sketchConicHasP1_)
        chord = gp_Vec(sketchConicP0_, sketchConicP1_).Magnitude();
    const double radius = std::clamp(chord > Precision::Confusion() ? chord * 0.02 : 0.09, 0.045, 0.14);

    const gp_Pnt pts[3] = {sketchConicP0_, sketchConicP1_, sketchConicPc_};
    const bool has[3] = {sketchConicHasP0_, sketchConicHasP1_, sketchConicHasPc_};

    for (int i = 0; i < 3; ++i) {
        if (!sketchConicMarkerActors_[i] || !sketchConicMarkerSpheres_[i])
            continue;
        if (!has[i]) {
            sketchConicMarkerActors_[i]->SetVisibility(false);
            continue;
        }
        sketchConicMarkerSpheres_[i]->SetRadius(radius);
        sketchConicMarkerSpheres_[i]->SetCenter(pts[i].X(), pts[i].Y(), pts[i].Z());
        sketchConicMarkerSpheres_[i]->Update();
        sketchConicMarkerActors_[i]->SetVisibility(true);
    }
}

void Widget::clearSketchConicMarkers()
{
    for (int i = 0; i < 3; ++i) {
        if (sketchConicMarkerActors_[i])
            sketchConicMarkerActors_[i]->SetVisibility(false);
    }
}

gp_Pnt Widget::sketchConicRhoAdjustedShoulder(const gp_Pnt& p0, const gp_Pnt& pEnd, const gp_Pnt& shoulderUser,
                                              double rho) const
{
    gp_Vec chord(p0, pEnd);
    const double len = chord.Magnitude();
    if (len <= Precision::Confusion())
        return shoulderUser;
    gp_Dir du(chord);
    gp_Dir n = activeSketchPlane_.Axis().Direction().Crossed(du);
    gp_Vec perp(n);
    if (perp.Magnitude() <= Precision::Confusion())
        return shoulderUser;
    perp.Normalize();
    const double bump = (rho - 0.5) * len * 0.25;
    return shoulderUser.Translated(bump * perp);
}

bool Widget::tryMakeSketchConicBezierEdge(const gp_Pnt& p0, const gp_Pnt& pEnd, const gp_Pnt& shoulderUser, double rho,
                                          TopoDS_Edge& outEdge) const
{
    outEdge.Nullify();
    const gp_Pnt S = sketchConicRhoAdjustedShoulder(p0, pEnd, shoulderUser, rho);
    const gp_Pnt mid(0.5 * (4.0 * S.X() - p0.X() - pEnd.X()),
                     0.5 * (4.0 * S.Y() - p0.Y() - pEnd.Y()),
                     0.5 * (4.0 * S.Z() - p0.Z() - pEnd.Z()));
    try {
        TColgp_Array1OfPnt poles(1, 3);
        poles.SetValue(1, p0);
        poles.SetValue(2, mid);
        poles.SetValue(3, pEnd);
        Handle(Geom_BezierCurve) bc = new Geom_BezierCurve(poles);
        outEdge = BRepBuilderAPI_MakeEdge(bc).Edge();
        return !outEdge.IsNull();
    } catch (...) {
        return false;
    }
}
