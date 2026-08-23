#include "chamferdialog.h"
#include "ui_chamferdialog.h"

#include <QComboBox>
#include <QDoubleSpinBox>

chamferdialog::chamferdialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::chamferdialog)
{
    ui->setupUi(this);

    ui->comboBox->setCurrentIndex(0); // 对称
    ui->doubleSpinBox->setDecimals(2);
    ui->doubleSpinBox->setMinimum(0.0);
    ui->doubleSpinBox->setMaximum(1e6);
    ui->doubleSpinBox->setSingleStep(0.1);
    ui->doubleSpinBox->setValue(0.3);

    if (ui->distance2SpinBox) {
        ui->distance2SpinBox->setDecimals(2);
        ui->distance2SpinBox->setMinimum(0.0);
        ui->distance2SpinBox->setMaximum(1e6);
        ui->distance2SpinBox->setSingleStep(0.1);
        ui->distance2SpinBox->setValue(0.3);
    }

    setSelectedEdgeCount(0);

    connect(ui->selectededge, &QPushButton::clicked, this, [this]() {
        emit edgeModeHintRequested();
    });
    connect(ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        emit parametersChanged();
        // 非对称模式才显示第二距离
        if (ui->distance2SpinBox) {
            const bool asym = (sectionText() == QStringLiteral("非对称"));
            ui->distance2SpinBox->setVisible(asym);
            // label 复用（靠近 doubleSpinBox 的“距离”标签）目前不做额外控制：用隐藏第二 spinbox 即可
        }
    });
    connect(ui->doubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emit parametersChanged();
    });
    if (ui->distance2SpinBox) {
        connect(ui->distance2SpinBox,
                qOverload<double>(&QDoubleSpinBox::valueChanged),
                this,
                [this](double) {
                    emit parametersChanged();
                });
    }

    // 初始状态：根据 combobox 设置第二距离可见性
    if (ui->distance2SpinBox) {
        const bool asym = (sectionText() == QStringLiteral("非对称"));
        ui->distance2SpinBox->setVisible(asym);
    }
}

chamferdialog::~chamferdialog()
{
    delete ui;
}

QString chamferdialog::sectionText() const
{
    return ui->comboBox->currentText();
}

double chamferdialog::distanceValue() const
{
    return ui->doubleSpinBox->value();
}

double chamferdialog::distance1Value() const
{
    return ui->doubleSpinBox->value();
}

double chamferdialog::distance2Value() const
{
    if (!ui->distance2SpinBox) return ui->doubleSpinBox->value();
    return ui->distance2SpinBox->value();
}

void chamferdialog::setDistance1Value(double v)
{
    if (ui->doubleSpinBox) ui->doubleSpinBox->setValue(v);
}

void chamferdialog::setDistance2Value(double v)
{
    if (ui->distance2SpinBox) ui->distance2SpinBox->setValue(v);
}

void chamferdialog::setSelectedEdgeCount(int count)
{
    ui->selectededge->setText(QStringLiteral("已选 %1 条").arg(count));
}
