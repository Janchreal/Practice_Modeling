#include "sketchellipsedialog.h"

#include <QAction>
#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace {

static void styleTitleBar(QWidget* bar)
{
    bar->setMinimumHeight(32);
    bar->setStyleSheet(QStringLiteral("background-color: #2aa89a;"));
}

static QLabel* makeSectionHeader(const QString& title, QWidget* parent)
{
    auto* header = new QLabel(title, parent);
    header->setStyleSheet(QStringLiteral(
        "QLabel { background: #e8f4f8; padding: 4px 8px; font-weight: bold; border: 1px solid #c0d8e0; }"));
    return header;
}

} // namespace

void SketchEllipseDialog::fillOriginSnapMenu(QToolButton* btn, int* kindStorage, QWidget* host)
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

QFrame* SketchEllipseDialog::makePickRow(const QString& label, QLineEdit** editOut, QToolButton** snapBtnOut,
                                         int* snapKindOut, QPushButton** pickBtnOut)
{
    auto* row = new QFrame(this);
    row->setFrameShape(QFrame::StyledPanel);
    auto* lay = new QHBoxLayout(row);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(new QLabel(label, row));
    auto* ed = new QLineEdit(row);
    ed->setReadOnly(true);
    ed->setPlaceholderText(tr("（未指定）"));
    lay->addWidget(ed, 1);
    auto* pickBtn = new QPushButton(QStringLiteral("+"), row);
    pickBtn->setFixedWidth(28);
    pickBtn->setToolTip(tr("拾取点"));
    auto* snapBtn = new QToolButton(row);
    snapBtn->setFixedWidth(24);
    fillOriginSnapMenu(snapBtn, snapKindOut, this);
    lay->addWidget(pickBtn);
    lay->addWidget(snapBtn);
    *editOut = ed;
    *snapBtnOut = snapBtn;
    *pickBtnOut = pickBtn;
    return row;
}

SketchEllipseDialog::SketchEllipseDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(340);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 8);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    styleTitleBar(titleBar);
    auto* ht = new QHBoxLayout(titleBar);
    ht->setContentsMargins(8, 4, 4, 4);
    auto* tLab = new QLabel(tr("椭圆"), titleBar);
    tLab->setStyleSheet(QStringLiteral("color: white; font-weight: bold;"));
    ht->addWidget(tLab);
    ht->addStretch();
    auto* tClose = new QToolButton(titleBar);
    tClose->setText(QStringLiteral("\u00d7"));
    tClose->setAutoRaise(true);
    connect(tClose, &QToolButton::clicked, this, &SketchEllipseDialog::onCloseClicked);
    ht->addWidget(tClose);
    root->addWidget(titleBar);

    auto* body = new QWidget(this);
    auto* vb = new QVBoxLayout(body);
    vb->setContentsMargins(8, 6, 8, 4);

    vb->addWidget(makeSectionHeader(tr("中心"), body));
    QPushButton* pickCenter = nullptr;
    rowCenter_ = makePickRow(tr("指定点"), &centerEdit_, &centerSnapBtn_, &centerSnapKind_, &pickCenter);
    vb->addWidget(rowCenter_);
    connect(pickCenter, &QPushButton::clicked, this, &SketchEllipseDialog::pickCenterRequested);

    vb->addWidget(makeSectionHeader(tr("大半径"), body));
    QPushButton* pickMajor = nullptr;
    rowMajor_ = makePickRow(tr("指定点"), &majorPickEdit_, &majorSnapBtn_, &majorSnapKind_, &pickMajor);
    vb->addWidget(rowMajor_);
    auto* majorValLay = new QHBoxLayout();
    majorValLay->addWidget(new QLabel(tr("大半径"), body));
    majorSpin_ = new QDoubleSpinBox(body);
    majorSpin_->setRange(0.0001, 1e9);
    majorSpin_->setDecimals(4);
    majorSpin_->setValue(5.0);
    majorSpin_->setSuffix(tr(" mm"));
    majorValLay->addWidget(majorSpin_, 1);
    vb->addLayout(majorValLay);
    connect(pickMajor, &QPushButton::clicked, this, &SketchEllipseDialog::pickMajorRequested);

    vb->addWidget(makeSectionHeader(tr("小半径"), body));
    QPushButton* pickMinor = nullptr;
    rowMinor_ = makePickRow(tr("指定点"), &minorPickEdit_, &minorSnapBtn_, &minorSnapKind_, &pickMinor);
    vb->addWidget(rowMinor_);
    auto* minorValLay = new QHBoxLayout();
    minorValLay->addWidget(new QLabel(tr("小半径"), body));
    minorSpin_ = new QDoubleSpinBox(body);
    minorSpin_->setRange(0.0001, 1e9);
    minorSpin_->setDecimals(4);
    minorSpin_->setValue(3.0);
    minorSpin_->setSuffix(tr(" mm"));
    minorValLay->addWidget(minorSpin_, 1);
    vb->addLayout(minorValLay);
    connect(pickMinor, &QPushButton::clicked, this, &SketchEllipseDialog::pickMinorRequested);

    vb->addWidget(makeSectionHeader(tr("旋转"), body));
    auto* rotLay = new QHBoxLayout();
    rotLay->addWidget(new QLabel(tr("角度"), body));
    rotSpin_ = new QDoubleSpinBox(body);
    rotSpin_->setRange(-360.0, 360.0);
    rotSpin_->setDecimals(2);
    rotSpin_->setSuffix(QStringLiteral(" \u00b0"));
    rotLay->addWidget(rotSpin_, 1);
    vb->addLayout(rotLay);

    root->addWidget(body);

    connect(majorSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { emit paramsChanged(); });
    connect(minorSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { emit paramsChanged(); });
    connect(rotSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { emit paramsChanged(); });

    auto* btnRow = new QHBoxLayout();
    auto* btnOk = new QPushButton(tr("确定"), this);
    auto* btnApply = new QPushButton(tr("应用"), this);
    auto* btnCancel = new QPushButton(tr("取消"), this);
    btnOk->setDefault(true);
    btnRow->addWidget(btnOk);
    btnRow->addWidget(btnApply);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    root->addLayout(btnRow);
    connect(btnOk, &QPushButton::clicked, this, &SketchEllipseDialog::onOk);
    connect(btnApply, &QPushButton::clicked, this, &SketchEllipseDialog::onApply);
    connect(btnCancel, &QPushButton::clicked, this, &SketchEllipseDialog::onCloseClicked);

    highlightPickField(PickCenter);
}

