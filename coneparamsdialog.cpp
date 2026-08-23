#include "coneparamsdialog.h"
#include "ui_coneparamsdialog.h"
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QToolButton>

ConeParamsDialog::ConeParamsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConeParamsDialog)
    , originPointSet(false)
    , originX(0.0)
    , originY(0.0)
    , originZ(0.0)
{
    ui->setupUi(this);
    ensureShowResultButton();

    // toolButton：矢量模式选择（用于打开 vectordialog）
    if (ui->toolButton) {
        QMenu* menu = new QMenu(this);
        auto addAct = [&](const QString& text, int modeIndex) {
            QAction* act = menu->addAction(text);
            connect(act, &QAction::triggered, this, [this, modeIndex]() {
                emit requestVectorMode(modeIndex);
            });
        };

        addAct(tr("自动判断"), 0);
        addAct(tr("两点"), 1);
        addAct(tr("曲线/轴矢量"), 3);
        addAct(tr("曲线上矢量"), 4);
        addAct(tr("面/平面法向量"), 5);
        addAct(tr("面上点的矢量"), 6);
        addAct(tr("XC轴"), 7);
        addAct(tr("YC轴"), 8);
        addAct(tr("ZC轴"), 9);
        addAct(tr("-XC轴"), 10);
        addAct(tr("-YC轴"), 11);
        addAct(tr("-ZC轴"), 12);
        addAct(tr("视图方向"), 13);

        ui->toolButton->setMenu(menu);
        ui->toolButton->setPopupMode(QToolButton::InstantPopup);
        ui->toolButton->setToolTip("矢量模式");
    }

    // 反向：由右侧独立按钮控制（取代 toolButton 内的反向菜单）
    if (ui->pushButton_2) {
        connect(ui->pushButton_2, &QPushButton::clicked, this, [this]() {
            axisReversed = !axisReversed;
        });
    }

    // originSnapToolButton 菜单：选择用于原点捕捉的类型
    if (ui->originSnapToolButton) {
        QMenu* menu = new QMenu(this);
        auto addAct = [&](const QString& text, int kind) {
            QAction* act = menu->addAction(text);
            connect(act, &QAction::triggered, this, [this, kind, text]() {
                originSnapKind_ = kind;
                ui->originSnapToolButton->setText(text);
            });
        };

        addAct(tr("最近点"), 0);
        addAct(tr("端点"), 1);
        addAct(tr("中点"), 2);
        addAct(tr("交点"), 3);
        addAct(tr("圆心"), 4);
        addAct(tr("象限点"), 5);

        ui->originSnapToolButton->setMenu(menu);
        ui->originSnapToolButton->setPopupMode(QToolButton::InstantPopup);
        ui->originSnapToolButton->setToolTip("原点捕捉类型");
    }
}

ConeParamsDialog::~ConeParamsDialog()
{
    delete ui;
}

double ConeParamsDialog::getRadius1() const
{
    return ui->Radius1SpinBox->value();
}

double ConeParamsDialog::getRadius2() const
{
    return ui->Radius2SpinBox->value();
}

double ConeParamsDialog::getHeight() const
{
    return ui->HeightSpinBox->value();
}

void ConeParamsDialog::setRadius1(double radius1)
{
    ui->Radius1SpinBox->setValue(radius1);
}

void ConeParamsDialog::setRadius2(double radius2)
{
    ui->Radius2SpinBox->setValue(radius2);
}

void ConeParamsDialog::setHeight(double height)
{
    ui->HeightSpinBox->setValue(height);
}

void ConeParamsDialog::on_okandcancel_accepted()
{
    accept();
}

void ConeParamsDialog::on_okandcancel_rejected()
{
    reject();
}

void ConeParamsDialog::setOriginPoint(double x, double y, double z)
{
    originX = x;
    originY = y;
    originZ = z;
    originPointSet = true;
    
    // 更新按钮文本显示已选择
    ui->designated_point->setText(QString("指定点: (%1, %2, %3)").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2));
}

void ConeParamsDialog::getOriginPoint(double& x, double& y, double& z) const
{
    x = originX;
    y = originY;
    z = originZ;
}

void ConeParamsDialog::on_designated_point_clicked()
{
    if (originSnapKind_ >= 0) {
        emit requestPointSelectionWithSnap(originSnapKind_);
    } else {
        emit requestPointSelection();
    }
}

void ConeParamsDialog::on_applyButton_clicked()
{
    // 发送应用请求信号
    emit applyRequested();
}

void ConeParamsDialog::ensureShowResultButton()
{
    if (showResultButton_) return;
    showResultButton_ = new QPushButton(tr("显示结果"), this);
    showResultButton_->setObjectName(QStringLiteral("showResultButton"));
    if (auto* layout = findChild<QHBoxLayout*>("horizontalLayout_5")) {
        layout->addWidget(showResultButton_);
    }
    connect(showResultButton_, &QPushButton::clicked, this, &ConeParamsDialog::onShowResultButtonClicked);
}

void ConeParamsDialog::applyResultPreviewUiLock(bool locked)
{
    const QList<QWidget*> interactive = findChildren<QWidget*>();
    for (QWidget* w : interactive) {
        if (!w || w == showResultButton_) continue;
        if (qobject_cast<QAbstractButton*>(w)
            || qobject_cast<QComboBox*>(w)
            || qobject_cast<QLineEdit*>(w)
            || qobject_cast<QAbstractSpinBox*>(w)) {
            w->setEnabled(!locked);
        }
    }
    if (showResultButton_) {
        showResultButton_->setEnabled(true);
        showResultButton_->setText(locked ? tr("取消显示结果") : tr("显示结果"));
    }
}

void ConeParamsDialog::setResultPreviewActive(bool active)
{
    if (resultPreviewActive_ == active) return;
    resultPreviewActive_ = active;
    applyResultPreviewUiLock(active);
}

void ConeParamsDialog::onShowResultButtonClicked()
{
    if (resultPreviewActive_) {
        setResultPreviewActive(false);
        emit cancelPreviewRequested();
        return;
    }
    emit previewRequested();
}

