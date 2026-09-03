#include "sphere_params_dialog.h"
#include "ui_sphere_params_dialog.h"
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QToolButton>

SphereParamsDialog::SphereParamsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SphereParamsDialog)
    , originPointSet(false)
    , originX(0.0)
    , originY(0.0)
    , originZ(0.0)
{
    ui->setupUi(this);
    ensureShowResultButton();

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

SphereParamsDialog::~SphereParamsDialog()
{
    delete ui;
}

double SphereParamsDialog::getRadius() const
{
    return ui->radiusSpinBox->value();
}

int SphereParamsDialog::getThetaResolution() const
{
    return ui->thetaResolutionSpinBox->value();
}

int SphereParamsDialog::getPhiResolution() const
{
    return ui->phiResolutionSpinBox->value();
}

void SphereParamsDialog::setRadius(double radius)
{
    ui->radiusSpinBox->setValue(radius);
}

void SphereParamsDialog::setThetaResolution(int resolution)
{
    ui->thetaResolutionSpinBox->setValue(resolution);
}

void SphereParamsDialog::setPhiResolution(int resolution)
{
    ui->phiResolutionSpinBox->setValue(resolution);
}

void SphereParamsDialog::on_okButton_clicked()
{
    accept();
}

void SphereParamsDialog::on_cancleButton_clicked()
{
    reject();
}

void SphereParamsDialog::setOriginPoint(double x, double y, double z)
{
    originX = x;
    originY = y;
    originZ = z;
    originPointSet = true;
    
    // 更新按钮文本显示已选择
    ui->designated_point->setText(QString("指定点: (%1, %2, %3)").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2));
}

void SphereParamsDialog::getOriginPoint(double& x, double& y, double& z) const
{
    x = originX;
    y = originY;
    z = originZ;
}

void SphereParamsDialog::on_designated_point_clicked()
{
    if (originSnapKind_ >= 0) {
        emit requestPointSelectionWithSnap(originSnapKind_);
    } else {
        emit requestPointSelection();
    }
}

void SphereParamsDialog::on_applyButton_clicked()
{
    // 发送应用请求信号
    emit applyRequested();
}

void SphereParamsDialog::ensureShowResultButton()
{
    if (showResultButton_) return;
    showResultButton_ = new QPushButton(tr("显示结果"), this);
    showResultButton_->setObjectName(QStringLiteral("showResultButton"));
    if (auto* layout = findChild<QGridLayout*>("gridLayout_3")) {
        layout->addWidget(showResultButton_, 1, 0, 1, 4);
    }
    connect(showResultButton_, &QPushButton::clicked, this, &SphereParamsDialog::onShowResultButtonClicked);
}

void SphereParamsDialog::applyResultPreviewUiLock(bool locked)
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

void SphereParamsDialog::setResultPreviewActive(bool active)
{
    if (resultPreviewActive_ == active) return;
    resultPreviewActive_ = active;
    applyResultPreviewUiLock(active);
}

void SphereParamsDialog::onShowResultButtonClicked()
{
    if (resultPreviewActive_) {
        setResultPreviewActive(false);
        emit cancelPreviewRequested();
        return;
    }
    emit previewRequested();
}


