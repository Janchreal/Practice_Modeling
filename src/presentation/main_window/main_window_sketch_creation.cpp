#include "main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "ui_main_window.h"

#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkRenderWindow.h>

#include <Qt>

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
