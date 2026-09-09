#include "sketch_polygon_dialog.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

static void styleTitleBar(QWidget* bar)
{
    bar->setMinimumHeight(32);
    bar->setStyleSheet(QStringLiteral("background-color: #2aa89a;"));
}

static QFrame* makeCollapsibleSection(const QString& title, QWidget* body, QWidget* parent)
{
    auto* wrap = new QFrame(parent);
    wrap->setFrameShape(QFrame::NoFrame);
    auto* lay = new QVBoxLayout(wrap);
    lay->setContentsMargins(0, 0, 0, 4);
    lay->setSpacing(2);

    auto* header = new QLabel(title, wrap);
    header->setStyleSheet(QStringLiteral(
        "QLabel { background: #e8f4f8; padding: 4px 8px; font-weight: bold; border: 1px solid #c0d8e0; }"));
    lay->addWidget(header);
    lay->addWidget(body);
    return wrap;
}

} // namespace

void SketchPolygonDialog::fillOriginSnapMenu(QToolButton* btn, int* kindStorage, QWidget* host)
{
    if (!btn || !kindStorage) return;
    auto* menu = new QMenu(host);
    auto addAct = [&](const QString& text, int kind) {
        QAction* act = menu->addAction(text);
        connect(act, &QAction::triggered, host, [btn, kindStorage, text, kind]() {
            *kindStorage = kind;
            btn->setText(text);
        });
    };
    addAct(QObject::tr("最近点"), 0);
    addAct(QObject::tr("端点"), 1);
    addAct(QObject::tr("中点"), 2);
    addAct(QObject::tr("交点"), 3);
    addAct(QObject::tr("圆心"), 4);
    addAct(QObject::tr("象限点"), 5);
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setToolTip(QObject::tr("点捕捉类型"));
    btn->setText(QStringLiteral("\u25be"));
}

