// 阵列特征：对话框、体选择、预览与执行
#include "widget.h"
#include "patternfeaturedialog.h"
#include "patterncommand.h"
#include "ui_widget.h"

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopoDS_Compound.hxx>

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

namespace {

constexpr double kDegToRad = M_PI / 180.0;

gp_Dir perpendicularDirection(const gp_Dir& axis, const gp_Dir& hint)
{
    gp_Vec perp = gp_Vec(axis).Crossed(gp_Vec(hint));
    if (perp.Magnitude() < Precision::Angular()) {
        perp = gp_Vec(axis).Crossed(gp_Vec(1, 0, 0));
    }
    if (perp.Magnitude() < Precision::Angular()) {
        perp = gp_Vec(axis).Crossed(gp_Vec(0, 1, 0));
    }
    perp.Normalize();
    return gp_Dir(perp);
}

gp_Trsf makeCircularInstanceTransform(const gp_Pnt& origin,
                                      const gp_Dir& axisDir,
                                      double angularPitchDeg,
                                      int indexI,
                                      bool useRadial,
                                      const gp_Dir& radialDir,
                                      double radialPitch,
                                      int indexJ)
{
    gp_Trsf trsf;
    if (indexI == 0 && indexJ == 0) {
        return trsf;
    }

    gp_Trsf rot;
    gp_Trsf radial;
    bool hasRot = false;
    bool hasRadial = false;

    if (indexI != 0) {
        rot.SetRotation(gp_Ax1(origin, axisDir), indexI * angularPitchDeg * kDegToRad);
        hasRot = true;
    }
    if (indexJ != 0 && useRadial) {
        radial.SetTranslation(gp_Vec(radialDir) * (indexJ * radialPitch));
        hasRadial = true;
    }

    if (hasRot && hasRadial) {
        trsf = radial;
        trsf.Multiply(rot);
    } else if (hasRot) {
        trsf = rot;
    } else if (hasRadial) {
        trsf = radial;
    }
    return trsf;
}

gp_Trsf makeRotatedRadialTransform(const gp_Pnt& origin,
                                   const gp_Dir& axisDir,
                                   double angleRad,
                                   bool useRadial,
                                   const gp_Dir& radialDir,
                                   double radialPitch,
                                   int radialIndex)
{
    gp_Trsf trsf;
    gp_Trsf rot;
    gp_Trsf radial;
    bool hasRot = false;
    bool hasRadial = false;

    if (std::abs(angleRad) > Precision::Angular()) {
        rot.SetRotation(gp_Ax1(origin, axisDir), angleRad);
        hasRot = true;
    }
    if (radialIndex != 0 && useRadial) {
        radial.SetTranslation(gp_Vec(radialDir) * (radialIndex * radialPitch));
        hasRadial = true;
    }

    if (hasRot && hasRadial) {
        trsf = radial;
        trsf.Multiply(rot);
    } else if (hasRot) {
        trsf = rot;
    } else if (hasRadial) {
        trsf = radial;
    }
    return trsf;
}

double polygonPatternRadius(const gp_Pnt& origin,
                            const gp_Dir& axisDir,
                            const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) {
        return 10.0;
    }
    Standard_Real xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const gp_Pnt center((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
    gp_Vec radial = gp_Vec(origin, center);
    radial -= gp_Vec(axisDir) * radial.Dot(gp_Vec(axisDir));
    const double r = radial.Magnitude();
    return (r > Precision::Confusion()) ? r : 10.0;
}

int polygonAlongEdgeInstanceCount(double edgeArcLength,
                                  PolygonSpacingMode spacing,
                                  int countPerSide,
                                  double edgePitch)
{
    if (spacing == PolygonSpacingMode::CountPerSide) {
        return std::max(1, countPerSide);
    }
    if (edgePitch <= Precision::Confusion()) {
        return 1;
    }
    return std::max(1, static_cast<int>(std::floor(edgeArcLength / edgePitch)) + 1);
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
        if (!historyList[i].actor) continue;
        const QColor c = historyList[i].color;
        historyList[i].actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
    }
    for (int index : patternSelectedIndices_) {
        if (index >= 0 && index < historyList.size() && historyList[index].actor) {
            historyList[index].actor->GetProperty()->SetColor(0.8, 0.8, 0.2);
        }
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearPatternSelectionHighlight()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (!historyList[i].actor) continue;
        const QColor c = historyList[i].color;
        historyList[i].actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
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
        const TopoDS_Shape& s = historyList[idx].occShape;
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
        const TopoDS_Shape& s = historyList[idx].occShape;
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
                refDir = perpendicularDirection(axisDir, patternDialog_->direction2());
            } else {
                refDir = perpendicularDirection(axisDir, gp_Dir(1, 0, 0));
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
            radial = gp_Vec(perpendicularDirection(axisDir, gp_Dir(1, 0, 0)));
        }
        gp_Vec tangent = gp_Vec(axisDir).Crossed(radial);
        if (tangent.Magnitude() < Precision::Confusion()) {
            return perpendicularDirection(axisDir, gp_Dir(1, 0, 0));
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

TopoDS_Shape Widget::buildLinearPatternShape(const QList<int>& sourceIndices,
                                             const gp_Dir& direction1,
                                             double pitch1,
                                             int count1,
                                             bool useDirection2,
                                             const gp_Dir& direction2,
                                             double pitch2,
                                             int count2) const
{
    if (sourceIndices.isEmpty() || count1 < 1) {
        return TopoDS_Shape();
    }
    const int c2 = useDirection2 ? std::max(1, count2) : 1;

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (int srcIdx : sourceIndices) {
        if (srcIdx < 0 || srcIdx >= historyList.size()) continue;
        const TopoDS_Shape& src = historyList[srcIdx].occShape;
        if (src.IsNull()) continue;

        for (int i = 0; i < count1; ++i) {
            for (int j = 0; j < c2; ++j) {
                if (i == 0 && j == 0) {
                    builder.Add(compound, src);
                    continue;
                }
                gp_Trsf trsf;
                const gp_Vec offset = gp_Vec(direction1) * (i * pitch1) + gp_Vec(direction2) * (j * pitch2);
                trsf.SetTranslation(offset);
                BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                builder.Add(compound, transformer.Shape());
            }
        }
    }
    return compound;
}

TopoDS_Shape Widget::buildCircularPatternShape(const QList<int>& sourceIndices,
                                               const gp_Pnt& origin,
                                               const gp_Dir& axisDir,
                                               double angularPitchDeg,
                                               int count1,
                                               bool useDirection2,
                                               const gp_Dir& radialDir,
                                               double radialPitch,
                                               int count2) const
{
    if (sourceIndices.isEmpty() || count1 < 1) {
        return TopoDS_Shape();
    }
    const int c2 = useDirection2 ? std::max(1, count2) : 1;

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (int srcIdx : sourceIndices) {
        if (srcIdx < 0 || srcIdx >= historyList.size()) continue;
        const TopoDS_Shape& src = historyList[srcIdx].occShape;
        if (src.IsNull()) continue;

        for (int i = 0; i < count1; ++i) {
            for (int j = 0; j < c2; ++j) {
                if (i == 0 && j == 0) {
                    builder.Add(compound, src);
                    continue;
                }
                const gp_Trsf trsf = makeCircularInstanceTransform(
                    origin, axisDir, angularPitchDeg, i, useDirection2, radialDir, radialPitch, j);
                BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                builder.Add(compound, transformer.Shape());
            }
        }
    }
    return compound;
}

TopoDS_Shape Widget::buildPolygonalPatternShape(const QList<int>& sourceIndices,
                                                const gp_Pnt& origin,
                                                const gp_Dir& axisDir,
                                                double spanDegrees,
                                                int sides,
                                                PolygonSpacingMode polygonSpacing,
                                                int alongEdgeCount,
                                                double alongEdgePitch,
                                                bool useRadialReplication,
                                                const gp_Dir& radialDir,
                                                double radialPitch,
                                                int count2) const
{
    if (sourceIndices.isEmpty() || sides < 1) {
        return TopoDS_Shape();
    }
    const int numSides = std::max(1, sides);
    const int c2 = useRadialReplication ? std::max(1, count2) : 1;
    const double spanRad = spanDegrees * kDegToRad;
    const double sideAngleRad = spanRad / static_cast<double>(numSides);

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for (int srcIdx : sourceIndices) {
        if (srcIdx < 0 || srcIdx >= historyList.size()) {
            continue;
        }
        const TopoDS_Shape& src = historyList[srcIdx].occShape;
        if (src.IsNull()) {
            continue;
        }

        const double radius = polygonPatternRadius(origin, axisDir, src);

        for (int s = 0; s < numSides; ++s) {
            const double ang0 = s * sideAngleRad;
            const double ang1 = (s + 1) * sideAngleRad;
            const double edgeArcLen = radius * (ang1 - ang0);
            const int nAlong = polygonAlongEdgeInstanceCount(
                edgeArcLen, polygonSpacing, alongEdgeCount, alongEdgePitch);

            for (int k = 0; k < nAlong; ++k) {
                const double t = (nAlong <= 1) ? 0.0 : static_cast<double>(k) / static_cast<double>(nAlong - 1);
                const double angleRad = ang0 + t * (ang1 - ang0);

                for (int j = 0; j < c2; ++j) {
                    if (s == 0 && k == 0 && j == 0) {
                        builder.Add(compound, src);
                        continue;
                    }
                    const gp_Trsf trsf = makeRotatedRadialTransform(
                        origin, axisDir, angleRad, useRadialReplication, radialDir, radialPitch, j);
                    BRepBuilderAPI_Transform transformer(src, trsf, Standard_True);
                    builder.Add(compound, transformer.Shape());
                }
            }
        }
    }
    return compound;
}

TopoDS_Shape Widget::buildPatternShape(PatternFeatureDialog* dialog,
                                       const QList<int>& sourceIndices) const
{
    if (!dialog || sourceIndices.isEmpty()) {
        return TopoDS_Shape();
    }

    const gp_Pnt origin = resolvePatternOrigin(dialog, sourceIndices);
    const gp_Dir d1 = dialog->direction1();
    gp_Dir radialDir = dialog->direction2();
    bool useRadial = false;

    if (dialog->layoutType() == PatternLayoutType::Linear) {
        useRadial = dialog->useDirection2() && dialog->hasDirection2();
        radialDir = dialog->direction2();
    } else {
        useRadial = dialog->createConcentricMembers();
        if (useRadial) {
            radialDir = perpendicularDirection(d1, gp_Dir(1, 0, 0));
        }
    }

    switch (dialog->layoutType()) {
    case PatternLayoutType::Circular:
        return buildCircularPatternShape(sourceIndices, origin, d1, dialog->pitch1(), dialog->count1(),
                                       useRadial, radialDir, dialog->pitch2(), dialog->count2());
    case PatternLayoutType::Polygonal:
        return buildPolygonalPatternShape(sourceIndices, origin, d1, dialog->polygonSpanDegrees(),
                                          dialog->count1(), dialog->polygonSpacingMode(),
                                          dialog->polygonAlongEdgeCount(), dialog->polygonAlongEdgePitch(),
                                          useRadial, radialDir, dialog->pitch2(), dialog->count2());
    case PatternLayoutType::Linear:
    default:
        return buildLinearPatternShape(sourceIndices, d1, dialog->pitch1(), dialog->count1(),
                                       useRadial, radialDir, dialog->pitch2(), dialog->count2());
    }
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

    const TopoDS_Shape shape = buildPatternShape(patternDialog_, patternSelectedIndices_);
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
            radialDir = perpendicularDirection(d1, gp_Dir(1, 0, 0));
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
