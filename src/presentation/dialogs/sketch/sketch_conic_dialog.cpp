#include "sketch_conic_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SketchConicDialog::SketchConicDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("二次曲线"));
    setModal(false);

    auto* root = new QVBoxLayout(this);

    auto makeRow = [&](const QString& title, QLineEdit** editOut, QComboBox** snapOut, QPushButton** pickOut,
                       QFrame** rowOut) {
        auto* row = new QFrame(this);
        row->setFrameShape(QFrame::StyledPanel);
        auto* lay = new QHBoxLayout(row);
        lay->addWidget(new QLabel(title, row));
        auto* ed = new QLineEdit(row);
        ed->setReadOnly(true);
        ed->setPlaceholderText(tr("（未指定）"));
        lay->addWidget(ed, 1);
        auto* cb = new QComboBox(row);
        fillSnapCombo(cb);
        lay->addWidget(cb);
        auto* pick = new QPushButton(tr("拾取"), row);
        lay->addWidget(pick);
        *editOut = ed;
        *snapOut = cb;
        *pickOut = pick;
        *rowOut = row;
        root->addWidget(row);
    };

    QPushButton* pick0 = nullptr;
    QPushButton* pick1 = nullptr;
    QPushButton* pick2 = nullptr;
    makeRow(tr("起点"), &startEdit_, &startSnap_, &pick0, &rowStart_);
    makeRow(tr("终点"), &endEdit_, &endSnap_, &pick1, &rowEnd_);
    makeRow(tr("控制点"), &ctrlEdit_, &ctrlSnap_, &pick2, &rowCtrl_);

    auto* rhoLay = new QHBoxLayout();
    rhoLay->addWidget(new QLabel(tr("Rho"), this));
    rhoSpin_ = new QDoubleSpinBox(this);
    rhoSpin_->setRange(0.05, 0.95);
    rhoSpin_->setSingleStep(0.05);
    rhoSpin_->setDecimals(2);
    rhoSpin_->setValue(0.5);
    rhoLay->addWidget(rhoSpin_);
    rhoLay->addStretch();
    root->addLayout(rhoLay);

    previewChk_ = new QCheckBox(tr("预览"), this);
    previewChk_->setChecked(true);
    root->addWidget(previewChk_);

    auto* btnRow = new QHBoxLayout();
    btnApply_ = new QPushButton(tr("应用"), this);
    btnOk_ = new QPushButton(tr("确定"), this);
    auto* btnClose = new QPushButton(tr("关闭"), this);
    btnRow->addWidget(btnApply_);
    btnRow->addWidget(btnOk_);
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    root->addLayout(btnRow);

    connect(pick0, &QPushButton::clicked, this, &SketchConicDialog::pickStartRequested);
    connect(pick1, &QPushButton::clicked, this, &SketchConicDialog::pickEndRequested);
    connect(pick2, &QPushButton::clicked, this, &SketchConicDialog::pickControlRequested);
    connect(rhoSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SketchConicDialog::emitRhoFromSpin);
    connect(previewChk_, &QCheckBox::toggled, this, &SketchConicDialog::previewToggled);
    connect(btnApply_, &QPushButton::clicked, this, &SketchConicDialog::onApply);
    connect(btnOk_, &QPushButton::clicked, this, &SketchConicDialog::onOk);
    connect(btnClose, &QPushButton::clicked, this, [this]() { reject(); });

    setOkApplyEnabled(false);
    restyleRows();
}

void SketchConicDialog::fillSnapCombo(QComboBox* cb)
{
    if (!cb) return;
    cb->clear();
    cb->addItem(tr("综合"), 0);
    cb->addItem(tr("端点"), 1);
    cb->addItem(tr("中点"), 2);
    cb->addItem(tr("交点"), 3);
    cb->addItem(tr("圆心"), 4);
    cb->addItem(tr("象限点"), 5);
    cb->setCurrentIndex(1);
}

double SketchConicDialog::rho() const
{
    return rhoSpin_ ? rhoSpin_->value() : 0.5;
}

bool SketchConicDialog::previewEnabled() const
{
    return previewChk_ && previewChk_->isChecked();
}

int SketchConicDialog::startSnapKind() const
{
    if (!startSnap_) return 1;
    return startSnap_->currentData().toInt();
}

int SketchConicDialog::endSnapKind() const
{
    if (!endSnap_) return 1;
    return endSnap_->currentData().toInt();
}

int SketchConicDialog::controlSnapKind() const
{
    if (!ctrlSnap_) return 1;
    return ctrlSnap_->currentData().toInt();
}

void SketchConicDialog::setOkApplyEnabled(bool on)
{
    if (btnApply_) btnApply_->setEnabled(on);
    if (btnOk_) btnOk_->setEnabled(on);
}

void SketchConicDialog::setStartText(const QString& s)
{
    if (startEdit_) startEdit_->setText(s);
}

void SketchConicDialog::setEndText(const QString& s)
{
    if (endEdit_) endEdit_->setText(s);
}

void SketchConicDialog::setControlText(const QString& s)
{
    if (ctrlEdit_) ctrlEdit_->setText(s);
}

void SketchConicDialog::highlightPickField(int field)
{
    highlightField_ = field;
    restyleRows();
}

void SketchConicDialog::restyleRows()
{
    // 未选中：浅灰边框即可（避免与系统主题抢色）
    const QString base = QStringLiteral(
        "QFrame { border: 1px solid #b0b0b0; border-radius: 4px; padding: 2px; background: palette(base); }");
    // 当前拾取行：浅色底 + 金色描边，保证「起点/终点/控制点」等标签字可读
    const QString hi = QStringLiteral(
        "QFrame {"
        "  border: 2px solid #c9a012;"
        "  border-radius: 4px;"
        "  padding: 2px;"
        "  background: #fff8e6;"
        "}"
        "QLabel { color: #1a1a1a; font-weight: 600; }"
        "QLineEdit { background: #ffffff; color: #1a1a1a; }"
        "QComboBox { color: #1a1a1a; }"
        "QComboBox QAbstractItemView { background: #ffffff; color: #1a1a1a; }"
        "QPushButton { color: #1a1a1a; }");
    auto apply = [&](QFrame* row, int idx) {
        if (!row) return;
        row->setStyleSheet((highlightField_ == idx) ? hi : base);
    };
    apply(rowStart_, 0);
    apply(rowEnd_, 1);
    apply(rowCtrl_, 2);
}

void SketchConicDialog::onApply()
{
    emit applyRequested();
}

void SketchConicDialog::onOk()
{
    emit applyRequested();
}

void SketchConicDialog::emitRhoFromSpin(double v)
{
    emit rhoValueChanged(v);
}
