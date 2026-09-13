// Sketch polygon and ellipse helpers (split from main_window.cpp)
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
