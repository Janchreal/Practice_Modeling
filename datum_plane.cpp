#include "datum_plane.h"
#include "ui_datum_plane.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>

datum_plane::datum_plane(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::datum_plane)
{
    ui->setupUi(this);

    // 偏置值：默认隐藏，勾选时显示
    if (ui->doubleSpinBox_BiasValue) {
        ui->doubleSpinBox_BiasValue->setVisible(false);
        connect(ui->doubleSpinBox_BiasValue, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &datum_plane::emitParametersChanged);
    }
    if (ui->checkBox_Bias) {
        connect(ui->checkBox_Bias, &QCheckBox::toggled, this, [this](bool checked) {
            if (ui->doubleSpinBox_BiasValue) {
                ui->doubleSpinBox_BiasValue->setVisible(checked);
            }
            emitParametersChanged();
        });
    }

    // 模式变化时：动态刷新“定义平面的对象”一行，并触发预览
    if (ui->comboBox) {
        connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
            refreshDefineObjectText();
            emitParametersChanged();
        });
    }
    refreshDefineObjectText();
}

void datum_plane::emitParametersChanged()
{
    emit parametersChanged();
}

datum_plane::~datum_plane()
{
    delete ui;
}

void datum_plane::setCapturedPoints(const QList<gp_Pnt>& points)
{
    capturedPoints_ = points;
    refreshDefineObjectText();
}

QString datum_plane::modeText() const
{
    return ui && ui->comboBox ? ui->comboBox->currentText() : QString();
}

bool datum_plane::isOffsetEnabled() const
{
    return ui && ui->checkBox_Bias ? ui->checkBox_Bias->isChecked() : false;
}

double datum_plane::offsetValue() const
{
    return ui && ui->doubleSpinBox_BiasValue ? ui->doubleSpinBox_BiasValue->value() : 0.0;
}

void datum_plane::refreshDefineObjectText()
{
    if (!ui || !ui->Define_PlanerObject) return;

    const QString mode = modeText();
    auto fmtP = [](const gp_Pnt& p) {
        return QString("(%1, %2, %3)")
            .arg(p.X(), 0, 'f', 3)
            .arg(p.Y(), 0, 'f', 3)
            .arg(p.Z(), 0, 'f', 3);
    };

    QString text;
    if (mode == "YC-ZC平面" || mode == "XC-ZC平面" || mode == "XC-YC平面") {
        text = tr("要定义平面的对象：全局坐标系平面（无需选择点）");
    } else if (capturedPoints_.isEmpty()) {
        text = tr("要定义平面的对象：当前未捕捉到点，请先使用捕捉点捕捉所需点。");
    } else {
        // 显示最近捕捉的几个点（动态变化）
        const int n = capturedPoints_.size();
        const int showN = qMin(5, n);
        QStringList ps;
        for (int i = n - showN; i < n; ++i) {
            ps << fmtP(capturedPoints_[i]);
        }
        text = tr("要定义平面的对象（最近 %1 个捕捉点）：\n%2").arg(showN).arg(ps.join("\n"));
    }

    ui->Define_PlanerObject->setText(text);
}
