// Sketch: plane pick, preview, history, trim/extend (split from widget.cpp)
#include "widget.h"
#include "ui_widget.h"
#include "command.h"
#include "sketchcreatedialog.h"
#include "sketchmodedialogs.h"
#include "sketchtoolinputdialog.h"
#include "sketchconicdialog.h"
#include "sketchpolygondialog.h"
#include "sketchellipsedialog.h"

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
#include <GeomAPI_ProjectPointOnCurve.hxx>
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

namespace {

/** 将系统标准图标转为近似单色（使用调色板 ButtonText），便于侧栏小按钮与 NX 风格接近。 */
static QIcon monochromeStandardIcon(const QWidget* w, QStyle::StandardPixmap sp, const QSize& sz)
{
    if (!w || !w->style())
        return {};
    const QIcon src = w->style()->standardIcon(sp, nullptr, w);
    const QPixmap pm = src.pixmap(sz, QIcon::Normal, QIcon::Off);
    if (pm.isNull())
        return {};
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    const QColor ink = w->palette().color(QPalette::ButtonText);
    for (int y = 0; y < img.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb px = line[x];
            const int a0 = qAlpha(px);
            if (a0 == 0)
                continue;
            const int lum = qGray(px);
            const int a = qBound(30, (a0 * lum + 64) / 256 + 48, 255);
            line[x] = qRgba(ink.red(), ink.green(), ink.blue(), a);
        }
    }
    return QIcon(QPixmap::fromImage(img));
}

void cycleSketchStackedPage(QStackedWidget* sw, int delta)
{
    if (!sw)
        return;
    const int n = sw->count();
    if (n <= 0)
        return;
    sw->setCurrentIndex((sw->currentIndex() + delta + n) % n);
}

} // namespace

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
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
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
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
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
        oldFillActorHist = historyList[sketchSelectedPlaneHistoryIndex_].actor;
        oldOutlineActorHist = historyList[sketchSelectedPlaneHistoryIndex_].outlineActor.GetPointer();
        if (historyList[sketchSelectedPlaneHistoryIndex_].actor) {
            oldDatumVisible = (historyList[sketchSelectedPlaneHistoryIndex_].actor->GetVisibility() != 0);
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
        historyList[sketchSelectedPlaneHistoryIndex_].outlineActor = outlineActor;
        // 确保轮廓可见性与填充一致（避免树更新时 outline 还未挂到 record）
        if (historyList[sketchSelectedPlaneHistoryIndex_].actor) {
            outlineActor->SetVisibility(historyList[sketchSelectedPlaneHistoryIndex_].actor->GetVisibility());
        }
        historyList[sketchSelectedPlaneHistoryIndex_].actor->SetPickable(false);
    } else {
        // 更新已有历史项
        ModelingHistory& rec = historyList[sketchSelectedPlaneHistoryIndex_];
        rec.type = DATUM_PLANE;
        rec.name = tr("基准平面(草图)");
        rec.actor = fillActor;
        rec.outlineActor = outlineActor;
        vtkSmartPointer<vtkPolyData> polyCopy = vtkSmartPointer<vtkPolyData>::New();
        polyCopy->ShallowCopy(planePd);
        rec.polyData = polyCopy;
        rec.color = QColor(60, 200, 255);
        rec.actor->SetPickable(false);
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

void Widget::armSnapFiltersForSketchToolFromMenuKind(int snapKind)
{
    clearSnapSettings();
    if (!ui) return;
    snap_.enabled = true;
    snap_.armed = true;
    if (ui->Use_Capture)
        ui->Use_Capture->setChecked(true);

    snap_.nearest = (snapKind == 0);
    snap_.endpoint = (snapKind == 1);
    snap_.midpoint = (snapKind == 2);
    snap_.intersection = (snapKind == 3);
    snap_.center = (snapKind == 4);
    snap_.quadrant = (snapKind == 5);
    if (snapKind == 0) {
        snap_.endpoint = true;
        snap_.midpoint = true;
        snap_.intersection = false;
        snap_.center = true;
        snap_.quadrant = true;
    }
    ui->Capture_Closed->setChecked(snap_.nearest);
    ui->Capture_Endpoint->setChecked(snap_.endpoint);
    ui->Capture_Midpoint->setChecked(snap_.midpoint);
    ui->Capture_Insertsectionpoint->setChecked(snap_.intersection);
    ui->Capture_Arccenterpoint->setChecked(snap_.center);
    ui->Capture_Quadrantpoint->setChecked(snap_.quadrant);
    syncTabPointSnapToolbarsFromCaptureRow();
    mergeSnapFiltersFromToolbarAndCaptureUi();
}

void Widget::restoreSnapAfterSketchConicPick()
{
    clearSnapSettings();
}

void Widget::syncSketchConicDialogOkState()
{
    if (!sketchConicDialog_) return;
    sketchConicDialog_->setOkApplyEnabled(sketchConicHasP0_ && sketchConicHasP1_ && sketchConicHasPc_);
}

void Widget::clearSketchConicInternalState(bool clearDialogFields)
{
    sketchConicPendingField_ = -1;
    sketchConicHasP0_ = false;
    sketchConicHasP1_ = false;
    sketchConicHasPc_ = false;
    sketchConicCommittedGeomIndex_ = -1;
    sketchConicDragActive_ = false;
    if (clearDialogFields && sketchConicDialog_) {
        sketchConicDialog_->setStartText(QString());
        sketchConicDialog_->setEndText(QString());
        sketchConicDialog_->setControlText(QString());
        sketchConicDialog_->highlightPickField(-1);
    }
    syncSketchConicDialogOkState();
    clearSketchPreviewConic();
    clearSketchConicMarkers();
}

void Widget::rebuildSketchConicPreview()
{
    if (!sketchConicDialog_ || !hasActiveSketch_) {
        clearSketchPreviewConic();
        updateSketchConicMarkers();
        if (vtkWidget && vtkWidget->renderWindow())
            vtkWidget->renderWindow()->Render();
        return;
    }
    if (!sketchConicDialog_->previewEnabled()) {
        clearSketchPreviewConic();
        updateSketchConicMarkers();
        if (vtkWidget && vtkWidget->renderWindow())
            vtkWidget->renderWindow()->Render();
        return;
    }
    const double rho = sketchConicDialog_->rho();
    if (sketchConicHasP0_ && sketchConicHasP1_ && sketchConicHasPc_) {
        TopoDS_Edge e;
        if (tryMakeSketchConicBezierEdge(sketchConicP0_, sketchConicP1_, sketchConicPc_, rho, e)) {
            const gp_Pnt S = sketchConicRhoAdjustedShoulder(sketchConicP0_, sketchConicP1_, sketchConicPc_, rho);
            const gp_Pnt mid(0.5 * (4.0 * S.X() - sketchConicP0_.X() - sketchConicP1_.X()),
                             0.5 * (4.0 * S.Y() - sketchConicP0_.Y() - sketchConicP1_.Y()),
                             0.5 * (4.0 * S.Z() - sketchConicP0_.Z() - sketchConicP1_.Z()));
            clearSketchPreviewLine();
            updateSketchPreviewConicBezier(sketchConicP0_, mid, sketchConicP1_);
        } else {
            clearSketchPreviewConic();
        }
    } else if (sketchConicHasP0_ && sketchConicHasP1_) {
        clearSketchPreviewConic();
        updateSketchPreviewLine(sketchConicP0_, sketchConicP1_);
    } else {
        clearSketchPreviewConic();
        clearSketchPreviewLine();
    }
    updateSketchConicMarkers();
    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::beginSketchConicPick(int whichField)
{
    if (!sketchConicDialog_ || !hasActiveSketch_) return;
    sketchConicPendingField_ = whichField;
    int k = sketchConicDialog_->startSnapKind();
    if (whichField == 1)
        k = sketchConicDialog_->endSnapKind();
    else if (whichField == 2)
        k = sketchConicDialog_->controlSnapKind();
    armSnapFiltersForSketchToolFromMenuKind(k);
    currentSelectionMode = SketchConicPick;
    sketchConicDialog_->highlightPickField(whichField);
    statusBar()->showMessage(tr("二次曲线：在草图平面内点击以确定点（可配合捕捉）。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::completeSketchConicPick(const gp_Pnt& p)
{
    if (!sketchConicDialog_) {
        currentSelectionMode = None;
        return;
    }
    auto fmt = [](const gp_Pnt& q) {
        return QStringLiteral("(%1,%2,%3)")
            .arg(q.X(), 0, 'f', 3)
            .arg(q.Y(), 0, 'f', 3)
            .arg(q.Z(), 0, 'f', 3);
    };
    if (sketchConicPendingField_ == 0) {
        sketchConicP0_ = p;
        sketchConicHasP0_ = true;
        sketchConicDialog_->setStartText(fmt(p));
    } else if (sketchConicPendingField_ == 1) {
        sketchConicP1_ = p;
        sketchConicHasP1_ = true;
        sketchConicDialog_->setEndText(fmt(p));
    } else if (sketchConicPendingField_ == 2) {
        sketchConicPc_ = p;
        sketchConicHasPc_ = true;
        sketchConicDialog_->setControlText(fmt(p));
    }
    sketchConicPendingField_ = -1;
    restoreSnapAfterSketchConicPick();
    sketchConicDialog_->highlightPickField(-1);
    syncSketchConicDialogOkState();
    rebuildSketchConicPreview();
    if (sketchConicHasP0_ && sketchConicHasP1_ && sketchConicHasPc_)
        currentSelectionMode = SketchConicDragControl;
    else
        currentSelectionMode = None;
}

void Widget::commitSketchConicFromDialog()
{
    if (!hasActiveSketch_ || !sketchConicDialog_) return;
    const double rho = sketchConicDialog_->rho();
    TopoDS_Edge e;
    if (!tryMakeSketchConicBezierEdge(sketchConicP0_, sketchConicP1_, sketchConicPc_, rho, e)) {
        statusBar()->showMessage(tr("二次曲线生成失败（请调整三点或 Rho）。"), 4000);
        return;
    }
    if (sketchConicCommittedGeomIndex_ < 0) {
        activeSketch_.addGeometry(e);
        const QList<TopoDS_Shape> gg = activeSketch_.getGeometries();
        sketchConicCommittedGeomIndex_ = gg.size() - 1;
    } else {
        QList<TopoDS_Shape> gg = activeSketch_.getGeometries();
        if (sketchConicCommittedGeomIndex_ >= 0 && sketchConicCommittedGeomIndex_ < gg.size()) {
            gg[sketchConicCommittedGeomIndex_] = e;
            activeSketch_.setGeometries(gg);
        }
    }
    ensureSketchHistoryRecord();
    updateSketchHistoryShape();
    markDocumentModified(true);
    currentSelectionMode = SketchConicDragControl;
    clearSketchPreviewConic();
    clearSketchPreviewLine();
    updateSketchConicMarkers();
    statusBar()->showMessage(tr("二次曲线已写入草图，可拖动控制点调整形状。"), 4000);
    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::onSketchConicDialogRhoChanged(double v)
{
    Q_UNUSED(v);
    rebuildSketchConicPreview();
}

void Widget::openOrRaiseSketchConicDialog()
{
    if (!sketchConicDialog_) {
        sketchConicDialog_ = new SketchConicDialog(this);
        sketchConicDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchConicDialog_, &QObject::destroyed, this, [this]() { sketchConicDialog_ = nullptr; });
        connect(sketchConicDialog_, &QDialog::rejected, this, [this]() {
            clearSketchConicInternalState(true);
            if (currentSelectionMode == SketchConicPick || currentSelectionMode == SketchConicDragControl)
                currentSelectionMode = None;
            sketchConicPendingField_ = -1;
            sketchConicDragActive_ = false;
            clearSketchPreviewConic();
            clearSketchPreviewLine();
            clearSketchConicMarkers();
        });
        connect(sketchConicDialog_, &SketchConicDialog::pickStartRequested, this, [this]() { beginSketchConicPick(0); });
        connect(sketchConicDialog_, &SketchConicDialog::pickEndRequested, this, [this]() { beginSketchConicPick(1); });
        connect(sketchConicDialog_, &SketchConicDialog::pickControlRequested, this, [this]() { beginSketchConicPick(2); });
        connect(sketchConicDialog_, &SketchConicDialog::rhoValueChanged, this, &Widget::onSketchConicDialogRhoChanged);
        connect(sketchConicDialog_, &SketchConicDialog::previewToggled, this, [this](bool) { rebuildSketchConicPreview(); });
        connect(sketchConicDialog_, &SketchConicDialog::applyRequested, this, &Widget::commitSketchConicFromDialog);
    }
    sketchConicDialog_->show();
    sketchConicDialog_->raise();
    positionSketchAuxDialog(sketchConicDialog_);
}

void Widget::closeSketchConicDialog()
{
    if (sketchConicDialog_) {
        clearSketchConicInternalState(true);
        sketchConicDialog_->close();
        sketchConicDialog_ = nullptr;
    }
    if (currentSelectionMode == SketchConicPick || currentSelectionMode == SketchConicDragControl)
        currentSelectionMode = None;
    sketchConicPendingField_ = -1;
    sketchConicDragActive_ = false;
    clearSketchPreviewConic();
    clearSketchPreviewLine();
    clearSketchConicMarkers();
}

void Widget::handleSketchConicDragMouseDown(int x, int y)
{
    if (!hasActiveSketch_ || !sketchConicDialog_ || !sketchConicHasP0_ || !sketchConicHasP1_ || !sketchConicHasPc_)
        return;
    gp_Pnt p;
    if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) return;
    {
        const gp_Ax3 ax = activeSketchPlane_.Position();
        const gp_Pnt o = ax.Location();
        const gp_Dir xd = ax.XDirection();
        const gp_Dir yd = ax.YDirection();
        gp_Vec op(o, p);
        const double u = op.Dot(gp_Vec(xd));
        const double v = op.Dot(gp_Vec(yd));
        p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v, o.Y() + xd.Y() * u + yd.Y() * v, o.Z() + xd.Z() * u + yd.Z() * v);
    }
    const double chord = gp_Vec(sketchConicP0_, sketchConicP1_).Magnitude();
    const double markerR = std::clamp(chord > Precision::Confusion() ? chord * 0.02 : 0.09, 0.045, 0.14);
    const double tol = std::max(markerR * 2.2, std::max(1.0, chord * 0.12));
    if (gp_Vec(sketchConicPc_, p).Magnitude() <= tol)
        sketchConicDragActive_ = true;
}

void Widget::handleSketchConicDragMouseMove(int x, int y)
{
    if (!sketchConicDragActive_ || !hasActiveSketch_) return;
    gp_Pnt p;
    if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) return;
    {
        const gp_Ax3 ax = activeSketchPlane_.Position();
        const gp_Pnt o = ax.Location();
        const gp_Dir xd = ax.XDirection();
        const gp_Dir yd = ax.YDirection();
        gp_Vec op(o, p);
        const double u = op.Dot(gp_Vec(xd));
        const double v = op.Dot(gp_Vec(yd));
        p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v, o.Y() + xd.Y() * u + yd.Y() * v, o.Z() + xd.Z() * u + yd.Z() * v);
    }
    sketchConicPc_ = p;
    if (!sketchConicDialog_) return;
    const double rho = sketchConicDialog_->rho();
    TopoDS_Edge e;
    if (!tryMakeSketchConicBezierEdge(sketchConicP0_, sketchConicP1_, sketchConicPc_, rho, e)) return;

    if (sketchConicCommittedGeomIndex_ >= 0) {
        QList<TopoDS_Shape> gg = activeSketch_.getGeometries();
        if (sketchConicCommittedGeomIndex_ < gg.size()) {
            gg[sketchConicCommittedGeomIndex_] = e;
            activeSketch_.setGeometries(gg);
            updateSketchHistoryShape();
        }
    } else {
        rebuildSketchConicPreview();
    }

    if (sketchConicDialog_) {
        auto fmt = [](const gp_Pnt& q) {
            return QStringLiteral("(%1,%2,%3)")
                .arg(q.X(), 0, 'f', 3)
                .arg(q.Y(), 0, 'f', 3)
                .arg(q.Z(), 0, 'f', 3);
        };
        sketchConicDialog_->setControlText(fmt(sketchConicPc_));
    }
    updateSketchConicMarkers();
    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::handleSketchConicDragMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    if (sketchConicDragActive_) {
        sketchConicDragActive_ = false;
        ensureSketchHistoryRecord();
        markDocumentModified(true);
    }
}

void Widget::clearSketchCommittedOverlay()
{
    if (!renderer) return;
    for (const auto& a : sketchCommittedOverlayActors_) {
        if (a) removeSceneActor(a);
    }
    sketchCommittedOverlayActors_.clear();
}

void Widget::rebuildSketchCommittedOverlay()
{
    // 覆盖层已禁用：保持空实现，避免额外 actor 干扰草图交互拾取链路。
}

void Widget::appendCommittedSketchEdgeOverlay(const TopoDS_Edge& edge)
{
    Q_UNUSED(edge);
    // 覆盖层已禁用：保持空实现。
}

bool Widget::sketchWorldToUV(const gp_Pln& pln, const gp_Pnt& p, double& xc, double& yc) const
{
    const gp_Ax3 ax = pln.Position();
    const gp_Pnt o = ax.Location();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    gp_Vec op(o, p);
    xc = op.Dot(gp_Vec(xd));
    yc = op.Dot(gp_Vec(yd));
    return true;
}

gp_Pnt Widget::sketchUVToWorld(const gp_Pln& pln, double xc, double yc) const
{
    const gp_Ax3 ax = pln.Position();
    const gp_Pnt o = ax.Location();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    return gp_Pnt(o.X() + xd.X() * xc + yd.X() * yc,
                 o.Y() + xd.Y() * xc + yd.Y() * yc,
                 o.Z() + xd.Z() * xc + yd.Z() * yc);
}

void Widget::sketchLineLengthAngle(const gp_Pln& pln, const gp_Pnt& a, const gp_Pnt& b, double& len, double& angDeg) const
{
    gp_Vec v(a, b);
    len = v.Magnitude();
    if (len <= Precision::Confusion()) {
        angDeg = 0.0;
        return;
    }
    const gp_Ax3 ax = pln.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    const double vx = v.Dot(gp_Vec(xd));
    const double vy = v.Dot(gp_Vec(yd));
    double ang = std::atan2(vy, vx) * 180.0 / M_PI;
    if (ang < 0.0) ang += 360.0;
    angDeg = ang;
}

gp_Pnt Widget::sketchLineEndFromLengthAngle(const gp_Pln& pln, const gp_Pnt& start, double len, double angDeg) const
{
    if (len <= Precision::Confusion()) return start;
    const gp_Ax3 ax = pln.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    const double rad = angDeg * M_PI / 180.0;
    const double vx = std::cos(rad);
    const double vy = std::sin(rad);
    gp_Vec dir(xd.X() * vx + yd.X() * vy,
              xd.Y() * vx + yd.Y() * vy,
              xd.Z() * vx + yd.Z() * vy);
    if (dir.Magnitude() <= Precision::Confusion()) return start;
    dir.Normalize();
    return start.Translated(len * dir);
}

double Widget::sketchArcRadiusFromThreePoints(const gp_Pnt& p1, const gp_Pnt& pm, const gp_Pnt& p2) const
{
    try {
        Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(p1, pm, p2);
        Handle(Geom_Curve) bc = arc->BasisCurve();
        Handle(Geom_Circle) c = Handle(Geom_Circle)::DownCast(bc);
        if (c.IsNull() && !bc.IsNull() && bc->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
            Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(bc);
            if (!tc.IsNull()) {
                c = Handle(Geom_Circle)::DownCast(tc->BasisCurve());
            }
        }
        if (!c.IsNull()) return c->Radius();
    } catch (...) {
    }
    return 0.0;
}

bool Widget::sketchArcCenterFromTwoPointsRadius(const gp_Pln& pln, const gp_Pnt& p1, const gp_Pnt& p2, double R,
                                             const gp_Pnt& hint, gp_Pnt& outCenter) const
{
    gp_Vec chord(p1, p2);
    const double d = chord.Magnitude();
    if (d <= Precision::Confusion()) return false;
    if (R <= d * 0.5 - 1e-9) return false;

    const gp_Pnt M = gp_Pnt((p1.X() + p2.X()) * 0.5, (p1.Y() + p2.Y()) * 0.5, (p1.Z() + p2.Z()) * 0.5);
    gp_Dir chordDir(chord);
    gp_Dir n = pln.Axis().Direction();
    gp_Vec perp = chordDir.Crossed(n);
    if (perp.Magnitude() <= Precision::Confusion()) return false;
    perp.Normalize();

    const double half = d * 0.5;
    const double h = std::sqrt(std::max(0.0, R * R - half * half));
    gp_Vec toHint(M, hint);
    const double sign = (toHint.Dot(perp) >= 0.0) ? 1.0 : -1.0;
    outCenter = M.Translated(sign * h * perp);
    return true;
}

gp_Pnt Widget::sketchArcMidPointOnCircle(const gp_Pln& pln, const gp_Pnt& C, double R, const gp_Pnt& p1, const gp_Pnt& p2) const
{
    gp_Vec v1(C, p1);
    gp_Vec v2(C, p2);
    gp_Vec bis = v1 + v2;
    if (bis.Magnitude() < Precision::Confusion()) {
        gp_Vec chord(p1, p2);
        if (chord.Magnitude() < Precision::Confusion()) return p1;
        gp_Dir chordDir(chord);
        gp_Dir n = pln.Axis().Direction();
        gp_Vec perp = chordDir.Crossed(n);
        if (perp.Magnitude() < Precision::Confusion()) return p1;
        perp.Normalize();
        return C.Translated(R * perp);
    }
    gp_Dir d(bis);
    return C.Translated(R * gp_Vec(d));
}

bool Widget::sketchArcMidFromTangentAndEnd(const gp_Pln& pln, const gp_Pnt& S, const gp_Dir& tanAtS, const gp_Pnt& E,
                                           gp_Pnt& outMid) const
{
    gp_Vec se(S, E);
    const double sq = se.SquareMagnitude();
    if (sq <= Precision::Confusion()) return false;
    gp_Vec t(tanAtS);
    gp_Vec nrm(pln.Axis().Direction());
    gp_Vec perpST = t.Crossed(nrm);
    if (perpST.Magnitude() <= Precision::Confusion()) return false;
    perpST.Normalize();
    const double denom = 2.0 * se.Dot(perpST);
    if (std::abs(denom) <= Precision::Confusion()) return false;
    const double s = sq / denom;
    const gp_Pnt C = S.Translated(s * perpST);
    const double R = gp_Vec(C, S).Magnitude();
    if (R <= Precision::Confusion()) return false;
    outMid = sketchArcMidPointOnCircle(pln, C, R, S, E);
    return true;
}

bool Widget::sketchArcFromStartRadiusSweep(const gp_Pln& pln, const gp_Pnt& S, const gp_Dir& refTan, double R,
                                           double sweepDeg, gp_Pnt& outEnd, gp_Pnt& outMid, gp_Pnt& outCenter) const
{
    if (R <= Precision::Confusion() || std::abs(sweepDeg) <= Precision::Confusion()) return false;
    gp_Vec tan(refTan);
    if (tan.Magnitude() <= Precision::Confusion()) return false;
    tan.Normalize();
    gp_Vec z(pln.Axis().Direction());
    gp_Vec toC = z.Crossed(tan);
    if (toC.Magnitude() <= Precision::Confusion()) return false;
    toC.Normalize();
    outCenter = S.Translated(R * toC);
    const gp_Ax1 roax(outCenter, pln.Axis().Direction());
    const double sweepRad = sweepDeg * M_PI / 180.0;
    outMid = S.Rotated(roax, sweepRad * 0.5);
    outEnd = S.Rotated(roax, sweepRad);
    return true;
}

void Widget::positionSketchToolInputDialog()
{
    if (!sketchToolInputDialog_ || !vtkWidget) return;
    const QPoint g = vtkWidget->mapToGlobal(QPoint(0, 0));
    sketchToolInputDialog_->move(g.x() + vtkWidget->width() - sketchToolInputDialog_->width() - 8,
                                 g.y() + 8);
}

void Widget::positionSketchAuxDialog(QDialog* dlg)
{
    if (!dlg || !vtkWidget) return;
    const QPoint g = vtkWidget->mapToGlobal(QPoint(0, 0));
    dlg->move(g.x() + vtkWidget->width() - dlg->width() - 8, g.y() + 8);
}

void Widget::openOrRaiseSketchRectangleModeDialog()
{
    if (!sketchRectangleModeDialog_) {
        sketchRectangleModeDialog_ = new SketchRectangleModeDialog(this);
        sketchRectangleModeDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchRectangleModeDialog_, &SketchRectangleModeDialog::closedByUser, this, [this]() {
            sketchRectangleModeDialog_ = nullptr;
        });
        connect(sketchRectangleModeDialog_, &QObject::destroyed, this, [this]() {
            sketchRectangleModeDialog_ = nullptr;
        });
        connect(sketchRectangleModeDialog_, &SketchRectangleModeDialog::methodChanged, this,
                [this](SketchRectangleModeDialog::Method) {
                    sketchClickCount_ = 0;
                    clearSketchPreviewRectangle();
                    clearSketchPreviewLine();
                });
    }
    sketchRectangleModeDialog_->show();
    sketchRectangleModeDialog_->raise();
    positionSketchAuxDialog(sketchRectangleModeDialog_);
}

