// 旋转：对话框、选择、预览与执行（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "revolve_dialog.h"
#include "addmodelcommand.h"
#include "feature_topology.h"
#include "revolve_geometry.h"

#include <QColor>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>
#include <Qt>

#include <cmath>

#include <Standard_Failure.hxx>

#include <TopoDS_Compound.hxx>

#include <gp_Ax1.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

// 旋转：开始选择（曲线/面片）
void Widget::onRevolveStartSelection()
{
    if (!revolveDialog) {
        return;
    }

    QString sectionType = revolveDialog->getSectionType();
    if (sectionType == QStringLiteral("曲线")) {
        currentSelectionMode = EdgeSelection;
    } else if (sectionType == QStringLiteral("面片")) {
        currentSelectionMode = FaceSelection;
    } else {
        currentSelectionMode = ExtrusionSelection;
    }

    if (renderer && shapePicker) {
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        prepareShapePickerBindingsForCurrentContext();
        for (int i = 0; i < historyList.size(); ++i) {
            if (historyList[i].shapeDataSource) {
                historyList[i].shapeDataSource->Modified();
                historyList[i].shapeDataSource->Update();
            }
        }
    }
    setFeatureGizmoActorsPickable(false);
    setFeatureOperationGhostMode(true);
}

// 旋转：清除选择
void Widget::onRevolveClearSelection()
{
    ++extrudeRevolveSelectionEpoch_;
    extrusionSelectedFaces.clear();
    clearExtrusionFaceHighlight();
    clearFeatureLivePreview();
    clearRevolveHandles();
    if (revolveDialog) {
        revolveDialog->setSelectedGeometryCount(0);
    }
}

// 旋转：选择模式改变（edge / face）
void Widget::onRevolveSelectionModeChanged(const QString& mode)
{
    if (mode == QStringLiteral("edge")) {
        currentSelectionMode = EdgeSelection;
    } else if (mode == QStringLiteral("face")) {
        currentSelectionMode = FaceSelection;
    }

    // 清除之前的选择与高亮
    ++extrudeRevolveSelectionEpoch_;
    extrusionSelectedFaces.clear();
    clearExtrusionFaceHighlight();
    clearFeatureLivePreview();
    clearRevolveHandles();
    if (revolveDialog) {
        revolveDialog->setSelectedGeometryCount(0);
    }

    if (renderer && shapePicker) {
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        prepareShapePickerBindingsForCurrentContext();
        for (int i = 0; i < historyList.size(); ++i) {
            if (historyList[i].shapeDataSource) {
                historyList[i].shapeDataSource->Modified();
                historyList[i].shapeDataSource->Update();
            }
        }
    }
    setFeatureGizmoActorsPickable(false);
    if (featureOperationGhostMode_) {
        setFeatureOperationGhostMode(true);
    }
}

// 旋转：结果预览锁定
void Widget::onRevolvePreviewRequested()
{
    enterFeatureResultPreview(false);
}

void Widget::onRevolveCancelPreviewRequested()
{
    leaveFeatureResultPreview(false);
}

