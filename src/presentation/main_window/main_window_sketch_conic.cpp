// Sketch conic and shared geometry helpers (split from main_window.cpp)
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