double SketchEllipseDialog::majorRadius() const
{
    return majorSpin_ ? majorSpin_->value() : 5.0;
}

double SketchEllipseDialog::minorRadius() const
{
    return minorSpin_ ? minorSpin_->value() : 3.0;
}

double SketchEllipseDialog::rotationDeg() const
{
    return rotSpin_ ? rotSpin_->value() : 0.0;
}

int SketchEllipseDialog::centerSnapKind() const { return centerSnapKind_; }
int SketchEllipseDialog::majorSnapKind() const { return majorSnapKind_; }
int SketchEllipseDialog::minorSnapKind() const { return minorSnapKind_; }

void SketchEllipseDialog::setCenterText(const QString& s)
{
    if (centerEdit_) centerEdit_->setText(s);
}

void SketchEllipseDialog::setMajorPickText(const QString& s)
{
    if (majorPickEdit_) majorPickEdit_->setText(s);
}

void SketchEllipseDialog::setMinorPickText(const QString& s)
{
    if (minorPickEdit_) minorPickEdit_->setText(s);
}

void SketchEllipseDialog::setMajorRadius(double v)
{
    if (!majorSpin_) return;
    const QSignalBlocker blocker(majorSpin_);
    majorSpin_->setValue(v);
}

void SketchEllipseDialog::setMinorRadius(double v)
{
    if (!minorSpin_) return;
    const QSignalBlocker blocker(minorSpin_);
    minorSpin_->setValue(v);
}

void SketchEllipseDialog::setRadii(double majorR, double minorR)
{
    if (!majorSpin_ || !minorSpin_) return;
    const QSignalBlocker blockerMajor(majorSpin_);
    const QSignalBlocker blockerMinor(minorSpin_);
    majorSpin_->setValue(majorR);
    minorSpin_->setValue(minorR);
}

void SketchEllipseDialog::setRotationDeg(double v)
{
    if (!rotSpin_) return;
    const QSignalBlocker blocker(rotSpin_);
    rotSpin_->setValue(v);
}

void SketchEllipseDialog::highlightPickField(PickField field)
{
    highlightField_ = field;
    restylePickRows();
}

void SketchEllipseDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchEllipseDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchEllipseDialog::onApply()
{
    emit applyRequested();
}

void SketchEllipseDialog::onOk()
{
    emit okRequested();
}

void SketchEllipseDialog::restylePickRows()
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
    apply(rowMajor_, PickMajor);
    apply(rowMinor_, PickMinor);
}

// ----- Angle dialog -----

SketchEllipseAngleDialog::SketchEllipseAngleDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::ClickFocus);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->addWidget(new QLabel(tr("角度"), this));
    angleEdit_ = new QLineEdit(this);
    angleEdit_->setFixedWidth(72);
    auto* dv = new QDoubleValidator(angleEdit_);
    dv->setNotation(QDoubleValidator::StandardNotation);
    angleEdit_->setValidator(dv);
    root->addWidget(angleEdit_);
    angleEdit_->installEventFilter(this);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #f5f5f5; border: 1px solid #999; }"
        "QLineEdit:focus { background: #3399ff; color: white; selection-background-color: #0066cc; }"));

    connect(angleEdit_, &QLineEdit::editingFinished, this, [this]() { emit angleCommitted(); });
    connect(angleEdit_, &QLineEdit::returnPressed, this, [this]() { emit angleCommitted(); });
}

void SketchEllipseAngleDialog::setRotationDeg(double v)
{
    if (angleEdit_) angleEdit_->setText(QString::number(v, 'f', 2));
}

double SketchEllipseAngleDialog::rotationDeg() const
{
    bool ok = false;
    const double val = angleEdit_ ? angleEdit_->text().trimmed().toDouble(&ok) : 0.0;
    return ok ? val : 0.0;
}

void SketchEllipseAngleDialog::focusAngleField()
{
    if (angleEdit_) {
        angleEdit_->setFocus();
        angleEdit_->selectAll();
    }
}

bool SketchEllipseAngleDialog::angleFieldHasFocus() const
{
    return angleEdit_ && angleEdit_->hasFocus();
}

void SketchEllipseAngleDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

bool SketchEllipseAngleDialog::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    Q_UNUSED(event);
    return QDialog::eventFilter(obj, event);
}