void Widget::closeSketchRectangleModeDialog()
{
    if (sketchRectangleModeDialog_) {
        sketchRectangleModeDialog_->close();
        sketchRectangleModeDialog_ = nullptr;
    }
}

void Widget::openOrRaiseSketchCircleModeDialog()
{
    if (!sketchCircleModeDialog_) {
        sketchCircleModeDialog_ = new SketchCircleModeDialog(this);
        sketchCircleModeDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchCircleModeDialog_, &SketchCircleModeDialog::closedByUser, this, [this]() {
            sketchCircleModeDialog_ = nullptr;
        });
        connect(sketchCircleModeDialog_, &QObject::destroyed, this, [this]() {
            sketchCircleModeDialog_ = nullptr;
        });
        connect(sketchCircleModeDialog_, &SketchCircleModeDialog::methodChanged, this,
                [this](SketchCircleModeDialog::Method) {
                    sketchClickCount_ = 0;
                    clearSketchPreviewCircle();
                    clearSketchPreviewLine();
                });
    }
    sketchCircleModeDialog_->show();
    sketchCircleModeDialog_->raise();
    positionSketchAuxDialog(sketchCircleModeDialog_);
}

void Widget::closeSketchCircleModeDialog()
{
    if (sketchCircleModeDialog_) {
        sketchCircleModeDialog_->close();
        sketchCircleModeDialog_ = nullptr;
    }
}

