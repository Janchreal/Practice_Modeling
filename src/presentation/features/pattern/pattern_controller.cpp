// 阵列特征：对话框、体选择、预览与执行
#include "main_window.h"
#include "geometry/pattern/pattern_geometry.h"
#include "presentation/dialogs/pattern/pattern_feature_dialog.h"
#include "application/commands/patterncommand.h"
#include "rendering/model/model_display_style.h"
#include "ui_main_window.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>

#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>

#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

constexpr double kDegToRad = M_PI / 180.0;

namespace {

QList<TopoDS_Shape> collectPatternSourceShapes(const QList<ModelingHistory>& historyList,
                                               const QList<int>& indices,
                                               const ModelGeometryStore& geometryStore)
{
    QList<TopoDS_Shape> shapes;
    for (int index : indices) {
        if (index < 0 || index >= historyList.size()) {
            continue;
        }
        const ModelGeometryState* state = geometryStore.find(historyList[index].id);
        if (state && !state->occShape.IsNull()) {
            shapes.append(state->occShape);
        }
    }
    return shapes;
}

} // namespace

void Widget::on_patternFeature_clicked()
{
    if (historyList.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("当前没有可阵列的模型，请先创建几何体。"));
        return;
    }

    patternSelectedIndices_.clear();
    clearPatternSelectionHighlight();
    clearPatternPreview();
    stopPatternPitchInteractive();
    patternVectorPick_ = PatternVectorPick::None;

    PatternFeatureDialog* dialog = new PatternFeatureDialog(dialogParentWidget());
    patternDialog_ = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    currentSelectionMode = PatternBodySelection;
    dialog->setSelectedBodyCount(0);

    connect(dialog, &PatternFeatureDialog::requestBodySelection, this, [this]() {
        currentSelectionMode = PatternBodySelection;
        if (vtkWidget) vtkWidget->setFocus();
    });
    connect(dialog, &PatternFeatureDialog::previewRequested, this, [this]() {
        updatePatternPreview();
    });
    connect(dialog, &PatternFeatureDialog::applyRequested, this, [this, dialog]() {
        performPattern(dialog);
        updatePatternPreview();
    });
    connect(dialog, &PatternFeatureDialog::parametersChanged, this, [this]() {
        updatePatternPreview();
        if (patternDialog_ && patternDialog_->hasDirection1()
            && patternDialog_->layoutType() != PatternLayoutType::Linear) {
            updatePatternRotationAxisArrow();
        }
        if (!patternDialog_ || !patternDialog_->hasDirection1()) {
            return;
        }
        if (patternDialog_->layoutType() != PatternLayoutType::Linear) {
            startPatternPitchInteractive(0);
            return;
        }
        if (patternDialog_->useDirection2() && patternDialog_->hasDirection2()) {
            startPatternPitchInteractive(1);
        } else {
            startPatternPitchInteractive(0);
        }
    });
    connect(dialog, &PatternFeatureDialog::pitch1Changed, this, [this](double) {
        if (patternPitchInteractiveActive_ && patternActivePitchAxis_ == 0) {
            updatePatternPreview();
        }
    });
    connect(dialog, &PatternFeatureDialog::pitch2Changed, this, [this](double) {
        if (patternPitchInteractiveActive_ && patternActivePitchAxis_ == 1) {
            updatePatternPreview();
        }
    });
    connect(dialog, &PatternFeatureDialog::requestVectorMode, this, [this](int dirIndex, int modeIndex) {
        patternVectorPick_ = (dirIndex == 2) ? PatternVectorPick::Direction2 : PatternVectorPick::Direction1;
        clearVectorDialogArrowPreview();
        clearPatternPitchGizmoOnly();
        openVectorDialog(modeIndex);
    });
    connect(dialog, &PatternFeatureDialog::requestOpenVectorDialog, this, [this](int dirIndex) {
        patternVectorPick_ = (dirIndex == 2) ? PatternVectorPick::Direction2 : PatternVectorPick::Direction1;
        clearVectorDialogArrowPreview();
        clearPatternPitchGizmoOnly();
        currentSelectionMode = VectorDialogPickDirection;
        openVectorDialog();
    });
    connect(dialog, &PatternFeatureDialog::requestPointSelection, this, [this]() {
        startPatternPointSelection(kPatternPointSnapArbitrary);
    });
    connect(dialog, &PatternFeatureDialog::requestPointSelectionWithSnap, this, [this](int snapKind) {
        startPatternPointSelection(snapKind);
    });

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        performPattern(dialog);
        stopPatternInteractiveCleanup();
    });
    connect(dialog, &QDialog::rejected, this, [this]() {
        stopPatternInteractiveCleanup();
    });
    connect(dialog, &QObject::destroyed, this, [this]() {
        patternDialog_ = nullptr;
    });

    dialog->show();
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::stopPatternInteractiveCleanup()
{
    patternSelectedIndices_.clear();
    clearPatternSelectionHighlight();
    clearPatternPreview();
    stopPatternPitchInteractive();
    patternVectorPick_ = PatternVectorPick::None;
    clearVectorDialogArrowPreview();
    clearSelectedPoint();
    if (currentSelectionMode == PatternBodySelection || currentSelectionMode == PatternPitchInteractive) {
        currentSelectionMode = None;
    }
    patternDialog_ = nullptr;
}

