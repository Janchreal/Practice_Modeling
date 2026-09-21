#include "main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "ui_main_window.h"

#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkCellPicker.h>
#include <vtkFeatureEdges.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkTransform.h>

#include <algorithm>
#include <cmath>

#include <Qt>

namespace {

void sketchPrincipalPlaneColor(int planeId, double& r, double& g, double& b)
{
    if (planeId == 0) {        // XC-YC
        r = 0.95; g = 0.58; b = 0.16;
    } else if (planeId == 1) { // YC-ZC
        r = 0.85; g = 0.28; b = 0.20;
    } else {                   // XC-ZC
        r = 0.20; g = 0.62; b = 0.54;
    }
}

} // namespace

void Widget::on_pushButton_5_clicked()
{
    clearSketchEditHover();
    endSketchBrushStroke();
    openSketchCreateDialog();
}

gp_Pln Widget::sketchPrincipalPlaneFromId(int planeId) const
{
    if (planeId == 1) {
        return gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)); // YC-ZC
    }
    if (planeId == 2) {
        return gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)); // XC-ZC
    }
    return gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));     // XC-YC
}

bool Widget::tryPickSketchPrincipalPlane(int x, int y, gp_Pln& outPlane)
{
    int planeId = -1;
    if (!pickSketchPrincipalPlaneAt(x, y, planeId)) {
        return false;
    }
    outPlane = sketchPrincipalPlaneFromId(planeId);
    applySketchPrincipalPlaneHighlight(planeId);
    return true;
}

void Widget::setSketchPrincipalPlanesVisible(bool visible)
{
    if (!renderer) {
        return;
    }
    if (!visible) {
        clearSketchPrincipalPlaneActors();
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        return;
    }

    clearSketchPrincipalPlaneActors();

    double halfSize = 3.0;
    bool hasBounds = false;
    double xmin = -1.0, xmax = 1.0;
    double ymin = -1.0, ymax = 1.0;
    double zmin = -1.0, zmax = 1.0;
    for (const ModelingHistory& record : historyList) {
        vtkActor* actor = renderStateFor(record).actor;
        if (!actor || actor->GetVisibility() == 0) continue;
        if (record.type == REFERENCE_CSYS || record.type == WORK_CSYS) continue;
        double b[6] = {0, 0, 0, 0, 0, 0};
        actor->GetBounds(b);
        if (!std::isfinite(b[0]) || !std::isfinite(b[1])) continue;
        if (!hasBounds) {
            xmin = b[0]; xmax = b[1];
            ymin = b[2]; ymax = b[3];
            zmin = b[4]; zmax = b[5];
            hasBounds = true;
        } else {
            xmin = std::min(xmin, b[0]); xmax = std::max(xmax, b[1]);
            ymin = std::min(ymin, b[2]); ymax = std::max(ymax, b[3]);
            zmin = std::min(zmin, b[4]); zmax = std::max(zmax, b[5]);
        }
    }
    if (hasBounds) {
        const double maxSize = std::max({xmax - xmin, ymax - ymin, zmax - zmin});
        if (std::isfinite(maxSize) && maxSize > 0.0) {
            halfSize = std::max(3.0, maxSize * 0.85);
        }
    }

    auto makePlane = [this, halfSize](int planeId, vtkSmartPointer<vtkActor>& outlineActor) {
        vtkSmartPointer<vtkPlaneSource> src = vtkSmartPointer<vtkPlaneSource>::New();
        const double h = halfSize;
        if (planeId == 0) {
            src->SetOrigin(-h, -h, 0.0);
            src->SetPoint1(h, -h, 0.0);
            src->SetPoint2(-h, h, 0.0);
        } else if (planeId == 1) {
            src->SetOrigin(0.0, -h, -h);
            src->SetPoint1(0.0, h, -h);
            src->SetPoint2(0.0, -h, h);
        } else {
            src->SetOrigin(-h, 0.0, -h);
            src->SetPoint1(h, 0.0, -h);
            src->SetPoint2(-h, 0.0, h);
        }
        src->SetXResolution(1);
        src->SetYResolution(1);
        src->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(src->GetOutputPort());
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        double r = 0, g = 0, b = 0;
        sketchPrincipalPlaneColor(planeId, r, g, b);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetOpacity(0.24);
        actor->GetProperty()->SetAmbient(0.75);
        actor->GetProperty()->SetDiffuse(0.35);
        actor->GetProperty()->SetSpecular(0.0);
        actor->GetProperty()->BackfaceCullingOff();
        actor->GetProperty()->SetRepresentationToSurface();
        actor->GetProperty()->EdgeVisibilityOff();
        actor->SetPickable(true);
        addAppearanceActor(actor);

        vtkSmartPointer<vtkFeatureEdges> edges = vtkSmartPointer<vtkFeatureEdges>::New();
        edges->SetInputConnection(src->GetOutputPort());
        edges->BoundaryEdgesOn();
        edges->FeatureEdgesOff();
        edges->ManifoldEdgesOff();
        edges->NonManifoldEdgesOff();
        edges->Update();

        vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        outlineMapper->SetInputConnection(edges->GetOutputPort());
        outlineMapper->ScalarVisibilityOff();
        outlineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-6.0, -6.0);

        outlineActor = vtkSmartPointer<vtkActor>::New();
        outlineActor->SetMapper(outlineMapper);
        outlineActor->GetProperty()->SetColor(r, g, b);
        outlineActor->GetProperty()->SetOpacity(1.0);
        outlineActor->GetProperty()->SetLineWidth(2.4);
        outlineActor->GetProperty()->SetLighting(false);
        outlineActor->SetPickable(false);
        addAppearanceActor(outlineActor);
        return actor;
    };

    sketchPrincipalPlaneXYActor_ = makePlane(0, sketchPrincipalPlaneXYOutlineActor_);
    sketchPrincipalPlaneYZActor_ = makePlane(1, sketchPrincipalPlaneYZOutlineActor_);
    sketchPrincipalPlaneXZActor_ = makePlane(2, sketchPrincipalPlaneXZOutlineActor_);

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearSketchPrincipalPlaneActors()
{
    if (!renderer) return;
    auto removeIf = [this](vtkSmartPointer<vtkActor>& actor) {
        if (actor) {
            removeSceneActor(actor);
            actor = nullptr;
        }
    };
    removeIf(sketchPrincipalPlaneXYActor_);
    removeIf(sketchPrincipalPlaneYZActor_);
    removeIf(sketchPrincipalPlaneXZActor_);
    removeIf(sketchPrincipalPlaneXYOutlineActor_);
    removeIf(sketchPrincipalPlaneYZOutlineActor_);
    removeIf(sketchPrincipalPlaneXZOutlineActor_);
}