void Widget::armSnapForCuboidOriginKind(int snapKind)
{
    clearSnapSettings();
    if (!ui) return;
    snap_.enabled = true;
    snap_.armed = true;
    if (ui->Use_Capture)
        ui->Use_Capture->setChecked(true);

    snap_.nearest = (snapKind == 0);
    snap_.endpoint = (snapKind == 1);
    snap_.midpoint = (snapKind == 2);
    snap_.intersection = (snapKind == 3);
    snap_.center = (snapKind == 4);
    snap_.quadrant = (snapKind == 5);

    ui->Capture_Closed->setChecked(snap_.nearest);
    ui->Capture_Endpoint->setChecked(snap_.endpoint);
    ui->Capture_Midpoint->setChecked(snap_.midpoint);
    ui->Capture_Insertsectionpoint->setChecked(snap_.intersection);
    ui->Capture_Arccenterpoint->setChecked(snap_.center);
    ui->Capture_Quadrantpoint->setChecked(snap_.quadrant);
    syncTabPointSnapToolbarsFromCaptureRow();
    mergeSnapFiltersFromToolbarAndCaptureUi();
}

QList<gp_Pnt> Widget::sketchPolygonVertices(const gp_Pnt& center, int n,
                                            SketchPolygonDialog::SizeMode mode,
                                            double sizeVal, double rotDeg) const
{
    QList<gp_Pnt> out;
    if (n < 3 || !hasActiveSketch_ || sizeVal <= Precision::Confusion()) return out;

    double R = 0.0;
    switch (mode) {
    case SketchPolygonDialog::InscribedRadius:
        R = sizeVal / std::cos(M_PI / static_cast<double>(n));
        break;
    case SketchPolygonDialog::CircumscribedRadius:
        R = sizeVal;
        break;
    case SketchPolygonDialog::SideLength:
        R = sizeVal / (2.0 * std::sin(M_PI / static_cast<double>(n)));
        break;
    }
    if (R <= Precision::Confusion()) return out;

    const gp_Ax3 ax = activeSketchPlane_.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    const double theta0 = rotDeg * M_PI / 180.0;

    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double ang = theta0 + (2.0 * M_PI * static_cast<double>(i)) / static_cast<double>(n);
        gp_Vec ux(xd);
        gp_Vec uy(yd);
        out.append(center.Translated(R * std::cos(ang) * ux + R * std::sin(ang) * uy));
    }
    return out;
}

bool Widget::sketchPolygonParamsFromHover(const gp_Pnt& center, const gp_Pnt& hover,
                                          double& outSize, double& outRotDeg) const
{
    if (!sketchPolygonDialog_ || !hasActiveSketch_) return false;

    double cu = 0.0, cv = 0.0, u = 0.0, v = 0.0;
    if (!sketchWorldToUV(activeSketchPlane_, center, cu, cv)) return false;
    if (!sketchWorldToUV(activeSketchPlane_, hover, u, v)) return false;

    const double du = u - cu;
    const double dv = v - cv;
    const double dist = std::hypot(du, dv);
    double angDeg = std::atan2(dv, du) * 180.0 / M_PI;

    const int n = sketchPolygonDialog_->sideCount();
    const auto mode = sketchPolygonDialog_->sizeMode();
    outSize = sketchPolygonDialog_->sizeValue();
    outRotDeg = sketchPolygonDialog_->rotationDeg();

    if (!sketchPolygonDialog_->sizeLocked()) {
        switch (mode) {
        case SketchPolygonDialog::InscribedRadius:
            outSize = dist;
            break;
        case SketchPolygonDialog::CircumscribedRadius:
            outSize = dist;
            break;
        case SketchPolygonDialog::SideLength:
            outSize = 2.0 * dist * std::sin(M_PI / static_cast<double>(n));
            break;
        }
    }

    if (!sketchPolygonDialog_->rotationLocked()) {
        switch (mode) {
        case SketchPolygonDialog::InscribedRadius:
            outRotDeg = angDeg + 180.0 / static_cast<double>(n);
            break;
        case SketchPolygonDialog::CircumscribedRadius:
        case SketchPolygonDialog::SideLength:
        default:
            outRotDeg = angDeg;
            break;
        }
    }

    return outSize > Precision::Confusion();
}

void Widget::rebuildSketchPolygonPreviewFromHover(const gp_Pnt& hoverWorld)
{
    if (!sketchPolygonDialog_ || !sketchPolygonHasCenter_ || !hasActiveSketch_) {
        clearSketchPreviewPolygon();
        clearSketchPreviewLine();
        return;
    }

    double sizeVal = 0.0;
    double rotDeg = 0.0;
    if (!sketchPolygonParamsFromHover(sketchPolygonCenter_, hoverWorld, sizeVal, rotDeg)) {
        clearSketchPreviewPolygon();
        clearSketchPreviewLine();
        return;
    }

    if (!sketchPolygonDialog_->sizeLocked())
        sketchPolygonDialog_->setSizeValue(sizeVal);
    if (!sketchPolygonDialog_->rotationLocked())
        sketchPolygonDialog_->setRotationDeg(rotDeg);

    const int n = sketchPolygonDialog_->sideCount();
    const auto mode = sketchPolygonDialog_->sizeMode();
    const QList<gp_Pnt> verts = sketchPolygonVertices(sketchPolygonCenter_, n, mode, sizeVal, rotDeg);
    updateSketchPreviewPolygon(verts);

    gp_Pnt guideEnd = hoverWorld;
    if (!verts.isEmpty()) {
        double bestScore = -2.0;
        const gp_Vec toMouse(sketchPolygonCenter_, hoverWorld);
        if (toMouse.Magnitude() > Precision::Confusion()) {
            for (const gp_Pnt& vtx : verts) {
                const gp_Vec toVtx(sketchPolygonCenter_, vtx);
                if (toVtx.Magnitude() <= Precision::Confusion()) continue;
                const double score = toVtx.Normalized().Dot(toMouse.Normalized());
                if (score > bestScore) {
                    bestScore = score;
                    guideEnd = vtx;
                }
            }
        }
    }
    updateSketchPreviewPolygonGuideLine(sketchPolygonCenter_, guideEnd, true);

    if (sketchPolygonValueDialog_) {
        sketchPolygonValueDialog_->setSizeMode(mode);
        sketchPolygonValueDialog_->setSizeValue(sizeVal);
        sketchPolygonValueDialog_->setRotationDeg(rotDeg);
    }

    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

bool Widget::commitSketchPolygonFromParams(const gp_Pnt& center, const gp_Pnt& sizeRef)
{
    if (!sketchPolygonDialog_ || !hasActiveSketch_) return false;

    double sizeVal = 0.0;
    double rotDeg = 0.0;
    if (sketchPolygonValueDialog_ && sketchPolygonValueDialog_->isVisible()) {
        sizeVal = sketchPolygonValueDialog_->sizeValue();
        rotDeg = sketchPolygonValueDialog_->rotationDeg();
    } else if (!sketchPolygonParamsFromHover(center, sizeRef, sizeVal, rotDeg)) {
        return false;
    }

    const int n = sketchPolygonDialog_->sideCount();
    const auto mode = sketchPolygonDialog_->sizeMode();
    const QList<gp_Pnt> verts = sketchPolygonVertices(center, n, mode, sizeVal, rotDeg);
    if (verts.size() < 3) return false;

    for (int i = 0; i < verts.size(); ++i) {
        const gp_Pnt& a = verts[i];
        const gp_Pnt& b = verts[(i + 1) % verts.size()];
        if (a.Distance(b) <= Precision::Confusion()) continue;
        activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(a, b).Edge());
    }
    ensureSketchHistoryRecord();
    updateSketchHistoryShape();
    markDocumentModified(true);

    clearSketchPreviewPolygon();
    clearSketchPreviewLine();
    sketchPolygonHasCenter_ = false;
    sketchClickCount_ = 0;
    if (sketchPolygonDialog_) {
        sketchPolygonDialog_->setCenterText(QString());
        sketchPolygonDialog_->setSizeText(QString());
        sketchPolygonDialog_->highlightPickField(SketchPolygonDialog::PickCenter);
    }
    closeSketchPolygonValueDialog();
    beginSketchPolygonPick(0);
    return true;
}

