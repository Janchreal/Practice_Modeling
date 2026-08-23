// 基础体素：长方体/圆柱/圆锥/球 对话框与参数化创建（从 widget.cpp 拆出）
#include "widget.h"
#include "ui_widget.h"
#include "cuboidparamsdialog.h"
#include "cylinderdialog.h"
#include "coneparamsdialog.h"
#include "sphereparamsdialog.h"
#include "creategeometrycommand.h"
#include "command.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>

#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

void Widget::enterPrimitiveResultShown()
{
    // 显示结果 = 确认前一步：已正式创建并入栈，删除全部手柄（非隐藏）
    clearFeatureLivePreview();
    stopCuboidInteractiveMode();
    clearVectorDialogArrowPreview();

    if (statusBar()) {
        statusBar()->showMessage(tr("结果已创建并加入历史：点击「撤销结果」可撤销并继续编辑"), 4000);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::leavePrimitiveResultShown()
{
    // 取消显示结果：撤销刚压入栈的创建
    if (canUndo()) {
        undo();
    }
    clearFeatureLivePreview();

    if (cuboidDialog) {
        startCuboidInteractiveMode();
    }

    if (statusBar()) {
        statusBar()->showMessage(tr("已取消显示结果，可继续调整参数"), 2500);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 点击长方体按钮
void Widget::on_cuboid_clicked()
{
    // 创建对话框，使用new以便在关闭时自动删除
    CuboidParamsDialog *dialog = new CuboidParamsDialog(dialogParentWidget());
    cuboidDialog = dialog;  // 保存对话框指针
    
    // 设置对话框为非模态，允许与主窗口交互
    dialog->setModal(false);
    
    // 连接点选择信号
    connect(dialog, &CuboidParamsDialog::requestPointSelection, this, [this]() {
        // 进入点选择模式（可多选切换原点 A → a）
        currentSelectionMode = PointSelection;
        if (statusBar()) {
            statusBar()->showMessage(tr("块：请在视图中点击选择原点（可再次点击切换）"), 4000);
        }
    });

    // 连接点选择信号（snap 捕捉类型）
    connect(dialog, &CuboidParamsDialog::requestPointSelectionWithSnap, this,
            [this](int snapKind) {
                startOriginSnapSelection(OriginDialogKind::Cuboid, snapKind);
            });

    // toolButton：矢量模式选择（打开 vectordialog 并切换模式）
    connect(dialog, &CuboidParamsDialog::requestVectorMode, this, [this](int modeIndex) {
        openVectorDialog(modeIndex);
    });

    // 打开矢量对话框：任意方向 gp_Dir
    if (auto* vectorDialogBtn = dialog->findChild<QPushButton*>("pushButton_3")) {
        connect(vectorDialogBtn, &QPushButton::clicked, this, [this]() {
            openVectorDialog();
        });
    }

    // 同步“反向”按钮：让向量箭头预览方向随之变化
    if (auto* reverseBtn = dialog->findChild<QPushButton*>("pushButton")) {
        connect(reverseBtn, &QPushButton::clicked, this, [this, dialog]() {
            updateVectorArrowPreviewWithAxisReversed(dialog->isAxisReversed());
            if (cuboidInteractiveActive_) {
                updateCuboidInteractivePreview();
            }
        });
    }

    // 尺寸表达式项变更 → 同步半透明预览与操作柄
    connect(dialog, &CuboidParamsDialog::dimensionsChanged, this, [this]() {
        if (!cuboidInteractiveActive_) return;
        syncCuboidInteractiveFromDialog();
        updateCuboidInteractivePreview();
    });

    // 创建模型 Lambda 函数（供显示结果 / accepted / applyRequested 复用）
    auto createCuboidModel = [this, dialog]() {
        double length = dialog->getLength();
        double width = dialog->getWidth();
        double height = dialog->getHeight();

        bool hasOrigin = dialog->hasOriginPoint();
        double originX = 0.0, originY = 0.0, originZ = 0.0;
        if (hasOrigin) {
            dialog->getOriginPoint(originX, originY, originZ);
        }
        
        const bool axisReversed = dialog->isAxisReversed();
        if (hasOrigin) {
            createCuboidWithParams(length, width, height, true, originX, originY, originZ, axisReversed);
        } else {
            createCuboidWithParams(length, width, height, false, 0.0, 0.0, 0.0, axisReversed);
        }
        
        QTimer::singleShot(300, this, [this]() {
            clearSelectedPoint();
        });
    };

    connect(dialog, &CuboidParamsDialog::previewRequested, this, [this, dialog, createCuboidModel]() {
        const int before = getHistorySize();
        createCuboidModel();
        if (getHistorySize() <= before) {
            if (statusBar()) statusBar()->showMessage(tr("参数无效，无法显示结果"), 2500);
            return;
        }
        enterPrimitiveResultShown();
        dialog->setResultPreviewActive(true);
    });
    connect(dialog, &CuboidParamsDialog::cancelPreviewRequested, this, [this, dialog]() {
        leavePrimitiveResultShown();
        if (dialog->isResultPreviewActive()) {
            dialog->setResultPreviewActive(false);
        }
    });
    
    // 连接对话框接受信号
    connect(dialog, &QDialog::accepted, this, [this, dialog, createCuboidModel]() {
        // 已显示结果则无需再次创建（确认=显示结果后的收尾）
        if (!dialog->isResultPreviewActive()) {
            createCuboidModel();
        } else {
            dialog->setResultPreviewActive(false);
        }
        clearVectorDialogArrowPreview();
        stopCuboidInteractiveMode();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        cuboidDialog = nullptr;
    });
    
    // 连接应用按钮信号（创建模型但不关闭对话框）
    connect(dialog, &CuboidParamsDialog::applyRequested, this, [this, createCuboidModel]() {
        createCuboidModel();
        clearVectorDialogArrowPreview();
    });
    
    // 连接对话框拒绝信号
    connect(dialog, &QDialog::rejected, this, [this, dialog]() {
        if (dialog->isResultPreviewActive()) {
            leavePrimitiveResultShown();
            dialog->setResultPreviewActive(false);
        }
        clearSelectedPoint();
        clearVectorDialogArrowPreview();
        stopCuboidInteractiveMode();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }

        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        cuboidDialog = nullptr;
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 打开后启动半透明实体预览 + 操作柄，并自动进入“指定点”
    startCuboidInteractiveMode();
    currentSelectionMode = PointSelection;
    if (statusBar()) {
        statusBar()->showMessage(tr("块：请指定原点（原点和边长）"), 4000);
    }
    vtkWidget->setFocus();
}
// 创建长方体函数
void Widget::createCuboidWithParams(double length, double width, double height,
                                     bool hasOrigin, double originX, double originY, double originZ,
                                     bool axisReversed)
{
    Command* command = new CreateGeometryCommand(
        this, CUBOID,
        QString("块(l=%1,w=%2,h=%3)").arg(length).arg(width).arg(height),
        QColor(255, 140, 0), length, width, height,  // 橙色
        hasOrigin, originX, originY, originZ,
        currentAxisDirection,
        axisReversed,
        hasCustomVectorDir_,
        customVectorDir_
        );
    executeCommand(command);
}
// 点击圆柱体按钮
void Widget::on_cylinder_clicked()
{
    CylinderDialog *dialog = new CylinderDialog(dialogParentWidget());
    cylinderDialog = dialog;
    
    dialog->setModal(false);
    
    connect(dialog, &CylinderDialog::requestPointSelection, this, [this]() {
        currentSelectionMode = PointSelection;
        QMessageBox::information(dialogParentWidget(), "提示", "请在3D视图中点击模型上的点来选择原点位置");
    });

    connect(dialog, &CylinderDialog::requestPointSelectionWithSnap, this,
            [this](int snapKind) {
                startOriginSnapSelection(OriginDialogKind::Cylinder, snapKind);
            });

    connect(dialog, &CylinderDialog::requestVectorMode, this, [this](int modeIndex) {
        openVectorDialog(modeIndex);
    });

    if (auto* vectorDialogBtn = dialog->findChild<QPushButton*>("pushButton_4")) {
        connect(vectorDialogBtn, &QPushButton::clicked, this, [this]() {
            openVectorDialog();
        });
    }

    if (auto* reverseBtn = dialog->findChild<QPushButton*>("pushButton_2")) {
        connect(reverseBtn, &QPushButton::clicked, this, [this, dialog]() {
            updateVectorArrowPreviewWithAxisReversed(dialog->isAxisReversed());
        });
    }

    auto createCylinderModel = [this, dialog]() {
        double radius = dialog->getRadius();
        double height = dialog->getHeight();
        
        bool hasOrigin = dialog->hasOriginPoint();
        double originX = 0.0, originY = 0.0, originZ = 0.0;
        if (hasOrigin) {
            dialog->getOriginPoint(originX, originY, originZ);
        }
        
        bool axisReversed = dialog->isAxisReversed();

        if (hasOrigin) {
            createCylinderWithParams(radius, height, true, originX, originY, originZ, axisReversed);
        } else {
            createCylinderWithParams(radius, height, false, 0.0, 0.0, 0.0, axisReversed);
        }
        
        QTimer::singleShot(300, this, [this]() {
            clearSelectedPoint();
        });
    };

    connect(dialog, &CylinderDialog::previewRequested, this, [this, dialog, createCylinderModel]() {
        const int before = getHistorySize();
        createCylinderModel();
        if (getHistorySize() <= before) {
            if (statusBar()) statusBar()->showMessage(tr("参数无效，无法显示结果"), 2500);
            return;
        }
        enterPrimitiveResultShown();
        dialog->setResultPreviewActive(true);
    });
    connect(dialog, &CylinderDialog::cancelPreviewRequested, this, [this, dialog]() {
        leavePrimitiveResultShown();
        if (dialog->isResultPreviewActive()) dialog->setResultPreviewActive(false);
    });
    
    connect(dialog, &QDialog::accepted, this, [this, dialog, createCylinderModel]() {
        if (!dialog->isResultPreviewActive()) {
            createCylinderModel();
        } else {
            dialog->setResultPreviewActive(false);
        }
        clearVectorDialogArrowPreview();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        cylinderDialog = nullptr;
    });
    
    connect(dialog, &CylinderDialog::applyRequested, this, [this, createCylinderModel]() {
        createCylinderModel();
        clearVectorDialogArrowPreview();
    });
    
    connect(dialog, &QDialog::rejected, this, [this, dialog]() {
        if (dialog->isResultPreviewActive()) {
            leavePrimitiveResultShown();
            dialog->setResultPreviewActive(false);
        }
        clearSelectedPoint();
        clearVectorDialogArrowPreview();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }

        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        cylinderDialog = nullptr;
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    vtkWidget->setFocus();
}
// 创建圆柱体函数
void Widget::createCylinderWithParams(double radius, double height,
                                     bool hasOrigin, double originX, double originY, double originZ,
                                     bool axisReversed)
{
    Command* command = new CreateGeometryCommand(
        this, CYLINDER,
        QString("圆柱体(r=%1,h=%2)").arg(radius).arg(height),
        QColor(255, 140, 0), radius, height, 0.0,
        hasOrigin, originX, originY, originZ,
        currentAxisDirection,
        axisReversed,
        hasCustomVectorDir_,
        customVectorDir_
        );
    executeCommand(command);
}
// 点击圆锥体按钮
void Widget::on_cone_clicked()
{
    ConeParamsDialog *dialog = new ConeParamsDialog(dialogParentWidget());
    coneDialog = dialog;
    
    dialog->setModal(false);
    
    connect(dialog, &ConeParamsDialog::requestPointSelection, this, [this]() {
        currentSelectionMode = PointSelection;
        QMessageBox::information(dialogParentWidget(), "提示", "请在3D视图中点击模型上的点来选择原点位置");
    });

    connect(dialog, &ConeParamsDialog::requestPointSelectionWithSnap, this,
            [this](int snapKind) {
                startOriginSnapSelection(OriginDialogKind::Cone, snapKind);
            });

    connect(dialog, &ConeParamsDialog::requestVectorMode, this, [this](int modeIndex) {
        openVectorDialog(modeIndex);
    });

    if (auto* vectorDialogBtn = dialog->findChild<QPushButton*>("pushButton_4")) {
        connect(vectorDialogBtn, &QPushButton::clicked, this, [this]() {
            openVectorDialog();
        });
    }

    if (auto* reverseBtn = dialog->findChild<QPushButton*>("pushButton_2")) {
        connect(reverseBtn, &QPushButton::clicked, this, [this, dialog]() {
            updateVectorArrowPreviewWithAxisReversed(dialog->isAxisReversed());
        });
    }

    auto createConeModel = [this, dialog]() {
        double radius1 = dialog->getRadius1();
        double radius2 = dialog->getRadius2();
        double height = dialog->getHeight();
        
        bool hasOrigin = dialog->hasOriginPoint();
        double originX = 0.0, originY = 0.0, originZ = 0.0;
        if (hasOrigin) {
            dialog->getOriginPoint(originX, originY, originZ);
        }
        
        bool axisReversed = dialog->isAxisReversed();

        if (hasOrigin) {
            createConeWithParams(radius1, radius2, height, true, originX, originY, originZ, axisReversed);
        } else {
            createConeWithParams(radius1, radius2, height, false, 0.0, 0.0, 0.0, axisReversed);
        }
        
        QTimer::singleShot(300, this, [this]() {
            clearSelectedPoint();
        });
    };

    connect(dialog, &ConeParamsDialog::previewRequested, this, [this, dialog, createConeModel]() {
        const int before = getHistorySize();
        createConeModel();
        if (getHistorySize() <= before) {
            if (statusBar()) statusBar()->showMessage(tr("参数无效，无法显示结果"), 2500);
            return;
        }
        enterPrimitiveResultShown();
        dialog->setResultPreviewActive(true);
    });
    connect(dialog, &ConeParamsDialog::cancelPreviewRequested, this, [this, dialog]() {
        leavePrimitiveResultShown();
        if (dialog->isResultPreviewActive()) dialog->setResultPreviewActive(false);
    });
    
    connect(dialog, &QDialog::accepted, this, [this, dialog, createConeModel]() {
        if (!dialog->isResultPreviewActive()) {
            createConeModel();
        } else {
            dialog->setResultPreviewActive(false);
        }
        clearVectorDialogArrowPreview();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        coneDialog = nullptr;
    });
    
    connect(dialog, &ConeParamsDialog::applyRequested, this, [this, createConeModel]() {
        createConeModel();
        clearVectorDialogArrowPreview();
    });
    
    connect(dialog, &QDialog::rejected, this, [this, dialog]() {
        if (dialog->isResultPreviewActive()) {
            leavePrimitiveResultShown();
            dialog->setResultPreviewActive(false);
        }
        clearSelectedPoint();
        clearVectorDialogArrowPreview();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        coneDialog = nullptr;
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    vtkWidget->setFocus();
}
// 创建圆锥体（圆台）函数
void Widget::createConeWithParams(double radius1, double radius2, double height,
                                  bool hasOrigin, double originX, double originY, double originZ,
                                  bool axisReversed)
{
    Command* command = new CreateGeometryCommand(
        this, CONE,
        QString("圆锥体(r1=%1,r2=%2,h=%3)").arg(radius1).arg(radius2).arg(height),
        QColor(255, 140, 0), radius1, radius2, height,
        hasOrigin, originX, originY, originZ,
        currentAxisDirection,
        axisReversed,
        hasCustomVectorDir_,
        customVectorDir_
        );
    executeCommand(command);
}
// 点击球体按钮
void Widget::on_sphere_clicked()
{
    SphereParamsDialog *dialog = new SphereParamsDialog(dialogParentWidget());
    sphereDialog = dialog;
    
    dialog->setModal(false);
    
    connect(dialog, &SphereParamsDialog::requestPointSelection, this, [this]() {
        currentSelectionMode = PointSelection;
        QMessageBox::information(dialogParentWidget(), "提示", "请在3D视图中点击模型上的点来选择原点位置");
    });

    connect(dialog, &SphereParamsDialog::requestPointSelectionWithSnap, this,
            [this](int snapKind) {
                startOriginSnapSelection(OriginDialogKind::Sphere, snapKind);
            });

    auto createSphereModel = [this, dialog]() {
        double radius = dialog->getRadius();
        int thetaResolution = dialog->getThetaResolution();
        int phiResolution = dialog->getPhiResolution();
        
        bool hasOrigin = dialog->hasOriginPoint();
        double originX = 0.0, originY = 0.0, originZ = 0.0;
        if (hasOrigin) {
            dialog->getOriginPoint(originX, originY, originZ);
        }
        
        if (hasOrigin) {
            createSphereWithParams(radius, thetaResolution, phiResolution, true, originX, originY, originZ);
        } else {
            createSphereWithParams(radius, thetaResolution, phiResolution, false);
        }
        
        QTimer::singleShot(300, this, [this]() {
            clearSelectedPoint();
        });
    };

    connect(dialog, &SphereParamsDialog::previewRequested, this, [this, dialog, createSphereModel]() {
        const int before = getHistorySize();
        createSphereModel();
        if (getHistorySize() <= before) {
            if (statusBar()) statusBar()->showMessage(tr("参数无效，无法显示结果"), 2500);
            return;
        }
        enterPrimitiveResultShown();
        dialog->setResultPreviewActive(true);
    });
    connect(dialog, &SphereParamsDialog::cancelPreviewRequested, this, [this, dialog]() {
        leavePrimitiveResultShown();
        if (dialog->isResultPreviewActive()) dialog->setResultPreviewActive(false);
    });
    
    connect(dialog, &QDialog::accepted, this, [this, dialog, createSphereModel]() {
        if (!dialog->isResultPreviewActive()) {
            createSphereModel();
        } else {
            dialog->setResultPreviewActive(false);
        }

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        sphereDialog = nullptr;
    });
    
    connect(dialog, &SphereParamsDialog::applyRequested, this, [this, createSphereModel]() {
        createSphereModel();
    });
    
    connect(dialog, &QDialog::rejected, this, [this, dialog]() {
        if (dialog->isResultPreviewActive()) {
            leavePrimitiveResultShown();
            dialog->setResultPreviewActive(false);
        }
        clearSelectedPoint();

        if (originSnapSelectionActive_) {
            originSnapSelectionActive_ = false;
            pendingOriginDialogKind_ = OriginDialogKind::None;
            pendingOriginSnapKind_ = -1;
            clearSnapSettings();
        }
        if (currentSelectionMode == PointSelection) {
            currentSelectionMode = None;
        }
        sphereDialog = nullptr;
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    vtkWidget->setFocus();
}
// 创建球体函数
void Widget::createSphereWithParams(double radius, int thetaResolution, int phiResolution,
                                    bool hasOrigin, double originX, double originY, double originZ)
{
    Command* command = new CreateGeometryCommand(
        this, SPHERE,
        QString("球体(r=%1)").arg(radius),
        QColor(255, 140, 0), radius, thetaResolution, phiResolution,
        hasOrigin, originX, originY, originZ,
        currentAxisDirection,
        false
        );
    executeCommand(command);
}