SketchPolygonDialog::SketchPolygonDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(320);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 8);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    styleTitleBar(titleBar);
    auto* ht = new QHBoxLayout(titleBar);
    ht->setContentsMargins(8, 4, 4, 4);
    auto* tLab = new QLabel(tr("多边形"), titleBar);
    tLab->setStyleSheet(QStringLiteral("color: white; font-weight: bold;"));
    ht->addWidget(tLab);
    ht->addStretch();
    auto* tClose = new QToolButton(titleBar);
    tClose->setText(QStringLiteral("\u00d7"));
    tClose->setAutoRaise(true);
    connect(tClose, &QToolButton::clicked, this, &SketchPolygonDialog::onCloseClicked);
    ht->addWidget(tClose);
    root->addWidget(titleBar);

    // ---------- 中心点 ----------
    auto* centerBody = new QWidget(this);
    auto* centerLay = new QVBoxLayout(centerBody);
    centerLay->setContentsMargins(8, 4, 8, 4);

    rowCenter_ = new QFrame(this);
    rowCenter_->setFrameShape(QFrame::StyledPanel);
    auto* centerRowLay = new QHBoxLayout(rowCenter_);
    centerRowLay->setContentsMargins(4, 4, 4, 4);
    centerRowLay->addWidget(new QLabel(tr("指定点"), rowCenter_));
    centerEdit_ = new QLineEdit(rowCenter_);
    centerEdit_->setReadOnly(true);
    centerEdit_->setPlaceholderText(tr("（未指定）"));
    centerRowLay->addWidget(centerEdit_, 1);
    auto* pickCenterBtn = new QPushButton(QStringLiteral("+"), rowCenter_);
    pickCenterBtn->setFixedWidth(28);
    pickCenterBtn->setToolTip(tr("拾取点"));
    centerSnapBtn_ = new QToolButton(rowCenter_);
    centerSnapBtn_->setFixedWidth(24);
    fillOriginSnapMenu(centerSnapBtn_, &centerSnapKind_, this);
    centerRowLay->addWidget(pickCenterBtn);
    centerRowLay->addWidget(centerSnapBtn_);
    centerLay->addWidget(rowCenter_);
    root->addWidget(makeCollapsibleSection(tr("中心点"), centerBody, this));

    connect(pickCenterBtn, &QPushButton::clicked, this, &SketchPolygonDialog::pickCenterRequested);

    // ---------- 边 ----------
    auto* sideBody = new QWidget(this);
    auto* sideLay = new QHBoxLayout(sideBody);
    sideLay->setContentsMargins(8, 4, 8, 4);
    sideLay->addWidget(new QLabel(tr("边数"), sideBody));
    sideSpin_ = new QSpinBox(sideBody);
    sideSpin_->setRange(3, 720);
    sideSpin_->setValue(6);
    sideLay->addWidget(sideSpin_);
    sideLay->addStretch();
    root->addWidget(makeCollapsibleSection(tr("边"), sideBody, this));
    connect(sideSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { emit paramsChanged(); });

    // ---------- 大小 ----------
    auto* sizeBody = new QWidget(this);
    auto* sizeBodyLay = new QVBoxLayout(sizeBody);
    sizeBodyLay->setContentsMargins(8, 4, 8, 4);

    rowSize_ = new QFrame(this);
    rowSize_->setFrameShape(QFrame::StyledPanel);
    auto* sizeRowLay = new QHBoxLayout(rowSize_);
    sizeRowLay->setContentsMargins(4, 4, 4, 4);
    sizeRowLay->addWidget(new QLabel(tr("指定点"), rowSize_));
    sizeEdit_ = new QLineEdit(rowSize_);
    sizeEdit_->setReadOnly(true);
    sizeEdit_->setPlaceholderText(tr("（未指定）"));
    sizeRowLay->addWidget(sizeEdit_, 1);
    auto* pickSizeBtn = new QPushButton(QStringLiteral("+"), rowSize_);
    pickSizeBtn->setFixedWidth(28);
    pickSizeBtn->setToolTip(tr("拾取点"));
    sizeSnapBtn_ = new QToolButton(rowSize_);
    sizeSnapBtn_->setFixedWidth(24);
    fillOriginSnapMenu(sizeSnapBtn_, &sizeSnapKind_, this);
    sizeRowLay->addWidget(pickSizeBtn);
    sizeRowLay->addWidget(sizeSnapBtn_);
    sizeBodyLay->addWidget(rowSize_);
    connect(pickSizeBtn, &QPushButton::clicked, this, &SketchPolygonDialog::pickSizeRequested);

    auto* modeLay = new QHBoxLayout();
    modeLay->addWidget(new QLabel(tr("大小"), sizeBody));
    sizeModeCombo_ = new QComboBox(sizeBody);
    sizeModeCombo_->addItem(tr("内切圆半径"), InscribedRadius);
    sizeModeCombo_->addItem(tr("外接圆半径"), CircumscribedRadius);
    sizeModeCombo_->addItem(tr("边长"), SideLength);
    modeLay->addWidget(sizeModeCombo_, 1);
    sizeBodyLay->addLayout(modeLay);

    auto* sizeParamLay = new QHBoxLayout();
    sizeLockChk_ = new QCheckBox(sizeBody);
    sizeLockChk_->setToolTip(tr("锁定尺寸"));
    sizeParamLabel_ = new QLabel(tr("半径"), sizeBody);
    sizeSpin_ = new QDoubleSpinBox(sizeBody);
    sizeSpin_->setRange(0.0, 1e9);
    sizeSpin_->setDecimals(4);
    sizeSpin_->setValue(100.0);
    sizeSpin_->setSuffix(tr(" mm"));
    sizeParamLay->addWidget(sizeLockChk_);
    sizeParamLay->addWidget(sizeParamLabel_);
    sizeParamLay->addWidget(sizeSpin_, 1);
    sizeBodyLay->addLayout(sizeParamLay);

    auto* rotLay = new QHBoxLayout();
    rotLockChk_ = new QCheckBox(sizeBody);
    rotLockChk_->setToolTip(tr("锁定旋转"));
    rotLay->addWidget(rotLockChk_);
    rotLay->addWidget(new QLabel(tr("旋转"), sizeBody));
    rotSpin_ = new QDoubleSpinBox(sizeBody);
    rotSpin_->setRange(-360.0, 360.0);
    rotSpin_->setDecimals(2);
    rotSpin_->setSuffix(QStringLiteral(" \u00b0"));
    rotLay->addWidget(rotSpin_, 1);
    sizeBodyLay->addLayout(rotLay);

    root->addWidget(makeCollapsibleSection(tr("大小"), sizeBody, this));

    connect(sizeModeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &SketchPolygonDialog::onSizeModeIndexChanged);
    connect(sizeSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { emit paramsChanged(); });
    connect(rotSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { emit paramsChanged(); });
    connect(sizeLockChk_, &QCheckBox::toggled, this, [this](bool) { emit paramsChanged(); });
    connect(rotLockChk_, &QCheckBox::toggled, this, [this](bool) { emit paramsChanged(); });

    auto* btnClose = new QPushButton(tr("关闭"), this);
    btnClose->setStyleSheet(QStringLiteral("QPushButton { background: #2aa89a; color: white; padding: 6px 16px; }"));
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    root->addLayout(btnRow);
    connect(btnClose, &QPushButton::clicked, this, &SketchPolygonDialog::onCloseClicked);

    syncSizeRowLabels();
    highlightPickField(PickCenter);
}