void Widget::sketchApplyPolygonManualInput()
{
    if (!sketchPolygonDialog_ || !sketchPolygonHasCenter_ || !hasActiveSketch_) return;

    double sizeVal = sketchPolygonValueDialog_ ? sketchPolygonValueDialog_->sizeValue()
                                               : sketchPolygonDialog_->sizeValue();
    double rotDeg = sketchPolygonValueDialog_ ? sketchPolygonValueDialog_->rotationDeg()
                                              : sketchPolygonDialog_->rotationDeg();

    sketchPolygonDialog_->setSizeValue(sizeVal);
    sketchPolygonDialog_->setRotationDeg(rotDeg);

    const int n = sketchPolygonDialog_->sideCount();
    const auto mode = sketchPolygonDialog_->sizeMode();
    const QList<gp_Pnt> verts = sketchPolygonVertices(sketchPolygonCenter_, n, mode, sizeVal, rotDeg);
    updateSketchPreviewPolygon(verts);

    gp_Pnt guideEnd = sketchLastHoverValid_ ? sketchLastHoverPoint_ : sketchPolygonCenter_;
    if (!verts.isEmpty() && sketchLastHoverValid_) {
        double bestScore = -1.0;
        for (const gp_Pnt& vtx : verts) {
            const gp_Vec toVtx(sketchPolygonCenter_, vtx);
            const gp_Vec toMouse(sketchPolygonCenter_, sketchLastHoverPoint_);
            if (toVtx.Magnitude() <= Precision::Confusion()) continue;
            const double score = toVtx.Normalized().Dot(toMouse.Normalized());
            if (score > bestScore) {
                bestScore = score;
                guideEnd = vtx;
            }
        }
    }
    updateSketchPreviewPolygonGuideLine(sketchPolygonCenter_, guideEnd, true);

    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::beginSketchPolygonPick(int field)
{
    if (!sketchPolygonDialog_ || !hasActiveSketch_) return;
    sketchPolygonPendingField_ = field;
    const int k = (field == 0) ? sketchPolygonDialog_->centerSnapKind()
                               : sketchPolygonDialog_->sizeSnapKind();
    if (k >= 0)
        armSnapForCuboidOriginKind(k);
    else
        restoreSnapAfterSketchConicPick();

    currentSelectionMode = SketchPolygonPick;
    sketchPolygonDialog_->highlightPickField(field == 0 ? SketchPolygonDialog::PickCenter
                                                        : SketchPolygonDialog::PickSize);
    statusBar()->showMessage(field == 0 ? tr("多边形：在草图平面内点击确定中心点。")
                                        : tr("多边形：在草图平面内点击确定大小与方向。"),
                             4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::completeSketchPolygonPick(const gp_Pnt& p)
{
    if (!sketchPolygonDialog_) {
        currentSelectionMode = None;
        return;
    }

    auto fmt = [](const gp_Pnt& q) {
        return QStringLiteral("(%1,%2,%3)")
            .arg(q.X(), 0, 'f', 3)
            .arg(q.Y(), 0, 'f', 3)
            .arg(q.Z(), 0, 'f', 3);
    };

    if (sketchPolygonPendingField_ == 0) {
        sketchPolygonCenter_ = p;
        sketchPolygonHasCenter_ = true;
        sketchP1_ = p;
        sketchClickCount_ = 1;
        sketchPolygonDialog_->setCenterText(fmt(p));
        sketchPolygonPendingField_ = -1;
        restoreSnapAfterSketchConicPick();
        sketchPolygonDialog_->highlightPickField(SketchPolygonDialog::PickSize);
        currentSelectionMode = SketchDrawPolygon;
        openOrRaiseSketchPolygonValueDialog();
        statusBar()->showMessage(tr("多边形：移动鼠标预览；点击确认或输入半径/长度与旋转。"), 5000);
    } else if (sketchPolygonPendingField_ == 1) {
        sketchPolygonDialog_->setSizeText(fmt(p));
        sketchPolygonPendingField_ = -1;
        restoreSnapAfterSketchConicPick();
        sketchPolygonDialog_->highlightPickField(SketchPolygonDialog::PickNone);
        commitSketchPolygonFromParams(sketchPolygonCenter_, p);
        currentSelectionMode = SketchPolygonPick;
        statusBar()->showMessage(tr("多边形已创建。"), 2500);
        return;
    }

    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::openOrRaiseSketchPolygonValueDialog()
{
    if (!sketchPolygonValueDialog_) {
        sketchPolygonValueDialog_ = new SketchPolygonValueDialog(this);
        sketchPolygonValueDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchPolygonValueDialog_, &QObject::destroyed, this, [this]() {
            sketchPolygonValueDialog_ = nullptr;
        });
        connect(sketchPolygonValueDialog_, &SketchPolygonValueDialog::valuesCommitted, this,
                &Widget::sketchApplyPolygonManualInput);
        connect(sketchPolygonValueDialog_, &SketchPolygonValueDialog::closedByUser, this, [this]() {
            sketchPolygonValueDialog_ = nullptr;
        });
    }
    if (sketchPolygonDialog_) {
        sketchPolygonValueDialog_->setSizeMode(sketchPolygonDialog_->sizeMode());
    }
    sketchPolygonValueDialog_->show();
    sketchPolygonValueDialog_->raise();
}

void Widget::closeSketchPolygonValueDialog()
{
    if (sketchPolygonValueDialog_) {
        sketchPolygonValueDialog_->close();
        sketchPolygonValueDialog_ = nullptr;
    }
}

void Widget::positionSketchPolygonValueDialog(int screenX, int screenY)
{
    if (!sketchPolygonValueDialog_) return;
    const int ox = 16;
    const int oy = 16;
    sketchPolygonValueDialog_->move(screenX + ox, screenY + oy);
}

void Widget::openOrRaiseSketchPolygonDialog()
{
    if (!sketchPolygonDialog_) {
        sketchPolygonDialog_ = new SketchPolygonDialog(this);
        sketchPolygonDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchPolygonDialog_, &QObject::destroyed, this, [this]() { sketchPolygonDialog_ = nullptr; });
        connect(sketchPolygonDialog_, &QDialog::rejected, this, [this]() {
            sketchPolygonHasCenter_ = false;
            sketchPolygonPendingField_ = -1;
            sketchClickCount_ = 0;
            if (currentSelectionMode == SketchPolygonPick || currentSelectionMode == SketchDrawPolygon)
                currentSelectionMode = None;
            clearSketchPreviewPolygon();
            clearSketchPreviewLine();
            closeSketchPolygonValueDialog();
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::pickCenterRequested, this, [this]() {
            beginSketchPolygonPick(0);
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::pickSizeRequested, this, [this]() {
            if (!sketchPolygonHasCenter_) {
                statusBar()->showMessage(tr("请先指定中心点。"), 2500);
                return;
            }
            beginSketchPolygonPick(1);
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::sizeModeChanged, this,
                [this](SketchPolygonDialog::SizeMode mode) {
                    if (sketchPolygonValueDialog_) {
                        sketchPolygonValueDialog_->setSizeMode(mode);
                    }
                    if (sketchLastHoverValid_)
                        rebuildSketchPolygonPreviewFromHover(sketchLastHoverPoint_);
                });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::paramsChanged, this, [this]() {
            if (sketchPolygonHasCenter_ && sketchLastHoverValid_)
                rebuildSketchPolygonPreviewFromHover(sketchLastHoverPoint_);
        });
    }

    sketchPolygonHasCenter_ = false;
    sketchClickCount_ = 0;
    sketchPolygonDialog_->setCenterText(QString());
    sketchPolygonDialog_->setSizeText(QString());
    sketchPolygonDialog_->show();
    sketchPolygonDialog_->raise();
    positionSketchAuxDialog(sketchPolygonDialog_);
    beginSketchPolygonPick(0);
}

void Widget::closeSketchPolygonDialog()
{
    if (sketchPolygonDialog_) {
        sketchPolygonDialog_->close();
        sketchPolygonDialog_ = nullptr;
    }
    sketchPolygonHasCenter_ = false;
    sketchPolygonPendingField_ = -1;
    closeSketchPolygonValueDialog();
    if (currentSelectionMode == SketchPolygonPick || currentSelectionMode == SketchDrawPolygon)
        currentSelectionMode = None;
    clearSketchPreviewPolygon();
    clearSketchPreviewLine();
}

bool Widget::buildSketchEllipseEdge(const gp_Pnt& center, double majorR, double minorR, double rotDeg,
                                    TopoDS_Edge& outEdge) const
{
    outEdge.Nullify();
    if (!hasActiveSketch_ || majorR <= Precision::Confusion() || minorR <= Precision::Confusion())
        return false;

    // OCCT 要求 major >= minor，否则构造 gp_Elips 可能抛异常导致崩溃
    if (minorR > majorR)
        minorR = majorR;

    const gp_Ax3 ax = activeSketchPlane_.Position();
    const gp_Dir xd = ax.XDirection();
    const gp_Dir yd = ax.YDirection();
    const double th = rotDeg * M_PI / 180.0;
    gp_Vec majDir = std::cos(th) * gp_Vec(xd) + std::sin(th) * gp_Vec(yd);
    if (majDir.Magnitude() <= Precision::Confusion()) return false;
    majDir.Normalize();

    try {
        const gp_Ax2 ax2(center, activeSketchPlane_.Axis().Direction(), gp_Dir(majDir));
        const gp_Elips elips(ax2, majorR, minorR);
        Handle(Geom_Ellipse) geom = new Geom_Ellipse(elips);
        BRepBuilderAPI_MakeEdge maker(geom);
        if (!maker.IsDone()) return false;
        outEdge = maker.Edge();
        return !outEdge.IsNull();
    } catch (const Standard_Failure&) {
        return false;
    }
}

void Widget::updateSketchEllipseGeometry()
{
    if (!sketchEllipseDialog_ || !sketchEllipseHasCenter_ || !hasActiveSketch_) return;

    TopoDS_Edge edge;
    if (!buildSketchEllipseEdge(sketchEllipseCenter_,
                                sketchEllipseDialog_->majorRadius(),
                                sketchEllipseDialog_->minorRadius(),
                                sketchEllipseDialog_->rotationDeg(),
                                edge)) {
        return;
    }

    if (sketchEllipseGeomIndex_ < 0) {
        activeSketch_.addGeometry(edge);
        const QList<TopoDS_Shape> gg = activeSketch_.getGeometries();
        sketchEllipseGeomIndex_ = gg.size() - 1;
    } else {
        QList<TopoDS_Shape> gg = activeSketch_.getGeometries();
        if (sketchEllipseGeomIndex_ >= 0 && sketchEllipseGeomIndex_ < gg.size()) {
            gg[sketchEllipseGeomIndex_] = edge;
            activeSketch_.setGeometries(gg);
        } else {
            activeSketch_.addGeometry(edge);
            sketchEllipseGeomIndex_ = activeSketch_.getGeometries().size() - 1;
        }
    }
    ensureSketchHistoryRecord();
    updateSketchHistoryShape();
    markDocumentModified(true);
    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::sketchEllipseRotationFromHover(const gp_Pnt& hoverWorld, double& outRotDeg) const
{
    double cu = 0.0, cv = 0.0, u = 0.0, v = 0.0;
    if (!sketchWorldToUV(activeSketchPlane_, sketchEllipseCenter_, cu, cv)) return;
    if (!sketchWorldToUV(activeSketchPlane_, hoverWorld, u, v)) return;
    const double du = u - cu;
    const double dv = v - cv;
    if (std::hypot(du, dv) <= Precision::Confusion()) return;
    outRotDeg = std::atan2(dv, du) * 180.0 / M_PI;
}

void Widget::sketchApplyEllipseAngleFromDialog()
{
    if (!sketchEllipseDialog_ || !sketchEllipseHasCenter_) return;
    if (sketchEllipseAngleDialog_) {
        sketchEllipseDialog_->setRotationDeg(sketchEllipseAngleDialog_->rotationDeg());
    }
    updateSketchEllipseGeometry();
}

void Widget::handleSketchEllipseAdjustMouseMove(int x, int y)
{
    if (!sketchEllipseHasCenter_ || !hasActiveSketch_ || !sketchEllipseDialog_) return;

    if (vtkWidget) {
        const QRect vtkLocal(0, 0, vtkWidget->width(), vtkWidget->height());
        if (!vtkLocal.contains(QPoint(x, y))) {
            return;
        }
    }

    if (sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_->setRotationDeg(sketchEllipseDialog_->rotationDeg());
        positionSketchEllipseAngleDialog(x, y);
        if (!sketchEllipseAngleDialog_->isVisible()) {
            sketchEllipseAngleDialog_->show();
        }
    }
}

void Widget::handleSketchEllipseAdjustMouseDown(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void Widget::handleSketchEllipseAdjustMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void Widget::beginSketchEllipsePick(int field)
{
    if (!sketchEllipseDialog_ || !hasActiveSketch_) return;
    if ((field == 1 || field == 2) && !sketchEllipseHasCenter_) {
        statusBar()->showMessage(tr("请先指定中心点。"), 2500);
        return;
    }

    if (sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_->hide();
    }

    sketchEllipsePendingField_ = field;
    int k = -1;
    if (field == 0) k = sketchEllipseDialog_->centerSnapKind();
    else if (field == 1) k = sketchEllipseDialog_->majorSnapKind();
    else if (field == 2) k = sketchEllipseDialog_->minorSnapKind();

    if (k >= 0) armSnapForCuboidOriginKind(k);
    else restoreSnapAfterSketchConicPick();

    currentSelectionMode = SketchEllipsePick;
    sketchEllipseDialog_->highlightPickField(static_cast<SketchEllipseDialog::PickField>(field));
    sketchEllipseDialog_->raise();
    sketchEllipseDialog_->activateWindow();
    statusBar()->showMessage(field == 0 ? tr("椭圆：在草图平面内点击确定中心点。")
                          : field == 1 ? tr("椭圆：在草图平面内点击确定大半径。")
                                       : tr("椭圆：在草图平面内点击确定小半径。"),
                             4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::completeSketchEllipsePick(const gp_Pnt& p)
{
    if (!sketchEllipseDialog_) {
        currentSelectionMode = None;
        return;
    }

    auto fmt = [](const gp_Pnt& q) {
        return QStringLiteral("(%1,%2,%3)")
            .arg(q.X(), 0, 'f', 3)
            .arg(q.Y(), 0, 'f', 3)
            .arg(q.Z(), 0, 'f', 3);
    };

    if (sketchEllipsePendingField_ == 0) {
        sketchEllipseCenter_ = p;
        sketchEllipseHasCenter_ = true;
        sketchEllipseGeomIndex_ = -1;
        sketchEllipseDialog_->setCenterText(fmt(p));
        sketchEllipsePendingField_ = -1;
        restoreSnapAfterSketchConicPick();
        sketchEllipseDialog_->highlightPickField(SketchEllipseDialog::PickNone);
        updateSketchEllipseGeometry();
        currentSelectionMode = SketchEllipseAdjust;
        openOrRaiseSketchEllipseAngleDialog();
        statusBar()->showMessage(tr("椭圆已创建（默认半径）；可在对话框或渲染窗口数值框中修改角度。"), 5000);
    } else if (sketchEllipsePendingField_ == 1) {
        double cu = 0.0, cv = 0.0, u = 0.0, v = 0.0;
        sketchWorldToUV(activeSketchPlane_, sketchEllipseCenter_, cu, cv);
        sketchWorldToUV(activeSketchPlane_, p, u, v);
        const double du = u - cu;
        const double dv = v - cv;
        const double dist = std::hypot(du, dv);
        if (dist > Precision::Confusion()) {
            const double th = sketchEllipseDialog_->rotationDeg() * M_PI / 180.0;
            const double mux = std::cos(th);
            const double muy = std::sin(th);
            const double majorR = std::abs(du * mux + dv * muy);
            if (majorR > Precision::Confusion()) {
                double minorR = sketchEllipseDialog_->minorRadius();
                if (minorR > majorR)
                    minorR = majorR;
                sketchEllipseDialog_->setRadii(majorR, minorR);
            }
        }
        sketchEllipseDialog_->setMajorPickText(fmt(p));
        sketchEllipsePendingField_ = -1;
        restoreSnapAfterSketchConicPick();
        sketchEllipseDialog_->highlightPickField(SketchEllipseDialog::PickNone);
        updateSketchEllipseGeometry();
        currentSelectionMode = SketchEllipseAdjust;
        openOrRaiseSketchEllipseAngleDialog();
        sketchEllipseDialog_->raise();
    } else if (sketchEllipsePendingField_ == 2) {
        double cu = 0.0, cv = 0.0, u = 0.0, v = 0.0;
        sketchWorldToUV(activeSketchPlane_, sketchEllipseCenter_, cu, cv);
        sketchWorldToUV(activeSketchPlane_, p, u, v);
        const double du = u - cu;
        const double dv = v - cv;
        const double dist = std::hypot(du, dv);
        if (dist > Precision::Confusion()) {
            const double th = sketchEllipseDialog_->rotationDeg() * M_PI / 180.0;
            const double mux = std::cos(th);
            const double muy = std::sin(th);
            const double minorR = std::abs(-du * muy + dv * mux);
            if (minorR > Precision::Confusion()) {
                double majorR = sketchEllipseDialog_->majorRadius();
                if (minorR > majorR)
                    majorR = minorR;
                sketchEllipseDialog_->setRadii(majorR, minorR);
            }
        }
        sketchEllipseDialog_->setMinorPickText(fmt(p));
        sketchEllipsePendingField_ = -1;
        restoreSnapAfterSketchConicPick();
        sketchEllipseDialog_->highlightPickField(SketchEllipseDialog::PickNone);
        updateSketchEllipseGeometry();
        currentSelectionMode = SketchEllipseAdjust;
        openOrRaiseSketchEllipseAngleDialog();
        sketchEllipseDialog_->raise();
    }

    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::openOrRaiseSketchEllipseAngleDialog()
{
    if (!sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_ = new SketchEllipseAngleDialog(this);
        sketchEllipseAngleDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchEllipseAngleDialog_, &QObject::destroyed, this, [this]() {
            sketchEllipseAngleDialog_ = nullptr;
        });
        connect(sketchEllipseAngleDialog_, &SketchEllipseAngleDialog::angleCommitted, this,
                &Widget::sketchApplyEllipseAngleFromDialog);
        connect(sketchEllipseAngleDialog_, &SketchEllipseAngleDialog::closedByUser, this, [this]() {
            sketchEllipseAngleDialog_ = nullptr;
        });
    }
    if (sketchEllipseDialog_) {
        sketchEllipseAngleDialog_->setRotationDeg(sketchEllipseDialog_->rotationDeg());
    }
    sketchEllipseAngleDialog_->show();
    sketchEllipseAngleDialog_->raise();
}

void Widget::closeSketchEllipseAngleDialog()
{
    if (sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_->close();
        sketchEllipseAngleDialog_ = nullptr;
    }
}

void Widget::positionSketchEllipseAngleDialog(int screenX, int screenY)
{
    if (!sketchEllipseAngleDialog_ || !vtkWidget) return;

    const QRect vtkLocal(0, 0, vtkWidget->width(), vtkWidget->height());
    if (!vtkLocal.contains(QPoint(screenX, screenY))) {
        return;
    }

    const QPoint g = vtkWidget->mapToGlobal(QPoint(screenX, screenY));
    const QSize sz = sketchEllipseAngleDialog_->sizeHint();
    const QRect vtkGlobal(vtkWidget->mapToGlobal(QPoint(0, 0)), vtkWidget->size());
    int px = g.x() + 16;
    int py = g.y() + 16;
    px = qBound(vtkGlobal.left(), px, qMax(vtkGlobal.left(), vtkGlobal.right() - sz.width()));
    py = qBound(vtkGlobal.top(), py, qMax(vtkGlobal.top(), vtkGlobal.bottom() - sz.height()));
    sketchEllipseAngleDialog_->move(px, py);
}

void Widget::openOrRaiseSketchEllipseDialog()
{
    if (!sketchEllipseDialog_) {
        sketchEllipseDialog_ = new SketchEllipseDialog(this);
        sketchEllipseDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchEllipseDialog_, &QObject::destroyed, this, [this]() { sketchEllipseDialog_ = nullptr; });
        connect(sketchEllipseDialog_, &QDialog::rejected, this, [this]() {
            sketchEllipseHasCenter_ = false;
            sketchEllipsePendingField_ = -1;
            sketchEllipseGeomIndex_ = -1;
            if (currentSelectionMode == SketchEllipsePick || currentSelectionMode == SketchEllipseAdjust)
                currentSelectionMode = None;
            closeSketchEllipseAngleDialog();
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickCenterRequested, this, [this]() {
            beginSketchEllipsePick(0);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickMajorRequested, this, [this]() {
            beginSketchEllipsePick(1);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickMinorRequested, this, [this]() {
            beginSketchEllipsePick(2);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::paramsChanged, this, [this]() {
            if (sketchEllipseHasCenter_) {
                if (sketchEllipseAngleDialog_) {
                    sketchEllipseAngleDialog_->setRotationDeg(sketchEllipseDialog_->rotationDeg());
                }
                updateSketchEllipseGeometry();
            }
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::applyRequested, this, [this]() {
            if (sketchEllipseHasCenter_) updateSketchEllipseGeometry();
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::okRequested, this, [this]() {
            if (sketchEllipseHasCenter_) updateSketchEllipseGeometry();
            statusBar()->showMessage(tr("椭圆参数已确认。"), 2500);
        });
    }

    sketchEllipseHasCenter_ = false;
    sketchEllipseGeomIndex_ = -1;
    sketchEllipsePendingField_ = -1;
    sketchEllipseDialog_->setCenterText(QString());
    sketchEllipseDialog_->setMajorPickText(QString());
    sketchEllipseDialog_->setMinorPickText(QString());
    sketchEllipseDialog_->show();
    sketchEllipseDialog_->raise();
    positionSketchAuxDialog(sketchEllipseDialog_);
    beginSketchEllipsePick(0);
}

void Widget::closeSketchEllipseDialog()
{
    if (sketchEllipseDialog_) {
        sketchEllipseDialog_->close();
        sketchEllipseDialog_ = nullptr;
    }
    sketchEllipseHasCenter_ = false;
    sketchEllipsePendingField_ = -1;
    sketchEllipseGeomIndex_ = -1;
    sketchEllipseAngleDragActive_ = false;
    closeSketchEllipseAngleDialog();
    if (currentSelectionMode == SketchEllipsePick || currentSelectionMode == SketchEllipseAdjust)
        currentSelectionMode = None;
}

void Widget::openOrRaiseSketchToolInput(SketchToolInputDialog::ObjectKind initialObject)
{
    if (!sketchToolInputDialog_) {
        sketchToolInputDialog_ = new SketchToolInputDialog(initialObject, this);
        sketchToolInputDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchToolInputDialog_, &SketchToolInputDialog::closedByUser, this, [this]() {
            sketchToolInputDialog_ = nullptr;
            sketchCommittedPointValid_ = false;
            clearSketchPreviewArc();
            clearSketchPreviewLine();
            if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        });
        connect(sketchToolInputDialog_, &QObject::destroyed, this, [this]() {
            sketchToolInputDialog_ = nullptr;
        });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::valuesCommitted, this, &Widget::sketchApplyManualInputFromDialog);
        connect(sketchToolInputDialog_, &SketchToolInputDialog::inputKindChanged, this, [this](SketchToolInputDialog::InputKind) {
            if (hasActiveSketch_) {
                sketchCommittedPointValid_ = false;
                clearSketchPreviewArc();
                clearSketchPreviewLine();
                if (sketchLastHoverValid_) updateSketchToolInputDialogFields(sketchLastHoverPoint_);
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
        });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::objectKindChanged, this,
                [this](SketchToolInputDialog::ObjectKind k) {
                    // 直线/圆弧切换时始终清除预览，避免相切圆弧(count==1)等情况下残留高亮
                    clearSketchPreviewArc();
                    clearSketchPreviewLine();
                    if (k == SketchToolInputDialog::ObjLine) {
                        if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 2) {
                            sketchClickCount_ = 0;
                        }
                        currentSelectionMode = SketchDrawLine;
                    } else {
                        currentSelectionMode = SketchDrawArc;
                    }
                    sketchCommittedPointValid_ = false;
                    if (hasActiveSketch_ && sketchLastHoverValid_) updateSketchToolInputDialogFields(sketchLastHoverPoint_);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::sketchButtonClicked, this, &Widget::openSketchPlaneDialogFromTool);
    } else {
        sketchToolInputDialog_->setObjectKind(initialObject);
    }
    sketchCommittedPointValid_ = false;
    sketchToolInputDialog_->show();
    sketchToolInputDialog_->raise();
    positionSketchToolInputDialog();
    {
        gp_Pnt ref = activeSketchPlane_.Location();
        if (sketchLastHoverValid_) ref = sketchLastHoverPoint_;
        updateSketchToolInputDialogFields(ref);
    }
}

void Widget::ensureDefaultSketchForTools()
{
    if (hasActiveSketch_) return;

    // 默认草图：使用世界 XY 平面
    activeSketch_.clear();
    clearSketchCommittedOverlay();
    activeSketchPlane_ = gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    activeSketch_.setPlane(activeSketchPlane_);
    activeSketch_.setName(tr("草图"));
    hasActiveSketch_ = true;
    sketchClickCount_ = 0;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    activeSketchHistoryIndex_ = -1;

    ensureSketchHistoryRecord();
    statusBar()->showMessage(tr("未确定基准平面：已使用默认XY平面开始草图，可点“草图”按钮重新选择平面。"), 4000);
}

void Widget::openSketchPlaneDialogFromTool()
{
    SelectionMode restoreMode = SketchDrawLine;
    SketchToolInputDialog::ObjectKind restoreObj = SketchToolInputDialog::ObjLine;
    if (sketchToolInputDialog_) {
        restoreObj = sketchToolInputDialog_->objectKind();
        restoreMode = (restoreObj == SketchToolInputDialog::ObjArc) ? SketchDrawArc : SketchDrawLine;
    } else {
        restoreMode = (currentSelectionMode == SketchDrawArc) ? SketchDrawArc : SketchDrawLine;
        restoreObj = (restoreMode == SketchDrawArc) ? SketchToolInputDialog::ObjArc : SketchToolInputDialog::ObjLine;
    }

    auto* dlg = new SketchCreateDialog(dialogParentWidget());
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    activeSketchCreateDialog_ = dlg;

    connect(dlg, &SketchCreateDialog::requestPickPlane, this, [this]() {
        currentSelectionMode = SketchPlaneSelection;
        statusBar()->showMessage(tr("草图：请在模型上点击一个平面作为参考平面。"), 4000);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg, restoreMode, restoreObj]() {
        if (!dlg->hasPickedPlane()) {
            statusBar()->showMessage(tr("草图：未拾取参考平面，保持当前平面不变。"), 3000);
            return;
        }

        // 若已有草图几何，为避免跨平面误差，切换平面时清空草图并重新开始
        if (hasActiveSketch_ && activeSketch_.isValid()) {
            activeSketch_.clear();
            clearSketchCommittedOverlay();
            activeSketchHistoryIndex_ = -1;
            clearSketchPreviewLine();
            clearSketchPreviewArc();
            clearSketchPreviewCircle();
            clearSketchPreviewConic();
        }

        activeSketchPlane_ = dlg->pickedPlane();
        activeSketch_.setPlane(activeSketchPlane_);
        activeSketch_.setName(tr("草图"));
        hasActiveSketch_ = true;
        sketchClickCount_ = 0;
        sketchCommittedPointValid_ = false;

        ensureSketchHistoryRecord();

        alignViewToSketchPlane(activeSketchPlane_);

        currentSelectionMode = restoreMode;
        openOrRaiseSketchToolInput(restoreObj);
        statusBar()->showMessage(tr("草图平面已更新，可继续绘制。"), 2500);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::rejected, this, [this, restoreMode]() {
        // 若用户取消拾取平面，回到原绘制模式
        if (currentSelectionMode == SketchPlaneSelection) {
            currentSelectionMode = restoreMode;
        }
        activeSketchCreateDialog_ = nullptr;
    });

    connect(dlg, &QObject::destroyed, this, [this, restoreMode]() {
        if (activeSketchCreateDialog_) activeSketchCreateDialog_ = nullptr;
        if (currentSelectionMode == SketchPlaneSelection) currentSelectionMode = restoreMode;
    });

    dlg->show();
    dlg->raise();
}

void Widget::closeSketchToolInput()
{
    if (sketchToolInputDialog_) {
        sketchToolInputDialog_->close();
        sketchToolInputDialog_ = nullptr;
    }
    sketchCommittedPointValid_ = false;
}

void Widget::updateSketchToolInputDialogFields(const gp_Pnt& hoverWorld)
{
    if (!sketchToolInputDialog_ || !hasActiveSketch_) return;

    SketchToolInputDialog* dlg = sketchToolInputDialog_;
    const gp_Pln& pln = activeSketchPlane_;

    double xc = 0, yc = 0;
    sketchWorldToUV(pln, hoverWorld, xc, yc);

    if (dlg->inputKind() == SketchToolInputDialog::Coordinate) {
        dlg->setCoordinateValues(xc, yc);
        return;
    }

    if (dlg->objectKind() == SketchToolInputDialog::ObjLine) {
        if (currentSelectionMode == SketchDrawLine) {
            if (sketchClickCount_ == 0) {
                const gp_Pnt o = pln.Location();
                double len = 0, ang = 0;
                sketchLineLengthAngle(pln, o, hoverWorld, len, ang);
                dlg->setLineParameters(len, ang);
            } else if (sketchClickCount_ == 1) {
                double len = 0, ang = 0;
                sketchLineLengthAngle(pln, sketchP1_, hoverWorld, len, ang);
                dlg->setLineParameters(len, ang);
            }
        }
    } else if (dlg->objectKind() == SketchToolInputDialog::ObjArc) {
        if (currentSelectionMode == SketchDrawArc) {
            if (sketchClickCount_ == 0) {
                dlg->setArcParameters(0.0, 90.0);
            } else if (sketchClickCount_ == 1) {
                gp_Pnt pm;
                double R = 0.0;
                double sw = 90.0;
                if (sketchContourChaining_ && sketchChainTangentValid_
                    && sketchArcMidFromTangentAndEnd(pln, sketchP1_, sketchChainTangentDir_, hoverWorld, pm)) {
                    R = sketchArcRadiusFromThreePoints(sketchP1_, pm, hoverWorld);
                    try {
                        Handle(Geom_TrimmedCurve) ta = GC_MakeArcOfCircle(sketchP1_, pm, hoverWorld);
                        sw = std::abs(ta->LastParameter() - ta->FirstParameter()) * 180.0 / M_PI;
                    } catch (...) {
                    }
                }
                dlg->setArcParameters(R, sw);
            } else if (sketchClickCount_ == 2) {
                const double r = sketchArcRadiusFromThreePoints(sketchP1_, hoverWorld, sketchP2_);
                double sw = 90.0;
                try {
                    Handle(Geom_TrimmedCurve) ta = GC_MakeArcOfCircle(sketchP1_, hoverWorld, sketchP2_);
                    sw = std::abs(ta->LastParameter() - ta->FirstParameter()) * 180.0 / M_PI;
                } catch (...) {
                }
                dlg->setArcParameters(r, sw);
            }
        }
    }
}

void Widget::sketchApplyManualInputFromDialog()
{
    if (!sketchToolInputDialog_ || !hasActiveSketch_) return;

    SketchToolInputDialog* dlg = sketchToolInputDialog_;
    const gp_Pln& pln = activeSketchPlane_;

    if (dlg->inputKind() == SketchToolInputDialog::Coordinate) {
        const double xc = dlg->xcValue();
        const double yc = dlg->ycValue();
        sketchCommittedPoint_ = sketchUVToWorld(pln, xc, yc);
        sketchCommittedPointValid_ = true;
    } else if (dlg->objectKind() == SketchToolInputDialog::ObjLine) {
        if (sketchClickCount_ == 0) {
            const gp_Pnt o = pln.Location();
            const double len = dlg->lengthValue();
            const double ang = dlg->angleValue();
            sketchCommittedPoint_ = sketchLineEndFromLengthAngle(pln, o, len, ang);
        } else if (sketchClickCount_ == 1) {
            const double len = dlg->lengthValue();
            const double ang = dlg->angleValue();
            sketchCommittedPoint_ = sketchLineEndFromLengthAngle(pln, sketchP1_, len, ang);
        }
        sketchCommittedPointValid_ = true;
    } else {
        if (sketchClickCount_ == 1) {
            const double R = dlg->radiusValue();
            const double sw = dlg->sweepAngleValue();
            gp_Pnt E, pm, C;
            gp_Dir refTan = sketchChainTangentValid_ ? sketchChainTangentDir_ : pln.Position().XDirection();
            if (R > Precision::Confusion()
                && sketchArcFromStartRadiusSweep(pln, sketchP1_, refTan, R, sw, E, pm, C)) {
                try {
                    Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(sketchP1_, pm, E);
                    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc).Edge();
                    activeSketch_.addGeometry(edge);
                    ensureSketchHistoryRecord();
                    updateSketchHistoryShape();
                    if (sketchContourChaining_) {
                        sketchP1_ = E;
                        sketchClickCount_ = 1;
                        gp_Vec re(C, E);
                        gp_Vec ez(pln.Axis().Direction());
                        gp_Vec endTan = ez.Crossed(re);
                        if (endTan.Magnitude() > Precision::Confusion()) {
                            endTan.Normalize();
                            sketchChainTangentDir_ = gp_Dir(endTan);
                            sketchChainTangentValid_ = true;
                        }
                    } else {
                        sketchClickCount_ = 0;
                        sketchChainTangentValid_ = false;
                    }
                    clearSketchPreviewArc();
                    clearSketchPreviewLine();
                    markDocumentModified(true);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                    statusBar()->showMessage(tr("参数圆弧已创建。"), 2500);
                } catch (...) {
                    statusBar()->showMessage(tr("参数圆弧创建失败。"), 3000);
                }
            }
            return;
        }
        if (sketchClickCount_ == 2) {
            const double R = dlg->radiusValue();
            if (R > Precision::Confusion()) {
                gp_Pnt C;
                if (sketchArcCenterFromTwoPointsRadius(pln, sketchP1_, sketchP2_, R, sketchLastHoverPoint_, C)) {
                    const gp_Pnt pm = sketchArcMidPointOnCircle(pln, C, R, sketchP1_, sketchP2_);
                    sketchCommittedPoint_ = pm;
                    sketchCommittedPointValid_ = true;
                    updateSketchPreviewArc(sketchP1_, pm, sketchP2_);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                }
            }
        }
        return;
    }

    sketchLastHoverPoint_ = sketchCommittedPoint_;
    sketchLastHoverValid_ = true;

    if (currentSelectionMode == SketchDrawLine && sketchClickCount_ == 1) {
        updateSketchPreviewLine(sketchP1_, sketchCommittedPoint_);
    } else if (currentSelectionMode == SketchDrawRectangle && sketchClickCount_ == 1) {
        updateSketchPreviewRectangle(sketchP1_, sketchCommittedPoint_);
        clearSketchPreviewLine();
    } else if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 1) {
        updateSketchPreviewLine(sketchP1_, sketchCommittedPoint_);
    } else if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 2) {
        const gp_Pnt& pm = sketchCommittedPoint_;
        updateSketchPreviewArc(sketchP1_, pm, sketchP2_);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::ensureSketchHistoryRecord()
{
    if (activeSketchHistoryIndex_ >= 0 && activeSketchHistoryIndex_ < historyList.size()
        && historyList[activeSketchHistoryIndex_].type == SKETCH) {
        return;
    }

    // 为草图创建独立的“曲线显示”历史项（线框渲染，保留 lines）
    const QColor color(60, 170, 255);
    const gp_Pnt o = activeSketchPlane_.Location();
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(o, gp_Pnt(o.X() + 1e-3, o.Y(), o.Z())).Edge();

    ++shapeIDCounter;
    Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(e);
    shapeWrapper->SetId(shapeIDCounter);

    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
        vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    shapeDataSource->SetShape(shapeWrapper);
    shapeDataSource->Modified();
    shapeDataSource->Update();
    // 草图主显示与高亮统一使用 shapeDataSource。
    // 注意：纯线框草图（尤其是仅包含 Edge）在该输出里可能没有可用点集，
    // 不能在这里提前返回，否则会出现“只看到预览高亮、正式草图不落地”的现象。
    vtkPolyData* meshPolyData = vtkPolyData::SafeDownCast(shapeDataSource->GetOutput());

    vtkSmartPointer<vtkPolyData> polyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    polyDataCopy->ShallowCopy(meshPolyData);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyDataCopy);
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPickable(true);
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->SetLineWidth(2.0);
    actor->GetProperty()->SetLighting(false);
    actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());

    IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, actor);
    // 注意：SetShapeSource 可能改写 mapper 连接，最后再强制回设显示 mapper
    actor->SetMapper(mapper);

    vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter =
        vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
    highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
    highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");

    vtkSmartPointer<vtkPolyDataMapper> highlightMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    highlightMapper->SetInputConnection(highlightFilter->GetOutputPort());
    highlightMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> highlightActor = vtkSmartPointer<vtkActor>::New();
    highlightActor->SetMapper(highlightMapper);
    highlightActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
    highlightActor->GetProperty()->SetOpacity(1.0);
    highlightActor->GetProperty()->SetRepresentationToWireframe();
    highlightActor->GetProperty()->SetLineWidth(4.0);
    highlightActor->GetProperty()->SetLighting(false);
    highlightActor->SetVisibility(false);
    highlightActor->SetPickable(false);

    renderer->AddActor(actor);
    addAppearanceActor(highlightActor);

    ModelingHistory record;
    QString sketchName = tr("草图");
    if (sketchSelectedPlaneHistoryIndex_ >= 0
        && sketchSelectedPlaneHistoryIndex_ < historyList.size()) {
        sketchName = tr("草图(%1)").arg(historyList[sketchSelectedPlaneHistoryIndex_].name);
    }
    record.type = SKETCH;
    record.name = sketchName;
    record.timestamp = QDateTime::currentDateTime();
    record.actor = actor;
    record.color = color;
    record.param1 = 0;
    record.param2 = 0;
    record.param3 = 0;
    record.polyData = polyDataCopy;
    record.occShape = e;
    record.shapeWrapper = shapeWrapper;
    record.shapeDataSource = shapeDataSource;
    record.highlightFilter = highlightFilter;
    record.highlightActor = highlightActor;
    record.outlineActor = nullptr;

    historyList.append(record);
    activeSketchHistoryIndex_ = historyList.size() - 1;

    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    const gp_Ax3 sketchAx = activeSketchPlane_.Position();
    recipe.sketch.planeOrigin = sketchAx.Location();
    recipe.sketch.planeNormal = sketchAx.Direction();
    recipe.sketch.planeXDir = sketchAx.XDirection();
    if (sketchSelectedPlaneHistoryIndex_ >= 0) {
        recipe.sketch.datumPlaneIndex = sketchSelectedPlaneHistoryIndex_;
        recipe.parentIndices.append(sketchSelectedPlaneHistoryIndex_);
    }
    assignFeatureRecipe(activeSketchHistoryIndex_, recipe);

    updateFeatureTree();
    markDocumentModified(true);
}

void Widget::updateSketchHistoryShape()
{
    if (activeSketchHistoryIndex_ < 0 || activeSketchHistoryIndex_ >= historyList.size()) return;
    if (!hasActiveSketch_) return;

    const QList<TopoDS_Shape> geometries = activeSketch_.getGeometries();
    if (geometries.isEmpty()) return;

    // 复用现有 VIS 管线，但对曲线：保留 lines，使用线框显示
    TopoDS_Compound newShape;
    BRep_Builder builder;
    builder.MakeCompound(newShape);
    for (const TopoDS_Shape& geom : geometries) {
        if (!geom.IsNull()) {
            builder.Add(newShape, geom);
        }
    }
    if (newShape.IsNull()) return;

    ModelingHistory& history = historyList[activeSketchHistoryIndex_];
    history.occShape = newShape;
    history.type = SKETCH;

    BRepMesh_IncrementalMesh mesh(newShape, 0.05, Standard_False, 0.3, Standard_True);
    mesh.Perform();

    ++shapeIDCounter;
    Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(newShape);
    shapeWrapper->SetId(shapeIDCounter);

    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
        vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    shapeDataSource->SetShape(shapeWrapper);
    shapeDataSource->Modified();
    shapeDataSource->Update();
    // 草图主显示与高亮统一使用 shapeDataSource，避免圆弧在 Mesher 管线下偶发缺失
    vtkPolyData* meshPolyData = vtkPolyData::SafeDownCast(shapeDataSource->GetOutput());

    vtkSmartPointer<vtkPolyData> polyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    {
        // 参照直线显示思路：草图主显示统一转为显式折线，避免圆弧在可视化管线下丢失
        vtkSmartPointer<vtkPoints> displayPoints = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> displayLines = vtkSmartPointer<vtkCellArray>::New();
        for (const TopoDS_Shape& geom : geometries) {
            if (geom.IsNull() || geom.ShapeType() != TopAbs_EDGE) continue;
            const TopoDS_Edge edge = TopoDS::Edge(geom);

            Standard_Real f = 0.0, l = 0.0;
            Handle(Geom_Curve) c = BRep_Tool::Curve(edge, f, l);
            if (c.IsNull()) continue;

            int sampleCount = 2; // 直线保持两点
            try {
                GeomAdaptor_Curve ad(c, f, l);
                if (ad.GetType() != GeomAbs_Line) sampleCount = 48; // 圆弧/样条离散为折线
            } catch (...) {
                sampleCount = 32;
            }
            if (sampleCount < 2) sampleCount = 2;

            vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
            polyLine->GetPointIds()->SetNumberOfIds(sampleCount);

            for (int i = 0; i < sampleCount; ++i) {
                const double t = (sampleCount == 1)
                    ? f
                    : (f + (l - f) * static_cast<double>(i) / static_cast<double>(sampleCount - 1));
                const gp_Pnt p = c->Value(t);
                const vtkIdType pid = displayPoints->InsertNextPoint(p.X(), p.Y(), p.Z());
                polyLine->GetPointIds()->SetId(i, pid);
            }
            displayLines->InsertNextCell(polyLine);
        }

        if (displayPoints->GetNumberOfPoints() > 0 && displayLines->GetNumberOfCells() > 0) {
            polyDataCopy->SetPoints(displayPoints);
            polyDataCopy->SetLines(displayLines);
        } else if (meshPolyData && meshPolyData->GetNumberOfPoints() > 0) {
            polyDataCopy->ShallowCopy(meshPolyData);
        } else {
            return;
        }
    }

    if (history.actor) {
        vtkSmartPointer<vtkPolyDataMapper> newMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        newMapper->SetInputData(polyDataCopy);
        newMapper->ScalarVisibilityOff();
        // 草图线与参考平面共面：只用该 mapper 的相对线偏移，禁止改进程级全局参数
        //（全局 LineOffset 会污染实体轮廓深度测试）。
        newMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2.0, -2.0);
        history.actor->GetProperty()->SetRepresentationToWireframe();
        history.actor->GetProperty()->SetLineWidth(2.0);
        history.actor->GetProperty()->SetLighting(false);
        history.actor->GetProperty()->SetRenderLinesAsTubes(1);
        history.actor->GetProperty()->SetAmbient(1.0);
        history.actor->GetProperty()->SetDiffuse(0.0);
        history.actor->GetProperty()->SetSpecular(0.0);
        history.actor->GetProperty()->SetColor(history.color.redF(), history.color.greenF(), history.color.blueF());
        history.actor->SetPickable(true);

        IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, history.actor);
        // 注意：SetShapeSource 可能改写 mapper 连接，最后再强制回设显示 mapper
        history.actor->SetMapper(newMapper);
        history.actor->SetVisibility(1);
        if (renderer && !renderer->HasViewProp(history.actor)) {
            renderer->AddActor(history.actor);
        }
    }

    if (history.highlightFilter) {
        history.highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
        history.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
    }

    history.polyData = polyDataCopy;
    history.shapeWrapper = shapeWrapper;
    history.shapeDataSource = shapeDataSource;
    rebuildSketchCommittedOverlay();
}

void Widget::on_pushButton_5_clicked()
{
    clearSketchEditHover();
    endSketchBrushStroke();
    // 创建草图：弹出对话框，拾取平面作为草图平面
    auto* dlg = new SketchCreateDialog(dialogParentWidget());
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    activeSketchCreateDialog_ = dlg;

    connect(dlg, &SketchCreateDialog::requestPickPlane, this, [this]() {
        currentSelectionMode = SketchPlaneSelection;
        statusBar()->showMessage(tr("创建草图：请在模型上点击一个平面作为参考平面。"), 4000);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        if (!dlg->hasPickedPlane()) {
            QMessageBox::warning(this, tr("创建草图"), tr("请先拾取参考平面。"));
            return;
        }

        activeSketch_.clear();
        clearSketchCommittedOverlay();
        activeSketchPlane_ = dlg->pickedPlane();
        activeSketch_.setPlane(activeSketchPlane_);
        activeSketch_.setName(tr("草图"));
        hasActiveSketch_ = true;
        sketchClickCount_ = 0;
        activeSketchHistoryIndex_ = -1;

        ensureSketchHistoryRecord();
        updateSketchHistoryShape();

        alignViewToSketchPlane(activeSketchPlane_);

        currentSelectionMode = None;
        statusBar()->showMessage(tr("草图已创建。现在可点击“直线/圆弧”在该平面上绘制。"), 4000);
    });

    connect(dlg, &QDialog::rejected, this, [this]() {
        if (currentSelectionMode == SketchPlaneSelection) {
            currentSelectionMode = None;
        }
        activeSketchCreateDialog_ = nullptr;
    });

    connect(dlg, &QObject::destroyed, this, [this]() {
        if (activeSketchCreateDialog_) activeSketchCreateDialog_ = nullptr;
        if (currentSelectionMode == SketchPlaneSelection) currentSelectionMode = None;
    });

    // 当处于拾取模式时，鼠标点击会在 handleVtkMouseClick 中处理，这里只需显示对话框
    dlg->show();
}

void Widget::setupSketchCreationToggleButtons()
{
    QList<QPushButton*> btns = {
        ui->pushButton_7,
        ui->pushButton_41,
        ui->pushButton_40,
        ui->pushButton_42,
        ui->pushButton_11,
        ui->pushButton_12,
        ui->pushButton_13,
        ui->pushButton_9,
        ui->pushButton_10,
    };
    for (QPushButton* b : btns) {
        if (!b) continue;
        b->setCheckable(true);
        b->setStyleSheet(QStringLiteral(
            "QPushButton:checked { background-color: #9a9a9a; border: 1px solid #666666; }"));
    }
}

void Widget::uncheckAllSketchCreationButtons()
{
    QList<QPushButton*> btns = {
        ui->pushButton_7,
        ui->pushButton_41,
        ui->pushButton_40,
        ui->pushButton_42,
        ui->pushButton_11,
        ui->pushButton_12,
        ui->pushButton_13,
        ui->pushButton_9,
        ui->pushButton_10,
    };
    for (QPushButton* b : btns) {
        if (!b) continue;
        const QSignalBlocker blocker(b);
        b->setChecked(false);
    }
}

bool Widget::sketchCreationExitIfRepeatClick(QPushButton* clickedButton)
{
    if (sketchCreationExclusiveButton_ == clickedButton) {
        if (clickedButton == ui->pushButton_13) {
            closeSketchConicDialog();
            sketchCreationExclusiveButton_ = nullptr;
            uncheckAllSketchCreationButtons();
            if (vtkWidget && vtkWidget->renderWindow())
                vtkWidget->renderWindow()->Render();
            return true;
        }
        if (clickedButton == ui->pushButton_9) {
            closeSketchPolygonDialog();
            sketchCreationExclusiveButton_ = nullptr;
            uncheckAllSketchCreationButtons();
            if (vtkWidget && vtkWidget->renderWindow())
                vtkWidget->renderWindow()->Render();
            return true;
        }
        if (clickedButton == ui->pushButton_10) {
            closeSketchEllipseDialog();
            sketchCreationExclusiveButton_ = nullptr;
            uncheckAllSketchCreationButtons();
            if (vtkWidget && vtkWidget->renderWindow())
                vtkWidget->renderWindow()->Render();
            return true;
        }
        exitSketchCreationMode();
        return true;
    }
    return false;
}

void Widget::prepareSketchCreationToolClick(QPushButton* clickedButton)
{
    if (clickedButton != ui->pushButton_13)
        closeSketchConicDialog();
    if (clickedButton != ui->pushButton_9)
        closeSketchPolygonDialog();
    if (clickedButton != ui->pushButton_10)
        closeSketchEllipseDialog();
    uncheckAllSketchCreationButtons();
    sketchCreationExclusiveButton_ = clickedButton;
    if (clickedButton)
        clickedButton->setChecked(true);
}

void Widget::exitSketchCreationMode()
{
    closeSketchConicDialog();
    closeSketchEllipseDialog();
    if (currentSelectionMode == SketchDrawLine || currentSelectionMode == SketchDrawArc
        || currentSelectionMode == SketchDrawRectangle || currentSelectionMode == SketchDrawCircle
        || currentSelectionMode == SketchDrawPoint || currentSelectionMode == SketchDrawPolygon
        || currentSelectionMode == SketchPolygonPick || currentSelectionMode == SketchEllipsePick
        || currentSelectionMode == SketchEllipseAdjust) {
        currentSelectionMode = None;
        sketchClickCount_ = 0;
        sketchChainTangentValid_ = false;
        sketchContourChaining_ = false;
        closeSketchToolInput();
        closeSketchRectangleModeDialog();
        closeSketchCircleModeDialog();
        closeSketchPolygonDialog();
        clearSketchPreviewLine();
        clearSketchPreviewRectangle();
        clearSketchPreviewArc();
        clearSketchPreviewCircle();
        clearSketchPreviewConic();
        clearSketchPreviewPolygon();
        statusBar()->showMessage(tr("已退出草图几何创建。"), 3000);
    }
    sketchCreationExclusiveButton_ = nullptr;
    uncheckAllSketchCreationButtons();
    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::on_pushButton_7_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_7))
        return;
    prepareSketchCreationToolClick(ui->pushButton_7);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawLine;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = true;
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjLine);
    statusBar()->showMessage(tr("轮廓：在对话框中选择直线/圆弧与输入模式；上一段终点为下一段起点。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_40_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_40))
        return;
    prepareSketchCreationToolClick(ui->pushButton_40);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawLine;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjLine);
    statusBar()->showMessage(tr("直线：两次点击画一条线段；下一条需重新点起点。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_41_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_41))
        return;
    prepareSketchCreationToolClick(ui->pushButton_41);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawRectangle;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchCircleModeDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    openOrRaiseSketchRectangleModeDialog();
    statusBar()->showMessage(tr("长方体草图：按对话框选择矩形方法后在平面内取点；每次完成后需重新指定起点。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_42_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_42))
        return;
    prepareSketchCreationToolClick(ui->pushButton_42);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawArc;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjArc);
    statusBar()->showMessage(tr("圆弧：三点定弧；仅在「轮廓」链式绘制且存在上一段切向时，第二次点击可一步完成相切弧。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_11_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_11))
        return;
    prepareSketchCreationToolClick(ui->pushButton_11);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawCircle;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    openOrRaiseSketchCircleModeDialog();
    statusBar()->showMessage(tr("圆：圆心+半径或三点定圆；每次完成后需重新点击定义。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_12_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_12))
        return;
    prepareSketchCreationToolClick(ui->pushButton_12);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = SketchDrawPoint;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    statusBar()->showMessage(tr("草图点：在草图平面内单击创建十字标记。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_13_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_13))
        return;
    prepareSketchCreationToolClick(ui->pushButton_13);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    currentSelectionMode = None;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    openOrRaiseSketchConicDialog();
    statusBar()->showMessage(tr("二次曲线：在对话框中拾取起点、终点与控制点，设置 Rho 后点击应用。"), 6000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_9_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_9))
        return;
    prepareSketchCreationToolClick(ui->pushButton_9);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    sketchPolygonHasCenter_ = false;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    closeSketchConicDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    clearSketchPreviewPolygon();
    openOrRaiseSketchPolygonDialog();
    statusBar()->showMessage(tr("多边形：指定中心点与大小；可锁定半径/长度或旋转。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::on_pushButton_10_clicked()
{
    if (sketchCreationExitIfRepeatClick(ui->pushButton_10))
        return;
    prepareSketchCreationToolClick(ui->pushButton_10);
    clearSketchEditHover();
    endSketchBrushStroke();
    ensureDefaultSketchForTools();
    sketchClickCount_ = 0;
    sketchEllipseHasCenter_ = false;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    closeSketchConicDialog();
    closeSketchPolygonDialog();
    clearSketchPreviewArc();
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    clearSketchPreviewPolygon();
    openOrRaiseSketchEllipseDialog();
    statusBar()->showMessage(tr("椭圆：指定中心点即可创建；可拾取大/小半径点或通过数值修改。"), 5000);
    if (vtkWidget) vtkWidget->setFocus();
}

class SketchEditCommand : public Command {
public:
    SketchEditCommand(Widget* w, const QList<TopoDS_Shape>& before, const QList<TopoDS_Shape>& after, const QString& desc)
        : before_(before), after_(after), desc_(desc)
    {
        widget = w;
    }

    void execute() override
    {
        if (!widget) return;
        widget->activeSketch_.setGeometries(after_);
        widget->ensureSketchHistoryRecord();
        widget->updateSketchHistoryShape();
        widget->markDocumentModified(true);
    }

    void undo() override
    {
        if (!widget) return;
        widget->activeSketch_.setGeometries(before_);
        widget->ensureSketchHistoryRecord();
        widget->updateSketchHistoryShape();
        widget->markDocumentModified(true);
    }

    QString getDescription() const override { return desc_; }

private:
    QList<TopoDS_Shape> before_;
    QList<TopoDS_Shape> after_;
    QString desc_;
};

bool Widget::isSketchEditMode(SelectionMode mode) const
{
    return mode == SketchQuickTrim || mode == SketchQuickExtend;
}

void Widget::startSketchQuickTrim()
{
    exitSketchCreationMode();
    ensureDefaultSketchForTools();
    if (hasActiveSketch_ && activeSketch_.isValid()) {
        ensureSketchHistoryRecord();
        if (!activeSketch_.getGeometries().isEmpty()) {
            updateSketchHistoryShape();
        }
    }
    sketchClickCount_ = 0;
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewArc();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchEditHover();
    currentSelectionMode = SketchQuickTrim;
    statusBar()->showMessage(tr("快速修剪：单击修剪，按住左键拖动可画笔批量修剪（快捷键 T）。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::startSketchQuickExtend()
{
    exitSketchCreationMode();
    ensureDefaultSketchForTools();
    if (hasActiveSketch_ && activeSketch_.isValid()) {
        ensureSketchHistoryRecord();
        if (!activeSketch_.getGeometries().isEmpty()) {
            updateSketchHistoryShape();
        }
    }
    sketchClickCount_ = 0;
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewArc();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchEditHover();
    currentSelectionMode = SketchQuickExtend;
    statusBar()->showMessage(tr("快速延伸：单击靠近端点延伸，按住左键拖动可画笔批量延伸（快捷键 E）。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::beginSketchBrushStroke()
{
    if (!isSketchEditMode(currentSelectionMode)) return;
    sketchBrushActive_ = true;
    sketchLastBrushShape_.Nullify();
}

void Widget::endSketchBrushStroke()
{
    sketchBrushActive_ = false;
    sketchLastBrushShape_.Nullify();
}

void Widget::clearSketchEditHover()
{
    if (sketchEditHoverActor_ && renderer) {
        removeSceneActor(sketchEditHoverActor_);
        sketchEditHoverActor_ = nullptr;
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

namespace {

Handle(Geom_Curve) sketchBasisCurve(Handle(Geom_Curve) curve)
{
    while (!curve.IsNull() && curve->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        curve = Handle(Geom_TrimmedCurve)::DownCast(curve)->BasisCurve();
    }
    return curve;
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

} // namespace

bool Widget::tryPickSketchEdgeAt(int x, int y, TopoDS_Edge& outEdge, gp_Pnt* outWorldPoint, double* outCurveParam)
{
    if (!shapePicker || !renderer) return false;
    if (activeSketchHistoryIndex_ < 0 || activeSketchHistoryIndex_ >= historyList.size()) return false;
    ModelingHistory& sketchRec = historyList[activeSketchHistoryIndex_];
    if (!sketchRec.actor || sketchRec.actor->GetVisibility() == 0 || !sketchRec.shapeDataSource) return false;

    IVtkTools_ShapeObject::SetShapeSource(sketchRec.shapeDataSource, sketchRec.actor);
    sketchRec.shapeDataSource->Modified();
    sketchRec.shapeDataSource->Update();

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);

    bool hasWorldPoint = false;
    if (outWorldPoint) {
        gp_Pnt p;
        if (tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            *outWorldPoint = p;
            hasWorldPoint = true;
        }
    }

    if (sketchRec.shapeWrapper.IsNull()) return false;
    const IVtk_IdType shapeId = sketchRec.shapeWrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeId);
    if (subIds.IsEmpty()) return false;

    const QList<TopoDS_Shape> sketchGeoms = activeSketch_.getGeometries();
    TopoDS_Edge bestEdge;
    double bestDist = std::numeric_limits<double>::max();
    double bestParam = 0.0;

    for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
        const TopoDS_Shape sh = sketchRec.shapeWrapper->GetSubShape(it.Value());
        if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
        const TopoDS_Edge candidate = TopoDS::Edge(sh);
        const gp_Pnt refPoint = hasWorldPoint ? *outWorldPoint : gp_Pnt();
        const double dist = hasWorldPoint
            ? distancePointToEdge(refPoint, candidate)
            : 0.0;
        if (!hasWorldPoint || dist < bestDist) {
            bestDist = dist;
            bestEdge = candidate;
            if (hasWorldPoint) {
                Standard_Real f = 0.0, l = 0.0;
                Handle(Geom_Curve) c = BRep_Tool::Curve(candidate, f, l);
                if (!c.IsNull()) {
                    const double lo = qMin(f, l);
                    const double hi = qMax(f, l);
                    GeomAPI_ProjectPointOnCurve proj(refPoint, c);
                    if (proj.NbPoints() > 0) {
                        bestParam = qBound(lo, proj.LowerDistanceParameter(), hi);
                    }
                }
            }
        }
    }

    if (bestEdge.IsNull()) return false;

    const int idx = findSketchEdgeIndex(sketchGeoms, bestEdge);
    outEdge = (idx >= 0) ? TopoDS::Edge(sketchGeoms[idx]) : bestEdge;

    if (outWorldPoint && !hasWorldPoint) {
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) c = BRep_Tool::Curve(outEdge, f, l);
        if (!c.IsNull()) {
            *outWorldPoint = c->Value((f + l) * 0.5);
        }
    }
    if (outCurveParam) {
        *outCurveParam = bestParam;
    }
    return true;
}

void Widget::handleSketchEditHover(int x, int y)
{
    TopoDS_Edge picked;
    if (!tryPickSketchEdgeAt(x, y, picked)) {
        clearSketchEditHover();
        return;
    }
    if (sketchEditHoverActor_ && renderer) {
        removeSceneActor(sketchEditHoverActor_);
        sketchEditHoverActor_ = nullptr;
    }
    sketchEditHoverActor_ = buildSnapShapeHighlightActor(
        picked, 1.0, 0.75, 0.0, 0.95, 4.0
    );
    if (sketchEditHoverActor_ && renderer) {
        addAppearanceActor(sketchEditHoverActor_);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::handleSketchEditClick(int x, int y, bool brushMode)
{
    TopoDS_Edge picked;
    gp_Pnt clickPoint;
    double clickParam = 0.0;
    if (!tryPickSketchEdgeAt(x, y, picked, &clickPoint, &clickParam)) return;
    if (brushMode && !sketchLastBrushShape_.IsNull() && sketchLastBrushShape_.IsSame(picked)) {
        return;
    }

    bool changed = false;
    if (currentSelectionMode == SketchQuickTrim) {
        changed = applyQuickTrimAt(picked, clickPoint);
    } else if (currentSelectionMode == SketchQuickExtend) {
        changed = applyQuickExtendAt(picked, clickPoint);
    }

    if (changed) {
        sketchLastBrushShape_ = picked;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    } else if (statusBar()) {
        if (currentSelectionMode == SketchQuickTrim) {
            statusBar()->showMessage(tr("修剪失败：未找到相交边界，或点击段过短。"), 2500);
        } else {
            statusBar()->showMessage(tr("延伸失败：未找到可延伸到的相交曲线。"), 2500);
        }
    }
}

bool Widget::replaceSketchEdgeWith(const TopoDS_Edge& targetEdge, const QList<TopoDS_Edge>& replacements, const QString& actionName)
{
    if (!hasActiveSketch_) return false;
    // 修改几何前先移除悬停高亮，避免引用旧 polydata/旧 edge 造成渲染崩溃
    clearSketchEditHover();
    const QList<TopoDS_Shape> before = activeSketch_.getGeometries();
    if (before.isEmpty()) return false;

    const int removeIdx = findSketchEdgeIndex(before, targetEdge);
    if (removeIdx < 0) return false;

    QList<TopoDS_Shape> after;
    after.reserve(before.size() + replacements.size());
    for (int i = 0; i < before.size(); ++i) {
        if (i == removeIdx) continue;
        after.append(before[i]);
    }

    for (const TopoDS_Edge& e : replacements) {
        if (!e.IsNull()) after.append(e);
    }

    if (after.isEmpty()) return false;
    executeCommand(new SketchEditCommand(this, before, after, actionName));
    return true;
}

bool Widget::applyQuickTrimAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint)
{
    try {
        if (targetEdge.IsNull()) return false;
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) targetCurve = BRep_Tool::Curve(targetEdge, f, l);
        if (targetCurve.IsNull()) return false;
        const double lo = qMin(f, l);
        const double hi = qMax(f, l);

        GeomAPI_ProjectPointOnCurve projClick(clickPoint, targetCurve);
        if (projClick.NbPoints() == 0) return false;
        const double tClick = qBound(lo, projClick.LowerDistanceParameter(), hi);

        auto isSameSupportCurve = [](const Handle(Geom_Curve)& a, const Handle(Geom_Curve)& b) -> bool {
            if (a.IsNull() || b.IsNull()) return false;
            if (a == b) return true;

            if (a->IsKind(STANDARD_TYPE(Geom_Line)) && b->IsKind(STANDARD_TYPE(Geom_Line))) {
                const gp_Lin la = Handle(Geom_Line)::DownCast(a)->Lin();
                const gp_Lin lb = Handle(Geom_Line)::DownCast(b)->Lin();
                const double dot = std::abs(la.Direction().Dot(lb.Direction()));
                if (dot < 1.0 - 1.0e-9) return false;
                // 两条线方向平行且彼此距离接近 0，则视作同一支撑直线（重合/共线）
                return la.Distance(lb.Location()) <= 1.0e-7;
            }

            if (a->IsKind(STANDARD_TYPE(Geom_Circle)) && b->IsKind(STANDARD_TYPE(Geom_Circle))) {
                const gp_Circ ca = Handle(Geom_Circle)::DownCast(a)->Circ();
                const gp_Circ cb = Handle(Geom_Circle)::DownCast(b)->Circ();
                const double rDiff = std::abs(ca.Radius() - cb.Radius());
                if (rDiff > 1.0e-7) return false;
                const double centerDist = ca.Location().Distance(cb.Location());
                if (centerDist > 1.0e-7) return false;
                const double axisDot = std::abs(ca.Axis().Direction().Dot(cb.Axis().Direction()));
                return axisDot >= 1.0 - 1.0e-9;
            }

            return false;
        };
        const Handle(Geom_Curve) targetBasis = sketchBasisCurve(targetCurve);

        QList<double> hits;
        for (const TopoDS_Shape& sh : activeSketch_.getGeometries()) {
            if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
            if (sketchEdgesEquivalent(TopoDS::Edge(sh), targetEdge)) continue;
            Standard_Real bf = 0.0, bl = 0.0;
            Handle(Geom_Curve) bCurve = BRep_Tool::Curve(TopoDS::Edge(sh), bf, bl);
            if (bCurve.IsNull()) continue;
            const Handle(Geom_Curve) bBasis = sketchBasisCurve(bCurve);
            // 跳过“同一支撑曲线”的边（例如第一次修剪后产生的共线兄弟线段），
            // 可避免 GeomAPI_ExtremaCurveCurve 在重合/退化情形下触发底层崩溃。
            if (isSameSupportCurve(targetBasis, bBasis)) continue;

            try {
                GeomAPI_ExtremaCurveCurve inter(targetCurve, bCurve);
                if (inter.NbExtrema() < 1) continue;
                for (int i = 1; i <= inter.NbExtrema(); ++i) {
                    gp_Pnt p1, p2;
                    inter.Points(i, p1, p2);
                    if (p1.Distance(p2) > 1.0e-6) continue;
                    GeomAPI_ProjectPointOnCurve proj(p1, targetCurve);
                    if (proj.NbPoints() < 1) continue;
                    const double t = proj.LowerDistanceParameter();
                    if (t >= lo + 1.0e-8 && t <= hi - 1.0e-8) {
                        hits.append(t);
                    }
                }
            } catch (...) {
                continue;
            }
        }

        if (hits.isEmpty()) return false;

        double bestT = hits.first();
        double bestDist = std::abs(bestT - tClick);
        for (double t : hits) {
            const double d = std::abs(t - tClick);
            if (d < bestDist) {
                bestDist = d;
                bestT = t;
            }
        }
        if (bestDist <= 1.0e-7) return false;

        const double cutA = qMin(bestT, tClick);
        const double cutB = qMax(bestT, tClick);
        QList<TopoDS_Edge> kept;
        if (cutA - lo > 1.0e-7) {
            BRepBuilderAPI_MakeEdge mkA(targetCurve, lo, cutA);
            if (mkA.IsDone() && !mkA.Edge().IsNull()) kept.append(mkA.Edge());
        }
        if (hi - cutB > 1.0e-7) {
            BRepBuilderAPI_MakeEdge mkB(targetCurve, cutB, hi);
            if (mkB.IsDone() && !mkB.Edge().IsNull()) kept.append(mkB.Edge());
        }
        if (kept.isEmpty()) return false;

        return replaceSketchEdgeWith(targetEdge, kept, tr("快速修剪"));
    } catch (...) {
        return false;
    }
}

bool Widget::applyQuickExtendAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint)
{
    try {
        if (targetEdge.IsNull()) return false;
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) targetCurve = BRep_Tool::Curve(targetEdge, f, l);
        if (targetCurve.IsNull()) return false;

        const double lo = qMin(f, l);
        const double hi = qMax(f, l);
        const gp_Pnt pStart = targetCurve->Value(lo);
        const gp_Pnt pEnd = targetCurve->Value(hi);
        const bool extendStart = (pStart.Distance(clickPoint) <= pEnd.Distance(clickPoint));
        const double refParam = extendStart ? lo : hi;

        gp_Pnt pRef;
        gp_Vec d1;
        targetCurve->D1(refParam, pRef, d1);
        if (d1.Magnitude() <= Precision::Confusion()) return false;
        const gp_Dir tangent = extendStart ? gp_Dir(-d1.X(), -d1.Y(), -d1.Z()) : gp_Dir(d1);

        bool found = false;
        double bestParam = refParam;
        double bestDist = std::numeric_limits<double>::max();

        for (const TopoDS_Shape& sh : activeSketch_.getGeometries()) {
            if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
            if (sketchEdgesEquivalent(TopoDS::Edge(sh), targetEdge)) continue;
            Standard_Real bf = 0.0, bl = 0.0;
            Handle(Geom_Curve) bCurve = BRep_Tool::Curve(TopoDS::Edge(sh), bf, bl);
            if (bCurve.IsNull()) continue;

            try {
                GeomAPI_ExtremaCurveCurve inter(targetCurve, bCurve);
                if (inter.NbExtrema() < 1) continue;
                for (int i = 1; i <= inter.NbExtrema(); ++i) {
                    gp_Pnt p1, p2;
                    inter.Points(i, p1, p2);
                    if (p1.Distance(p2) > 1.0e-6) continue;
                    const gp_Pnt ip((p1.X() + p2.X()) * 0.5, (p1.Y() + p2.Y()) * 0.5, (p1.Z() + p2.Z()) * 0.5);

                    GeomAPI_ProjectPointOnCurve pb(ip, bCurve);
                    if (pb.NbPoints() < 1) continue;
                    const double tb = pb.LowerDistanceParameter();
                    const double bLo = qMin(bf, bl);
                    const double bHi = qMax(bf, bl);
                    if (tb < bLo - 1.0e-7 || tb > bHi + 1.0e-7) continue;

                    GeomAPI_ProjectPointOnCurve pt(ip, targetCurve);
                    if (pt.NbPoints() < 1) continue;
                    const double tt = pt.LowerDistanceParameter();
                    if ((!extendStart && tt <= hi + 1.0e-7) || (extendStart && tt >= lo - 1.0e-7)) continue;

                    gp_Vec dirToHit(pRef, ip);
                    if (dirToHit.Magnitude() <= Precision::Confusion()) continue;
                    if (dirToHit.Dot(gp_Vec(tangent)) <= 0.0) continue;

                    const double d = pRef.Distance(ip);
                    if (d < bestDist) {
                        bestDist = d;
                        bestParam = tt;
                        found = true;
                    }
                }
            } catch (...) {
                continue;
            }
        }
        if (!found) return false;

        const double newLo = extendStart ? bestParam : lo;
        const double newHi = extendStart ? hi : bestParam;
        if (newHi - newLo <= 1.0e-7) return false;

        QList<TopoDS_Edge> repl;
        BRepBuilderAPI_MakeEdge mk(targetCurve, newLo, newHi);
        if (!mk.IsDone() || mk.Edge().IsNull()) return false;
        repl.append(mk.Edge());
        return replaceSketchEdgeWith(targetEdge, repl, tr("快速延伸"));
    } catch (...) {
        return false;
    }
}

void Widget::wireSketchTabStackedPages()
{
    if (!ui->stackedWidget || !ui->stackedWidget_2)
        return;
    if (!ui->toolButton_sketchCurveStack_prev || !ui->toolButton_sketchCurveStack_next
        || !ui->toolButton_sketchEditStack_prev || !ui->toolButton_sketchEditStack_next)
        return;

    const QSize iconSz(14, 14);
    const QSize btnSz(20, 20);
    const QIcon icoPrev = monochromeStandardIcon(this, QStyle::SP_ArrowUp, iconSz);
    const QIcon icoNext = monochromeStandardIcon(this, QStyle::SP_ArrowDown, iconSz);

    ui->toolButton_sketchCurveStack_prev->setText(QString());
    ui->toolButton_sketchCurveStack_prev->setIcon(icoPrev);
    ui->toolButton_sketchCurveStack_prev->setIconSize(iconSz);
    ui->toolButton_sketchCurveStack_prev->setFixedSize(btnSz);
    ui->toolButton_sketchCurveStack_prev->setToolTip(tr("上一页"));
    ui->toolButton_sketchCurveStack_prev->setAutoRaise(true);

    ui->toolButton_sketchCurveStack_next->setText(QString());
    ui->toolButton_sketchCurveStack_next->setIcon(icoNext);
    ui->toolButton_sketchCurveStack_next->setIconSize(iconSz);
    ui->toolButton_sketchCurveStack_next->setFixedSize(btnSz);
    ui->toolButton_sketchCurveStack_next->setToolTip(tr("下一页"));
    ui->toolButton_sketchCurveStack_next->setAutoRaise(true);

    ui->toolButton_sketchEditStack_prev->setText(QString());
    ui->toolButton_sketchEditStack_prev->setIcon(icoPrev);
    ui->toolButton_sketchEditStack_prev->setIconSize(iconSz);
    ui->toolButton_sketchEditStack_prev->setFixedSize(btnSz);
    ui->toolButton_sketchEditStack_prev->setToolTip(tr("上一页"));
    ui->toolButton_sketchEditStack_prev->setAutoRaise(true);

    ui->toolButton_sketchEditStack_next->setText(QString());
    ui->toolButton_sketchEditStack_next->setIcon(icoNext);
    ui->toolButton_sketchEditStack_next->setIconSize(iconSz);
    ui->toolButton_sketchEditStack_next->setFixedSize(btnSz);
    ui->toolButton_sketchEditStack_next->setToolTip(tr("下一页"));
    ui->toolButton_sketchEditStack_next->setAutoRaise(true);

    connect(ui->toolButton_sketchCurveStack_prev, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget, -1);
    });
    connect(ui->toolButton_sketchCurveStack_next, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget, +1);
    });
    connect(ui->toolButton_sketchEditStack_prev, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget_2, -1);
    });
    connect(ui->toolButton_sketchEditStack_next, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget_2, +1);
    });
}
