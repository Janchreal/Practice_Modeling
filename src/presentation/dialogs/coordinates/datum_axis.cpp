#include "datum_axis.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QIcon makeReverseIcon(bool active)
{
    QPixmap pm(22, 22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor stroke = active ? QColor(180, 90, 20) : QColor(60, 60, 60);
    const QColor fill = active ? QColor(255, 210, 120) : QColor(230, 230, 230);
    p.setBrush(fill);
    p.setPen(QPen(stroke, 1.4));
    p.drawRoundedRect(QRectF(1.5, 1.5, 19.0, 19.0), 3.0, 3.0);

    p.setPen(QPen(stroke, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // 对角双向箭头
    p.drawLine(QPointF(6, 15), QPointF(16, 5));
    p.drawLine(QPointF(16, 5), QPointF(12, 5));
    p.drawLine(QPointF(16, 5), QPointF(16, 9));
    p.drawLine(QPointF(6, 15), QPointF(10, 15));
    p.drawLine(QPointF(6, 15), QPointF(6, 11));
    p.end();
    return QIcon(pm);
}

} // namespace

DatumAxisDialog::DatumAxisDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("基准轴"));
    setModal(false);
    resize(280, 160);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    axisCombo_ = new QComboBox(this);
    axisCombo_->addItem(tr("XC 轴"), static_cast<int>(AxisDirection::X));
    axisCombo_->addItem(tr("YC 轴"), static_cast<int>(AxisDirection::Y));
    axisCombo_->addItem(tr("ZC 轴"), static_cast<int>(AxisDirection::Z));
    root->addWidget(axisCombo_);

    auto* orientBox = new QGroupBox(tr("轴方位"), this);
    auto* orientLayout = new QHBoxLayout(orientBox);
    orientLayout->setContentsMargins(8, 6, 8, 6);
    auto* reverseLabel = new QLabel(tr("反向"), orientBox);
    reverseBtn_ = new QPushButton(orientBox);
    reverseBtn_->setFixedSize(28, 28);
    reverseBtn_->setIconSize(QSize(22, 22));
    reverseBtn_->setToolTip(tr("切换轴方向反向"));
    reverseBtn_->setCheckable(true);
    refreshReverseButtonStyle();
    orientLayout->addWidget(reverseLabel);
    orientLayout->addStretch(1);
    orientLayout->addWidget(reverseBtn_);
    root->addWidget(orientBox);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    root->addWidget(buttons);

    connect(axisCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DatumAxisDialog::parametersChanged);
    connect(reverseBtn_, &QPushButton::clicked, this, [this](bool checked) {
        setReversed(checked);
        emit parametersChanged();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AxisDirection DatumAxisDialog::axisDirection() const
{
    if (!axisCombo_) return AxisDirection::X;
    return static_cast<AxisDirection>(axisCombo_->currentData().toInt());
}

QString DatumAxisDialog::axisText() const
{
    return axisCombo_ ? axisCombo_->currentText() : QString();
}

void DatumAxisDialog::setReversed(bool on)
{
    reversed_ = on;
    if (reverseBtn_) {
        reverseBtn_->setChecked(on);
    }
    refreshReverseButtonStyle();
}

void DatumAxisDialog::refreshReverseButtonStyle()
{
    if (!reverseBtn_) return;
    reverseBtn_->setIcon(makeReverseIcon(reversed_));
    reverseBtn_->setStyleSheet(reversed_
        ? QStringLiteral("QPushButton { background-color: #ffe08a; border: 1px solid #c87820; border-radius: 3px; }"
                         "QPushButton:checked { background-color: #ffd060; }")
        : QStringLiteral("QPushButton { background-color: #f3f3f3; border: 1px solid #999; border-radius: 3px; }"));
}