int SketchPolygonDialog::sideCount() const
{
    return sideSpin_ ? sideSpin_->value() : 6;
}

SketchPolygonDialog::SizeMode SketchPolygonDialog::sizeMode() const
{
    if (!sizeModeCombo_) return InscribedRadius;
    return static_cast<SizeMode>(sizeModeCombo_->currentData().toInt());
}

double SketchPolygonDialog::sizeValue() const
{
    return sizeSpin_ ? sizeSpin_->value() : 0.0;
}

double SketchPolygonDialog::rotationDeg() const
{
    return rotSpin_ ? rotSpin_->value() : 0.0;
}

bool SketchPolygonDialog::sizeLocked() const
{
    return sizeLockChk_ && sizeLockChk_->isChecked();
}

bool SketchPolygonDialog::rotationLocked() const
{
    return rotLockChk_ && rotLockChk_->isChecked();
}

int SketchPolygonDialog::centerSnapKind() const
{
    return centerSnapKind_;
}

int SketchPolygonDialog::sizeSnapKind() const
{
    return sizeSnapKind_;
}

void SketchPolygonDialog::setCenterText(const QString& s)
{
    if (centerEdit_) centerEdit_->setText(s);
}

void SketchPolygonDialog::setSizeText(const QString& s)
{
    if (sizeEdit_) sizeEdit_->setText(s);
}

void SketchPolygonDialog::setSizeValue(double v)
{
    if (sizeSpin_) sizeSpin_->setValue(v);
}

void SketchPolygonDialog::setRotationDeg(double v)
{
    if (rotSpin_) rotSpin_->setValue(v);
}

void SketchPolygonDialog::highlightPickField(PickField field)
{
    highlightField_ = field;
    restylePickRows();
}

void SketchPolygonDialog::syncSizeRowLabels()
{
    if (!sizeParamLabel_) return;
    const bool sideLen = (sizeMode() == SideLength);
    sizeParamLabel_->setText(sideLen ? tr("长度") : tr("半径"));
    if (sizeSpin_) sizeSpin_->setSuffix(tr(" mm"));
}

void SketchPolygonDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchPolygonDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchPolygonDialog::onSizeModeIndexChanged(int index)
{
    Q_UNUSED(index);
    syncSizeRowLabels();
    emit sizeModeChanged(sizeMode());
    emit paramsChanged();
}

void SketchPolygonDialog::restylePickRows()
{
    const QString base = QStringLiteral(
        "QFrame { border: 1px solid #b0b0b0; border-radius: 4px; padding: 2px; background: palette(base); }");
    const QString hi = QStringLiteral(
        "QFrame { border: 2px solid #c9a012; border-radius: 4px; padding: 2px; background: #fff8e6; }"
        "QLabel { color: #1a1a1a; font-weight: 600; }"
        "QLineEdit { background: #ffffff; color: #1a1a1a; }");
    auto apply = [&](QFrame* row, PickField f) {
        if (!row) return;
        row->setStyleSheet((highlightField_ == f) ? hi : base);
    };
    apply(rowCenter_, PickCenter);
    apply(rowSize_, PickSize);
}