void Widget::resetSketchPrincipalPlaneHighlight()
{
    auto resetPlane = [](const vtkSmartPointer<vtkActor>& actor, int planeId) {
        if (!actor) return;
        double r = 0, g = 0, b = 0;
        sketchPrincipalPlaneColor(planeId, r, g, b);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetEdgeColor(r, g, b);
        actor->GetProperty()->SetOpacity(0.24);
    };
    auto resetOutline = [](const vtkSmartPointer<vtkActor>& actor, int planeId) {
        if (!actor) return;
        double r = 0, g = 0, b = 0;
        sketchPrincipalPlaneColor(planeId, r, g, b);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetOpacity(1.0);
        actor->GetProperty()->SetLineWidth(2.4);
    };
    resetPlane(sketchPrincipalPlaneXYActor_, 0);
    resetPlane(sketchPrincipalPlaneYZActor_, 1);
    resetPlane(sketchPrincipalPlaneXZActor_, 2);
    resetOutline(sketchPrincipalPlaneXYOutlineActor_, 0);
    resetOutline(sketchPrincipalPlaneYZOutlineActor_, 1);
    resetOutline(sketchPrincipalPlaneXZOutlineActor_, 2);
}

void Widget::applySketchPrincipalPlaneHighlight(int planeId)
{
    resetSketchPrincipalPlaneHighlight();
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkActor> outlineActor;
    if (planeId == 0) {
        actor = sketchPrincipalPlaneXYActor_;
        outlineActor = sketchPrincipalPlaneXYOutlineActor_;
    } else if (planeId == 1) {
        actor = sketchPrincipalPlaneYZActor_;
        outlineActor = sketchPrincipalPlaneYZOutlineActor_;
    } else {
        actor = sketchPrincipalPlaneXZActor_;
        outlineActor = sketchPrincipalPlaneXZOutlineActor_;
    }
    if (!actor) return;
    double r = 0, g = 0, b = 0;
    sketchPrincipalPlaneColor(planeId, r, g, b);
    actor->GetProperty()->SetColor(r, g, b);
    actor->GetProperty()->SetEdgeColor(r, g, b);
    actor->GetProperty()->SetOpacity(0.50);
    if (outlineActor) {
        outlineActor->GetProperty()->SetColor(r, g, b);
        outlineActor->GetProperty()->SetOpacity(1.0);
        outlineActor->GetProperty()->SetLineWidth(3.2);
    }
}

bool Widget::pickSketchPrincipalPlaneAt(int x, int y, int& outPlaneId) const
{
    if (!renderer || !sketchPrincipalPlaneXYActor_) return false;
    if (sketchPrincipalPlaneXYActor_->GetVisibility() == 0) return false;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.02);
    picker->PickFromListOn();
    picker->AddPickList(const_cast<vtkActor*>(sketchPrincipalPlaneXYActor_.GetPointer()));
    picker->AddPickList(const_cast<vtkActor*>(sketchPrincipalPlaneYZActor_.GetPointer()));
    picker->AddPickList(const_cast<vtkActor*>(sketchPrincipalPlaneXZActor_.GetPointer()));
    picker->Pick(x, y, 0, renderer);

    vtkActor* picked = picker->GetActor();
    if (picked == sketchPrincipalPlaneXYActor_.GetPointer()) {
        outPlaneId = 0;
        return true;
    }
    if (picked == sketchPrincipalPlaneYZActor_.GetPointer()) {
        outPlaneId = 1;
        return true;
    }
    if (picked == sketchPrincipalPlaneXZActor_.GetPointer()) {
        outPlaneId = 2;
        return true;
    }
    return false;
}