void Widget::handlePatternBodySelection(vtkActor* selectedActor)
{
    if (!selectedActor || currentSelectionMode != PatternBodySelection) {
        return;
    }
    const int idx = resolveHistoryIndexByActor(selectedActor);
    if (idx < 0) {
        return;
    }
    if (patternSelectedIndices_.contains(idx)) {
        patternSelectedIndices_.removeOne(idx);
    } else {
        patternSelectedIndices_.append(idx);
    }
    updatePatternSelectionHighlight();
    if (patternDialog_) {
        patternDialog_->setSelectedBodyCount(patternSelectedIndices_.size());
    }
    updatePatternPreview();
}

void Widget::updatePatternSelectionHighlight()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (!renderStateFor(historyList[i]).actor) continue;
        ModelDisplayStyle::restoreModelAppearance(
            renderStateFor(historyList[i]).actor->GetProperty(),
            historyList[i].type,
            historyList[i].color);
    }
    for (int index : patternSelectedIndices_) {
        if (index >= 0 && index < historyList.size() && renderStateFor(historyList[index]).actor) {
            ModelDisplayStyle::applyModelColorOpacity(
                renderStateFor(historyList[index]).actor->GetProperty(),
                QColor::fromRgbF(0.8, 0.8, 0.2),
                1.0);
        }
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearPatternSelectionHighlight()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (!renderStateFor(historyList[i]).actor) continue;
        ModelDisplayStyle::restoreModelAppearance(
            renderStateFor(historyList[i]).actor->GetProperty(),
            historyList[i].type,
            historyList[i].color);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::restorePatternPitchInteractiveAfterOriginPick()
{
    if (!patternDialog_ || !patternDialog_->hasDirection1()) {
        currentSelectionMode = PatternBodySelection;
        updatePatternPreview();
        return;
    }
    const int axis = (patternDialog_->useDirection2() && patternDialog_->hasDirection2()
                        && patternActivePitchAxis_ == 1)
                           ? 1
                           : 0;
    startPatternPitchInteractive(axis);
    updatePatternPreview();
}

void Widget::startPatternPointSelection(int snapKind)
{
    if (!patternDialog_) {
        return;
    }
    if (snapKind == kPatternPointSnapArbitrary) {
        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        currentSelectionMode = PointSelection;
        if (statusBar()) {
            statusBar()->showMessage(tr("请在3D视图中点击模型上的点作为阵列旋转中心（任意点）。"), 4000);
        }
    } else {
        startOriginSnapSelection(OriginDialogKind::Pattern, snapKind);
    }
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

double Widget::patternPitchInteractiveValue(int axisIndex) const
{
    if (!patternDialog_) {
        return 0.0;
    }
    if (axisIndex == 0 && patternDialog_->layoutType() == PatternLayoutType::Polygonal) {
        return patternDialog_->polygonSpanDegrees();
    }
    return (axisIndex == 0) ? patternDialog_->pitch1() : patternDialog_->pitch2();
}

void Widget::applyPatternPitchInteractiveValue(double value, int axisIndex)
{
    if (!patternDialog_) {
        return;
    }
    if (axisIndex == 0 && patternDialog_->layoutType() == PatternLayoutType::Polygonal) {
        patternDialog_->setPolygonSpan(value);
        return;
    }
    if (axisIndex == 0) {
        patternDialog_->setPitch1(value);
    } else {
        patternDialog_->setPitch2(value);
    }
}

void Widget::updatePatternRotationAxisArrow()
{
    if (!patternDialog_ || !renderer || !patternDialog_->hasDirection1()) {
        return;
    }
    const gp_Pnt origin = resolvePatternOrigin(patternDialog_, patternSelectedIndices_);
    updateVectorDialogArrow(patternDialog_->direction1(), origin);
}

void Widget::applyPatternVectorPick(const gp_Dir& dir)
{
    if (!patternDialog_) return;

    patternPitchDragActive_ = false;
    clearPatternPitchGizmoOnly();

    if (patternVectorPick_ == PatternVectorPick::Direction1) {
        patternDialog_->setDirection1(dir, true);
        patternActivePitchAxis_ = 0;
    } else if (patternVectorPick_ == PatternVectorPick::Direction2) {
        patternDialog_->setDirection2(dir, true);
        patternActivePitchAxis_ = 1;
    } else {
        return;
    }

    if (patternDialog_->layoutType() != PatternLayoutType::Linear) {
        updatePatternRotationAxisArrow();
    } else {
        clearVectorDialogArrowPreview();
    }

    updatePatternPreview();
    startPatternPitchInteractive(patternActivePitchAxis_);

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

gp_Pnt Widget::computePatternArrayOriginForIndices(const QList<int>& indices) const
{
    Bnd_Box box;
    box.SetGap(0.0);
    bool hasBox = false;
    for (int idx : indices) {
        if (idx < 0 || idx >= historyList.size()) continue;
        const TopoDS_Shape& s = geometryStateFor(historyList[idx]).occShape;
        if (s.IsNull()) continue;
        BRepBndLib::Add(s, box);
        hasBox = true;
    }
    if (!hasBox) {
        return gp_Pnt(0, 0, 0);
    }
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return gp_Pnt((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
}

gp_Pnt Widget::resolvePatternOrigin(PatternFeatureDialog* dialog, const QList<int>& indices) const
{
    if (dialog && dialog->hasRotationCenter()) {
        return dialog->rotationCenter();
    }
    return computePatternArrayOriginForIndices(indices);
}

gp_Pnt Widget::computePatternArrayOrigin() const
{
    if (patternDialog_) {
        return resolvePatternOrigin(patternDialog_, patternSelectedIndices_);
    }
    return computePatternArrayOriginForIndices(patternSelectedIndices_);
}

PatternLayoutType Widget::patternLayoutType() const
{
    return patternDialog_ ? patternDialog_->layoutType() : PatternLayoutType::Linear;
}

bool Widget::patternAxisUsesAngularPitch(int axisIndex) const
{
    if (axisIndex != 0) {
        return false;
    }
    const PatternLayoutType layout = patternLayoutType();
    return layout == PatternLayoutType::Circular || layout == PatternLayoutType::Polygonal;
}

double Widget::patternGizmoSpanLength(const gp_Dir& axisDir) const
{
    Bnd_Box box;
    box.SetGap(0.0);
    bool hasBox = false;
    for (int idx : patternSelectedIndices_) {
        if (idx < 0 || idx >= historyList.size()) continue;
        const TopoDS_Shape& s = geometryStateFor(historyList[idx]).occShape;
        if (s.IsNull()) continue;
        BRepBndLib::Add(s, box);
        hasBox = true;
    }
    if (!hasBox) {
        return 10.0;
    }
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const gp_Pnt corners[8] = {
        gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymin, zmin), gp_Pnt(xmin, ymax, zmin), gp_Pnt(xmax, ymax, zmin),
        gp_Pnt(xmin, ymin, zmax), gp_Pnt(xmax, ymin, zmax), gp_Pnt(xmin, ymax, zmax), gp_Pnt(xmax, ymax, zmax)
    };
    const gp_Pnt origin = computePatternArrayOrigin();
    double maxRadial = 1.0;
    for (const gp_Pnt& corner : corners) {
        gp_Vec radial(gp_Vec(origin, corner));
        const double axial = radial.Dot(gp_Vec(axisDir));
        radial -= gp_Vec(axisDir) * axial;
        maxRadial = std::max(maxRadial, radial.Magnitude());
    }
    return maxRadial;
}

void Widget::computePatternGizmoSegment(gp_Pnt& origin, gp_Pnt& tip, int axisIndex) const
{
    origin = patternArrayOrigin_;
    if (!patternDialog_) {
        tip = origin;
        return;
    }

    const gp_Dir dir = patternDirection(axisIndex);
    const double pitch = (axisIndex == 0) ? patternDialog_->pitch1() : patternDialog_->pitch2();
    const int count = (axisIndex == 0) ? patternDialog_->count1() : patternDialog_->count2();
    const int span = std::max(1, count - 1);

    if (patternAxisUsesAngularPitch(axisIndex)) {
        const gp_Dir axisDir = patternDialog_->direction1();
        gp_Dir refDir = dir;
        if (axisIndex == 0) {
            if (patternDialog_->layoutType() == PatternLayoutType::Linear
                && patternDialog_->useDirection2() && patternDialog_->hasDirection2()) {
                refDir = PatternGeometry::perpendicularDirection(axisDir, patternDialog_->direction2());
            } else {
                refDir = PatternGeometry::perpendicularDirection(axisDir, gp_Dir(1, 0, 0));
            }
        }
        const double radius = patternGizmoSpanLength(axisDir);
        gp_Trsf rot;
        rot.SetRotation(gp_Ax1(origin, axisDir), span * pitch * kDegToRad);
        const gp_Pnt refPt = origin.Translated(gp_Vec(refDir) * radius);
        tip = refPt.Transformed(rot);
        return;
    }

    tip = origin.Translated(gp_Vec(dir) * (pitch * span));
}

gp_Dir Widget::patternGizmoArrowDirection(const gp_Pnt& origin, const gp_Pnt& tip, int axisIndex) const
{
    if (!patternDialog_) {
        return gp_Dir(1, 0, 0);
    }
    if (patternAxisUsesAngularPitch(axisIndex)) {
        const gp_Dir axisDir = patternDialog_->direction1();
        gp_Vec radial(origin, tip);
        if (radial.Magnitude() < Precision::Confusion()) {
            radial = gp_Vec(PatternGeometry::perpendicularDirection(axisDir, gp_Dir(1, 0, 0)));
        }
        gp_Vec tangent = gp_Vec(axisDir).Crossed(radial);
        if (tangent.Magnitude() < Precision::Confusion()) {
            return PatternGeometry::perpendicularDirection(axisDir, gp_Dir(1, 0, 0));
        }
        tangent.Normalize();
        return gp_Dir(tangent);
    }
    return patternDirection(axisIndex);
}

void Widget::computePatternPitchDragScreenAxis(double& ox, double& oy, double& dx, double& dy,
                                             int axisIndex) const
{
    ox = oy = dx = dy = 0.0;
    if (!renderer || !vtkWidget || !patternDialog_) {
        return;
    }

    gp_Pnt O;
    gp_Pnt tip;
    computePatternGizmoSegment(O, tip, axisIndex);

    const int vtkH = vtkWidget->height();
    auto toScreen = [&](const gp_Pnt& p, double& sx, double& sy) {
        renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
        renderer->WorldToDisplay();
        double d[3] = {0, 0, 0};
        renderer->GetDisplayPoint(d);
        sx = d[0];
        sy = static_cast<double>(vtkH) - d[1];
    };

    if (patternAxisUsesAngularPitch(axisIndex)) {
        const gp_Dir tangent = patternGizmoArrowDirection(O, tip, axisIndex);
        const double lead = std::max(1.0, patternGizmoSpanLength(patternDialog_->direction1()) * 0.12);
        const gp_Pnt leadPt = tip.Translated(gp_Vec(tangent) * lead);
        double tx = 0.0, ty = 0.0, lx = 0.0, ly = 0.0;
        toScreen(tip, tx, ty);
        toScreen(leadPt, lx, ly);
        ox = tx;
        oy = ty;
        dx = lx - tx;
        dy = ly - ty;
        return;
    }

    double tx = 0.0, ty = 0.0;
    toScreen(O, ox, oy);
    toScreen(tip, tx, ty);
    dx = tx - ox;
    dy = ty - oy;
}

gp_Dir Widget::patternDirection(int axisIndex) const
{
    if (!patternDialog_) {
        return gp_Dir(1, 0, 0);
    }
    if (axisIndex == 1 && patternDialog_->layoutType() == PatternLayoutType::Linear
        && patternDialog_->useDirection2()) {
        return patternDialog_->direction2();
    }
    return patternDialog_->direction1();
}

void Widget::updatePatternPreview()
{
    if (!patternDialog_ || patternSelectedIndices_.isEmpty() || !patternDialog_->hasDirection1()) {
        clearPatternPreview();
        return;
    }
    const PatternLayoutType layout = patternDialog_->layoutType();
    if ((layout == PatternLayoutType::Circular || layout == PatternLayoutType::Polygonal)
        && !patternDialog_->hasRotationCenter()) {
        clearPatternPreview();
        return;
    }

    const gp_Pnt origin = resolvePatternOrigin(patternDialog_, patternSelectedIndices_);
    const gp_Dir d1 = patternDialog_->direction1();
    gp_Dir radialDir = patternDialog_->direction2();
    bool useRadial = false;
    if (patternDialog_->layoutType() == PatternLayoutType::Linear) {
        useRadial = patternDialog_->useDirection2() && patternDialog_->hasDirection2();
        radialDir = patternDialog_->direction2();
    } else {
        useRadial = patternDialog_->createConcentricMembers();
        if (useRadial) {
            radialDir = PatternGeometry::perpendicularDirection(d1, gp_Dir(1, 0, 0));
        }
    }

    const QList<TopoDS_Shape> sourceShapes = collectPatternSourceShapes(
        historyList, patternSelectedIndices_, geometryStore_);
    const TopoDS_Shape shape = PatternGeometry::buildPatternShape(
        sourceShapes, patternDialog_->layoutType(), origin, d1, patternDialog_->pitch1(),
        patternDialog_->count1(), useRadial, radialDir, patternDialog_->pitch2(),
        patternDialog_->count2(), patternDialog_->polygonSpanDegrees(),
        patternDialog_->polygonSpacingMode(), patternDialog_->polygonAlongEdgeCount(),
        patternDialog_->polygonAlongEdgePitch());
    if (shape.IsNull()) {
        clearPatternPreview();
        return;
    }

    if (previewActor) {
        removeSceneActor(previewActor);
        previewActor = nullptr;
    }
    vtkSmartPointer<vtkPolyData> poly = m_occConverter.convert(shape);
    if (!poly || poly->GetNumberOfPoints() == 0) {
        return;
    }
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(poly);
    previewActor = vtkSmartPointer<vtkActor>::New();
    previewActor->SetMapper(mapper);
    previewActor->GetProperty()->SetColor(0.55, 0.78, 0.95);
    previewActor->GetProperty()->SetOpacity(0.55);
    previewActor->GetProperty()->SetBackfaceCulling(false);
    previewActor->SetPickable(false);
    addAppearanceActor(previewActor);

    patternArrayOrigin_ = computePatternArrayOrigin();
    if (patternPitchInteractiveActive_) {
        updatePatternPitchGizmo();
        updatePatternPitchOverlay();
    }

    refreshCameraClippingRange();

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearPatternPreview()
{
    if (previewActor && renderer) {
        removeSceneActor(previewActor);
        previewActor = nullptr;
    }
    clearPatternPitchGizmoOnly();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::performPattern(PatternFeatureDialog* dialog)
{
    if (!dialog || patternSelectedIndices_.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择要阵列的体并指定方向1矢量。"));
        return;
    }
    if (!dialog->hasDirection1()) {
        QMessageBox::warning(this, tr("提示"), tr("请先指定方向1矢量。"));
        return;
    }
    if (dialog->layoutType() == PatternLayoutType::Linear && dialog->useDirection2()
        && !dialog->hasDirection2()) {
        QMessageBox::warning(this, tr("提示"), tr("已启用方向2，请先指定方向2矢量。"));
        return;
    }
    if ((dialog->layoutType() == PatternLayoutType::Circular
         || dialog->layoutType() == PatternLayoutType::Polygonal)
        && !dialog->hasRotationCenter()) {
        QMessageBox::warning(this, tr("提示"), tr("请先指定旋转中心点。"));
        return;
    }

    const gp_Pnt origin = resolvePatternOrigin(dialog, patternSelectedIndices_);
    const gp_Dir d1 = dialog->direction1();
    gp_Dir radialDir = dialog->direction2();
    bool useRadial = false;
    if (dialog->layoutType() == PatternLayoutType::Linear) {
        useRadial = dialog->useDirection2() && dialog->hasDirection2();
        radialDir = dialog->direction2();
    } else {
        useRadial = dialog->createConcentricMembers();
        if (useRadial) {
            radialDir = PatternGeometry::perpendicularDirection(d1, gp_Dir(1, 0, 0));
        }
    }

    PatternCommand* cmd = new PatternCommand(
        this, dialog->layoutType(), patternSelectedIndices_, origin, d1, dialog->pitch1(),
        dialog->count1(), useRadial, radialDir, dialog->pitch2(), dialog->count2(),
        dialog->polygonSpanDegrees(), dialog->polygonSpacingMode(), dialog->polygonAlongEdgeCount(),
        dialog->polygonAlongEdgePitch(), QStringLiteral("阵列特征"), QColor(100, 180, 255));
    executeCommand(cmd);
    clearSelectedPoint();
    updatePatternPreview();
}