// ----- Value dialog -----

SketchPolygonValueDialog::SketchPolygonValueDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(2);

    auto* sizeRow = new QHBoxLayout();
    sizeLabel_ = new QLabel(tr("半径"), this);
    sizeEdit_ = new QLineEdit(this);
    sizeEdit_->setFixedWidth(72);
    auto* dv = new QDoubleValidator(sizeEdit_);
    dv->setNotation(QDoubleValidator::StandardNotation);
    sizeEdit_->setValidator(dv);
    sizeRow->addWidget(sizeLabel_);
    sizeRow->addWidget(sizeEdit_);
    root->addLayout(sizeRow);

    auto* rotRow = new QHBoxLayout();
    rotRow->addWidget(new QLabel(tr("旋转"), this));
    rotEdit_ = new QLineEdit(this);
    rotEdit_->setFixedWidth(72);
    rotEdit_->setValidator(new QDoubleValidator(rotEdit_));
    rotRow->addWidget(rotEdit_);
    root->addLayout(rotRow);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #f5f5f5; border: 1px solid #999; }"
        "QLineEdit:focus { background: #3399ff; color: white; selection-background-color: #0066cc; }"));

    hookLineEdit(sizeEdit_, 0);
    hookLineEdit(rotEdit_, 1);
}

void SketchPolygonValueDialog::setSizeMode(SketchPolygonDialog::SizeMode mode)
{
    sizeMode_ = mode;
    if (sizeLabel_) {
        sizeLabel_->setText(mode == SketchPolygonDialog::SideLength ? tr("长度") : tr("半径"));
    }
}

void SketchPolygonValueDialog::setSizeValue(double v)
{
    if (sizeEdit_) sizeEdit_->setText(QString::number(v, 'f', 3));
}

void SketchPolygonValueDialog::setRotationDeg(double v)
{
    if (rotEdit_) rotEdit_->setText(QString::number(v, 'f', 2));
}

double SketchPolygonValueDialog::sizeValue() const
{
    bool ok = false;
    const double val = sizeEdit_ ? sizeEdit_->text().trimmed().toDouble(&ok) : 0.0;
    return ok ? val : 0.0;
}

double SketchPolygonValueDialog::rotationDeg() const
{
    bool ok = false;
    const double val = rotEdit_ ? rotEdit_->text().trimmed().toDouble(&ok) : 0.0;
    return ok ? val : 0.0;
}

void SketchPolygonValueDialog::focusField(int index)
{
    focusedField_ = (index <= 0) ? 0 : 1;
    if (focusedField_ == 0 && sizeEdit_) {
        sizeEdit_->setFocus();
        sizeEdit_->selectAll();
    } else if (rotEdit_) {
        rotEdit_->setFocus();
        rotEdit_->selectAll();
    }
}

void SketchPolygonValueDialog::focusFirstFieldFromArrowKey()
{
    focusField(0);
}

void SketchPolygonValueDialog::focusNextFieldOnEnter()
{
    focusField(1);
}

void SketchPolygonValueDialog::switchFieldByVerticalArrow(int key)
{
    if (key == Qt::Key_Up || key == Qt::Key_Down) {
        focusField(focusedField_ == 0 ? 1 : 0);
    }
}

void SketchPolygonValueDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

bool SketchPolygonValueDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (obj == sizeEdit_) {
                focusNextFieldOnEnter();
                return true;
            }
            if (obj == rotEdit_) {
                emit valuesCommitted();
                return true;
            }
        }
        if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
            switchFieldByVerticalArrow(ke->key());
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void SketchPolygonValueDialog::hookLineEdit(QLineEdit* le, int fieldIndex)
{
    if (!le) return;
    le->installEventFilter(this);
    connect(le, &QLineEdit::editingFinished, this, [this, fieldIndex]() {
        Q_UNUSED(fieldIndex);
        emit valuesCommitted();
    });
}

void SketchPolygonValueDialog::onSizeEditingFinished()
{
    emit valuesCommitted();
}

void SketchPolygonValueDialog::onRotEditingFinished()
{
    emit valuesCommitted();
}
