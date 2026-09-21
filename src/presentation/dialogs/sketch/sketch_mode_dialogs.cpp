#include "sketch_mode_dialogs.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

static void styleTitleBar(QWidget* bar)
{
    bar->setMinimumHeight(32);
    bar->setStyleSheet(QStringLiteral("background-color: #2aa89a;"));
}

static void styleSelButton(QToolButton* b, bool on, int minHeight = 48)
{
    const QString base = QStringLiteral(
        "QToolButton { border: 1px solid #ccc; border-radius: 4px; background: %1; font-weight: bold; "
        "min-width: 48px; min-height: %2px; padding: 4px 8px; }");
    b->setStyleSheet(base.arg(on ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff"))
                         .arg(minHeight));
}

} // namespace

SketchRectangleModeDialog::SketchRectangleModeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(300);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 8);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    styleTitleBar(titleBar);
    auto* ht = new QHBoxLayout(titleBar);
    ht->setContentsMargins(8, 4, 4, 4);
    auto* tClose = new QToolButton(titleBar);
    tClose->setText(QStringLiteral("\u00d7"));
    tClose->setAutoRaise(true);
    connect(tClose, &QToolButton::clicked, this, &SketchRectangleModeDialog::onCloseClicked);
    auto* tLab = new QLabel(tr("矩形"), titleBar);
    tLab->setStyleSheet(QStringLiteral("color: white; font-weight: bold;"));
    ht->addWidget(tLab);
    ht->addStretch();
    ht->addWidget(tClose);
    root->addWidget(titleBar);

    auto* body = new QWidget(this);
    auto* vb = new QVBoxLayout(body);
    vb->setContentsMargins(10, 8, 10, 4);

    vb->addWidget(new QLabel(tr("矩形方法"), body));
    auto* row = new QHBoxLayout();
    btnTwo_ = new QToolButton(body);
    btnTwo_->setCheckable(true);
    btnTwo_->setText(tr("两点"));
    btnThree_ = new QToolButton(body);
    btnThree_->setCheckable(true);
    btnThree_->setText(tr("三点"));
    btnCenter_ = new QToolButton(body);
    btnCenter_->setCheckable(true);
    btnCenter_->setText(tr("中心"));
    row->addWidget(btnTwo_);
    row->addWidget(btnThree_);
    row->addWidget(btnCenter_);
    vb->addLayout(row);

    connect(btnTwo_, &QToolButton::toggled, this, &SketchRectangleModeDialog::onM0);
    connect(btnThree_, &QToolButton::toggled, this, &SketchRectangleModeDialog::onM1);
    connect(btnCenter_, &QToolButton::toggled, this, &SketchRectangleModeDialog::onM2);

    setMethod(TwoDiagonal);
    root->addWidget(body);
}

SketchRectangleModeDialog::Method SketchRectangleModeDialog::method() const
{
    return method_;
}

void SketchRectangleModeDialog::setMethod(Method m)
{
    method_ = m;
    updating_ = true;
    btnTwo_->setChecked(m == TwoDiagonal);
    btnThree_->setChecked(m == ThreePoint);
    btnCenter_->setChecked(m == FromCenter);
    updating_ = false;
    syncButtons();
}

void SketchRectangleModeDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchRectangleModeDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchRectangleModeDialog::onM0(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnThree_->setChecked(false);
        btnCenter_->setChecked(false);
        updating_ = false;
        method_ = TwoDiagonal;
        syncButtons();
        emit methodChanged(TwoDiagonal);
    } else if (!btnThree_->isChecked() && !btnCenter_->isChecked()) {
        updating_ = true;
        btnTwo_->setChecked(true);
        updating_ = false;
    }
}

void SketchRectangleModeDialog::onM1(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnTwo_->setChecked(false);
        btnCenter_->setChecked(false);
        updating_ = false;
        method_ = ThreePoint;
        syncButtons();
        emit methodChanged(ThreePoint);
    } else if (!btnTwo_->isChecked() && !btnCenter_->isChecked()) {
        updating_ = true;
        btnThree_->setChecked(true);
        updating_ = false;
    }
}

void SketchRectangleModeDialog::onM2(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnTwo_->setChecked(false);
        btnThree_->setChecked(false);
        updating_ = false;
        method_ = FromCenter;
        syncButtons();
        emit methodChanged(FromCenter);
    } else if (!btnTwo_->isChecked() && !btnThree_->isChecked()) {
        updating_ = true;
        btnCenter_->setChecked(true);
        updating_ = false;
    }
}

void SketchRectangleModeDialog::syncButtons()
{
    styleSelButton(btnTwo_, method_ == TwoDiagonal);
    styleSelButton(btnThree_, method_ == ThreePoint);
    styleSelButton(btnCenter_, method_ == FromCenter);
}

// ----- Circle -----

