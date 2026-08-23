#include "filletdialog.h"
#include "ui_filletdialog.h"

#include <QComboBox>
#include <QGridLayout>

filletdialog::filletdialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::filletdialog)
{
    ui->setupUi(this);

    // 默认参数：先实现 G1 + 圆
    ui->comboBox->setCurrentIndex(0);   // G1（相切）
    ui->comboBox_2->setCurrentIndex(0); // 圆

    ui->doubleSpinBox->setDecimals(2);
    ui->doubleSpinBox->setMinimum(0.0);
    ui->doubleSpinBox->setMaximum(1e6);
    ui->doubleSpinBox->setSingleStep(0.1);
    ui->doubleSpinBox->setValue(0.3);
    ui->radius->setText(QStringLiteral("半径 1"));

    rhoLabel_ = new QLabel(QStringLiteral("Rho 1"), this);
    rhoSpinBox_ = new QDoubleSpinBox(this);
    rhoSpinBox_->setDecimals(3);
    // 参照 NX Conic Rho 常见范围：0<rho<2，rho=1 近似抛物线风格。
    rhoSpinBox_->setMinimum(0.05);
    rhoSpinBox_->setMaximum(1.95);
    rhoSpinBox_->setSingleStep(0.01);
    rhoSpinBox_->setValue(1.0);

    if (auto *layout = qobject_cast<QGridLayout*>(ui->groupBox->layout())) {
        layout->addWidget(rhoLabel_, 4, 0);
        layout->addWidget(rhoSpinBox_, 4, 1);
    }

    connect(ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateUiByContinuity();
        emit parametersChanged();
    });
    connect(ui->comboBox_2, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        emit parametersChanged();
    });
    connect(ui->doubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emit parametersChanged();
    });
    if (rhoSpinBox_) {
        connect(rhoSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            emit parametersChanged();
        });
    }
    updateUiByContinuity();

    setSelectedEdgeCount(0);

    connect(ui->Selected_Edge, &QPushButton::clicked, this, [this]() {
        emit edgeModeHintRequested();
    });
}

filletdialog::~filletdialog()
{
    delete ui;
}

QString filletdialog::continuityText() const
{
    return ui->comboBox->currentText();
}

QString filletdialog::shapeText() const
{
    return ui->comboBox_2->currentText();
}

double filletdialog::radiusValue() const
{
    return ui->doubleSpinBox->value();
}

void filletdialog::setRadiusValue(double v)
{
    if (ui->doubleSpinBox) ui->doubleSpinBox->setValue(v);
}

double filletdialog::rhoValue() const
{
    return rhoSpinBox_ ? rhoSpinBox_->value() : 1.0;
}

void filletdialog::setSelectedEdgeCount(int count)
{
    ui->Selected_Edge->setText(QStringLiteral("已选 %1 条").arg(count));
}

void filletdialog::updateUiByContinuity()
{
    const bool isG2 = ui->comboBox->currentText().startsWith(QStringLiteral("G2"));
    ui->shape->setVisible(!isG2);
    ui->comboBox_2->setVisible(!isG2);
    if (rhoLabel_) rhoLabel_->setVisible(isG2);
    if (rhoSpinBox_) rhoSpinBox_->setVisible(isG2);
}