void Widget::openSketchCreateDialog(SelectionMode restoreMode,
                                    SketchToolInputDialog::ObjectKind restoreObj,
                                    bool restoreSketchTool)
{
    clearSketchEditHover();
    endSketchBrushStroke();

    if (activeSketchCreateDialog_) {
        activeSketchCreateDialog_->raise();
        activeSketchCreateDialog_->activateWindow();
        return;
    }

    auto* dlg = new SketchCreateDialog(dialogParentWidget());
    dlg->setModal(false);
    dlg->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    activeSketchCreateDialog_ = dlg;

    connect(dlg, &SketchCreateDialog::showPrincipalPlanesChanged,
            this, &Widget::setSketchPrincipalPlanesVisible);

    connect(dlg, &SketchCreateDialog::planeNormalReversed, this, [this](const gp_Pln& pln) {
        createOrUpdateSketchSelectedDatumPlane(pln, TopoDS_Face());
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &SketchCreateDialog::requestPickPlane, this, [this]() {
        currentSelectionMode = SketchPlaneSelection;
        statusBar()->showMessage(tr("创建草图：请点击模型平面或显示的主平面。"), 4000);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg, restoreMode, restoreObj, restoreSketchTool]() {
        if (!dlg->hasPickedPlane()) {
            QMessageBox::warning(this, tr("创建草图"), tr("请先拾取参考平面。"));
            return;
        }

        if (!restoreSketchTool || (hasActiveSketch_ && activeSketch_.isValid())) {
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

        createOrUpdateSketchSelectedDatumPlane(activeSketchPlane_, TopoDS_Face());
        ensureSketchHistoryRecord();
        updateSketchHistoryShape();

        alignViewToSketchPlane(activeSketchPlane_);

        if (restoreSketchTool) {
            currentSelectionMode = restoreMode;
            openOrRaiseSketchToolInput(restoreObj, sketchContourChaining_);
            statusBar()->showMessage(tr("草图平面已更新，可继续绘制。"), 2500);
        } else {
            currentSelectionMode = None;
            statusBar()->showMessage(tr("草图已创建。现在可点击“直线/圆弧”在该平面上绘制。"), 4000);
        }
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::rejected, this, [this, restoreMode, restoreSketchTool]() {
        if (currentSelectionMode == SketchPlaneSelection) {
            currentSelectionMode = restoreSketchTool ? restoreMode : None;
        }
        activeSketchCreateDialog_ = nullptr;
    });

    connect(dlg, &QObject::destroyed, this,
            [this, restoreMode, restoreSketchTool]() {
        if (activeSketchCreateDialog_) activeSketchCreateDialog_ = nullptr;
        if (currentSelectionMode == SketchPlaneSelection) {
            currentSelectionMode = restoreSketchTool ? restoreMode : None;
        }
        setSketchPrincipalPlanesVisible(false);
    });

    setSketchPrincipalPlanesVisible(dlg->showPrincipalPlanes());
    dlg->show();
    dlg->raise();
    if (vtkWidget) {
        const QPoint g = vtkWidget->mapToGlobal(QPoint(8, 8));
        dlg->move(g);
    }
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

void Widget::setActiveSketchGeometriesForCommand(const QList<TopoDS_Shape>& geometries)
{
    activeSketch_.setGeometries(geometries);
    ensureSketchHistoryRecord();
    updateSketchHistoryShape();
    if (activeSketchHistoryIndex_ >= 0
        && activeSketchHistoryIndex_ < historyList.size()) {
        // 草图编辑是其下游特征的根变更，沿用现有配方/历史级联重建。
        updateDependentFeatures(activeSketchHistoryIndex_);
    }
    markDocumentModified(true);
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
        if (clickedButton == ui->pushButton_42) {
            closeSketchArcModeDialog();
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
    if (clickedButton != ui->pushButton_42)
        closeSketchArcModeDialog();
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
        closeSketchArcModeDialog();
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
    closeSketchArcModeDialog();
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjLine, true);
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
    closeSketchArcModeDialog();
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjLine, false);
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
    openOrRaiseSketchToolInput(SketchToolInputDialog::ObjArc, false);
    openOrRaiseSketchArcModeDialog();
    statusBar()->showMessage(tr("圆弧：可用三点或中心端点方式创建。"), 5000);
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
    statusBar()->showMessage(tr("圆：圆心+半径或圆上两点+半径；每次完成后需重新点击定义。"), 5000);
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