void Widget::on_revolve_clicked()
{
    // 创建旋转对话框（与拉伸类似，非模态 + 置顶）
    revolvedialog* dialog = new revolvedialog(dialogParentWidget());

    dialog->setModal(false);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    // 保存对话框指针
    revolveDialog = dialog;

    // 连接信号槽
    connect(dialog, &revolvedialog::startSelection, this, &Widget::onRevolveStartSelection);
    connect(dialog, &revolvedialog::clearSelection, this, &Widget::onRevolveClearSelection);
    connect(dialog, &revolvedialog::previewRequested, this, &Widget::onRevolvePreviewRequested);
    connect(dialog, &revolvedialog::cancelPreviewRequested, this, &Widget::onRevolveCancelPreviewRequested);
    connect(dialog, &revolvedialog::selectionModeChanged, this, &Widget::onRevolveSelectionModeChanged);

    connect(dialog, &revolvedialog::requestVectorMode, this, [this](int modeIndex) {
        if (revolveDialog) {
            revolveDialog->setVectorModeIndex(modeIndex);
        }
        if (modeIndex == 0) {
            applyAutoVectorFromSelection();
        } else {
            openVectorDialog(modeIndex);
        }
    });

    // 选择旋转轴方向矢量（提示用户去点中心三重轴）
    connect(dialog, &revolvedialog::vectorSelectionRequested, this, [this]() {
        statusBar()->showMessage(tr("请在视图中点击左下角三重轴选择旋转轴方向"), 5000);
    });

    connect(dialog, &revolvedialog::parametersChanged, this, [this]() {
        if (featureResultPreviewActive_) return;
        updateRevolveHandles();
        refreshRevolveLivePreview();
    });
    connect(dialog, &revolvedialog::startBooleanTargetSelection, this, [this]() {
        currentSelectionMode = FeatureBooleanTargetSelect;
        if (statusBar()) {
            statusBar()->showMessage(tr("请选择布尔运算的目标体"), 4000);
        }
    });

    // 打开矢量对话框（任意方向 gp_Dir）
    if (auto* vectorDialogBtn = dialog->findChild<QPushButton*>("pushButton_3")) {
        connect(vectorDialogBtn, &QPushButton::clicked, this, [this]() {
            openVectorDialog();
        });
    }

    // 同步“反向”按钮：让向量箭头预览方向随之变化
    if (auto* reverseBtn = dialog->findChild<QPushButton*>("pushButton_2")) {
        connect(reverseBtn, &QPushButton::clicked, this, [this, dialog]() {
            updateVectorArrowPreviewWithAxisReversed(dialog->isAxisReversed());
            updateRevolveHandles();
            refreshRevolveLivePreview();
        });
    }

    // 选择旋转中心点：进入点选择模式，使用已有 PointSelection 机制
    // 只在第一次进入该模式时弹出提示，避免重复弹窗打扰
    connect(dialog, &revolvedialog::pointSelectionRequested, this, [this]() {
        currentSelectionMode = PointSelection;
        static bool firstTimeRevolvePointHint = true;
        if (firstTimeRevolvePointHint) {
            QMessageBox::information(this, "提示", "请在模型上点击选择旋转中心点。");
            firstTimeRevolvePointHint = false;
        }
    });

    // 选择旋转中心点：使用 snap 捕捉类型
    connect(dialog, &revolvedialog::pointSelectionRequestedWithSnap, this, [this](int snapKind) {
        startOriginSnapSelection(OriginDialogKind::Revolve, snapKind);
    });

    // 点击“确定”时执行最终旋转
    connect(dialog, &revolvedialog::accepted, this, [this, dialog]() {
        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }

        bool hasSelection = !extrusionSelectedFaces.isEmpty();
        if (!hasSelection) {
            QMessageBox::warning(this, "警告", "请先选择要旋转的曲线或面片！");
            return;
        }

        double startAngleDeg = dialog->getStartAngle();
        double endAngleDeg = dialog->getEndAngle();
        double sweepDeg = endAngleDeg - startAngleDeg;
        while (sweepDeg > 360.0) sweepDeg -= 360.0;
        while (sweepDeg < -360.0) sweepDeg += 360.0;
        if (std::abs(sweepDeg) < 1e-6) {
            QMessageBox::warning(this, "警告", "旋转角度为 0，无法执行旋转操作！");
            return;
        }

        const int boolMode = dialog->booleanMode();
        const int boolTargetIdx = dialog->booleanTargetIndex();
        if (boolMode >= 0 && (boolTargetIdx < 0 || boolTargetIdx >= historyList.size())) {
            QMessageBox::warning(this, "警告", "请先选择布尔运算的目标体！");
            return;
        }

        TopoDS_Shape resultShape;
        if (!buildRevolvePreviewShape(dialog, resultShape, false) || resultShape.IsNull()) {
            if (boolMode >= 0) {
                QMessageBox::warning(this, "错误",
                    QStringLiteral("布尔运算失败！请确认旋转体与目标体有重叠（减去/求交），或目标体有效。"));
            } else {
                QMessageBox::warning(this, "错误", "旋转结果为空，请检查选择的几何体和旋转轴！");
            }
            return;
        }

        gp_Ax1 axis = getRevolutionAxis(dialog);
        FeatureRecipe recipe;
        recipe.hasRecipe = true;
        recipe.revolve.axisOrigin = axis.Location();
        recipe.revolve.axisDir = axis.Direction();
        recipe.revolve.angleDeg = sweepDeg;
        recipe.revolve.startAngleDeg = startAngleDeg;
        recipe.revolve.endAngleDeg = endAngleDeg;
        recipe.revolve.boolOpType = boolMode;
        recipe.revolve.boolTargetIndex = boolTargetIdx;
        for (const ExtrusionFaceSelection& selection : extrusionSelectedFaces) {
            recipe.revolve.profiles.append(makeSubShapeRef(
                selection.modelIndex,
                historyList[selection.modelIndex].occShape,
                selection.shape,
                selection.shapeType,
                selection.subShapeId));
            if (!recipe.parentIndices.contains(selection.modelIndex)) {
                recipe.parentIndices.append(selection.modelIndex);
            }
        }

        QList<int> hideSources = recipe.parentIndices;
        if (boolMode >= 0 && boolTargetIdx >= 0 && !hideSources.contains(boolTargetIdx)) {
            hideSources.append(boolTargetIdx);
        }

        executeCommand(new AddModelCommand(
            this, resultShape, QStringLiteral("旋转_特征"), REVOLUTION, QColor(255, 140, 0),
            sweepDeg, 0.0, 0.0, hideSources, !hideSources.isEmpty(), &recipe));

        clearFeatureLivePreview();
        clearVectorDialogArrowPreview();
        clearRevolveHandles();
        clearSelectedPoint();
    });

    // 点击“取消”时：清理预览和高亮
    connect(dialog, &revolvedialog::rejected, this, [this]() {
        cleanupExtrudeRevolveDialogSession();
    });

    // 对话框关闭时统一清理
    connect(dialog, &revolvedialog::finished, this, [this](int) {
        cleanupExtrudeRevolveDialogSession();
        if (vtkWidget) {
            vtkWidget->setFocus();
        }
        revolveDialog = nullptr;
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    // 幽灵模式延后到开始选择截面时再启用
    if (statusBar()) {
        statusBar()->showMessage(tr("旋转：默认自动判断方向，请选择截面并指定旋转轴/点"), 4000);
    }
    if (vtkWidget) {
        vtkWidget->setFocus();
        if (vtkWidget->renderWindow() && vtkWidget->renderWindow()->GetInteractor()) {
            vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
        }
    }
}

// 执行旋转操作
void Widget::performRevolution(double angle, const gp_Ax1& axis)
{
    try {
        // 基于当前选中的子几何（extrusionSelectedFaces）进行旋转
        if (extrusionSelectedFaces.isEmpty()) {
            QMessageBox::warning(this, "错误", "没有选中的曲线或面片，无法执行旋转！");
            return;
        }

        QList<TopoDS_Shape> profiles;
        profiles.reserve(extrusionSelectedFaces.size());
        for (const ExtrusionFaceSelection& selection : extrusionSelectedFaces) {
            profiles.append(selection.shape);
        }

        TopoDS_Compound resultCompound;
        if (!buildRevolutionCompound(profiles, axis, angle, resultCompound)) {
            QMessageBox::warning(this, "错误", "旋转结果为空，请检查选择的几何体和旋转轴！");
            return;
        }

        QString resultName = QString("旋转_特征");

        FeatureRecipe recipe;
        recipe.hasRecipe = true;
        recipe.revolve.axisOrigin = axis.Location();
        recipe.revolve.axisDir = axis.Direction();
        recipe.revolve.angleDeg = angle * 180.0 / M_PI;
        for (const ExtrusionFaceSelection& selection : extrusionSelectedFaces) {
            recipe.revolve.profiles.append(makeSubShapeRef(
                selection.modelIndex,
                historyList[selection.modelIndex].occShape,
                selection.shape,
                selection.shapeType,
                selection.subShapeId));
            if (!recipe.parentIndices.contains(selection.modelIndex)) {
                recipe.parentIndices.append(selection.modelIndex);
            }
        }

        executeCommand(new AddModelCommand(this, resultCompound, resultName, REVOLUTION, QColor(255, 140, 0), angle,
                                           0.0, 0.0, recipe.parentIndices, true, &recipe));


    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("旋转操作异常: %1").arg(e.GetMessageString()));
    }
}