SketchCircleModeDialog::SketchCircleModeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(300);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 8);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    styleTitleBar(titleBar);
    auto* ht = new QHBoxLayout(titleBar);
    ht->setContentsMargins(8, 4, 4, 4);
    auto* tClose = new QToolButton(titleBar);
    tClose->setText(QStringLiteral("\u00d7"));
    tClose->setAutoRaise(true);
    connect(tClose, &QToolButton::clicked, this, &SketchCircleModeDialog::onCloseClicked);
    auto* tLab = new QLabel(tr("圆"), titleBar);
    tLab->setStyleSheet(QStringLiteral("color: white; font-weight: bold;"));
    ht->addWidget(tLab);
    ht->addStretch();
    ht->addWidget(tClose);
    root->addWidget(titleBar);

    auto* body = new QWidget(this);
    auto* vb = new QVBoxLayout(body);
    vb->setContentsMargins(10, 8, 10, 4);
    vb->addWidget(new QLabel(tr("圆方法"), body));
    auto* row = new QHBoxLayout();
    btnCenter_ = new QToolButton(body);
    btnCenter_->setCheckable(true);
    btnCenter_->setText(tr("圆心"));
    btnThree_ = new QToolButton(body);
    btnThree_->setCheckable(true);
    btnThree_->setText(tr("两点半径"));
    row->addWidget(btnCenter_);
    row->addWidget(btnThree_);
    vb->addLayout(row);

    connect(btnCenter_, &QToolButton::toggled, this, &SketchCircleModeDialog::onCenter);
    connect(btnThree_, &QToolButton::toggled, this, &SketchCircleModeDialog::onThree);

    setMethod(CenterRadius);
    root->addWidget(body);
}

SketchCircleModeDialog::Method SketchCircleModeDialog::method() const
{
    return method_;
}

void SketchCircleModeDialog::setMethod(Method m)
{
    method_ = m;
    updating_ = true;
    btnCenter_->setChecked(m == CenterRadius);
    btnThree_->setChecked(m == TwoPointRadius);
    updating_ = false;
    syncButtons();
}

void SketchCircleModeDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchCircleModeDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchCircleModeDialog::onCenter(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnThree_->setChecked(false);
        updating_ = false;
        method_ = CenterRadius;
        syncButtons();
        emit methodChanged(CenterRadius);
    } else if (!btnThree_->isChecked()) {
        updating_ = true;
        btnCenter_->setChecked(true);
        updating_ = false;
    }
}

void SketchCircleModeDialog::onThree(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnCenter_->setChecked(false);
        updating_ = false;
        method_ = TwoPointRadius;
        syncButtons();
        emit methodChanged(TwoPointRadius);
    } else if (!btnCenter_->isChecked()) {
        updating_ = true;
        btnThree_->setChecked(true);
        updating_ = false;
    }
}

void SketchCircleModeDialog::syncButtons()
{
    styleSelButton(btnCenter_, method_ == CenterRadius);
    styleSelButton(btnThree_, method_ == TwoPointRadius);
}

// ----- Arc -----

SketchArcModeDialog::SketchArcModeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(280);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 8);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    styleTitleBar(titleBar);
    auto* ht = new QHBoxLayout(titleBar);
    ht->setContentsMargins(8, 4, 4, 4);
    auto* tClose = new QToolButton(titleBar);
    tClose->setText(QStringLiteral("\u00d7"));
    tClose->setAutoRaise(true);
    connect(tClose, &QToolButton::clicked, this, &SketchArcModeDialog::onCloseClicked);
    auto* tLab = new QLabel(tr("圆弧"), titleBar);
    tLab->setStyleSheet(QStringLiteral("color: white; font-weight: bold;"));
    ht->addWidget(tLab);
    ht->addStretch();
    ht->addWidget(tClose);
    root->addWidget(titleBar);

    auto* body = new QWidget(this);
    auto* vb = new QVBoxLayout(body);
    vb->setContentsMargins(10, 6, 10, 4);
    vb->setSpacing(6);
    vb->addWidget(new QLabel(tr("圆弧方法"), body));
    auto* row = new QHBoxLayout();
    row->setSpacing(8);
    btnThree_ = new QToolButton(body);
    btnThree_->setCheckable(true);
    btnThree_->setText(tr("三点"));
    btnThree_->setFixedHeight(38);
    btnCenterEndpoint_ = new QToolButton(body);
    btnCenterEndpoint_->setCheckable(true);
    btnCenterEndpoint_->setText(tr("中心端点"));
    btnCenterEndpoint_->setFixedHeight(38);
    row->addWidget(btnThree_);
    row->addWidget(btnCenterEndpoint_);
    vb->addLayout(row);

    connect(btnThree_, &QToolButton::toggled, this, &SketchArcModeDialog::onThree);
    connect(btnCenterEndpoint_, &QToolButton::toggled, this, &SketchArcModeDialog::onCenterEndpoint);

    setMethod(ThreePoint);
    root->addWidget(body);
}

SketchArcModeDialog::Method SketchArcModeDialog::method() const
{
    return method_;
}

void SketchArcModeDialog::setMethod(Method m)
{
    method_ = m;
    updating_ = true;
    btnThree_->setChecked(m == ThreePoint);
    btnCenterEndpoint_->setChecked(m == CenterEndpoint);
    updating_ = false;
    syncButtons();
}

void SketchArcModeDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchArcModeDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchArcModeDialog::onThree(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnCenterEndpoint_->setChecked(false);
        updating_ = false;
        method_ = ThreePoint;
        syncButtons();
        emit methodChanged(ThreePoint);
    } else if (!btnCenterEndpoint_->isChecked()) {
        updating_ = true;
        btnThree_->setChecked(true);
        updating_ = false;
    }
}

void SketchArcModeDialog::onCenterEndpoint(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        btnThree_->setChecked(false);
        updating_ = false;
        method_ = CenterEndpoint;
        syncButtons();
        emit methodChanged(CenterEndpoint);
    } else if (!btnThree_->isChecked()) {
        updating_ = true;
        btnCenterEndpoint_->setChecked(true);
        updating_ = false;
    }
}

void SketchArcModeDialog::syncButtons()
{
    styleSelButton(btnThree_, method_ == ThreePoint, 32);
    styleSelButton(btnCenterEndpoint_, method_ == CenterEndpoint, 32);
}
