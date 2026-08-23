#include "sketchmodedialogs.h"

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

static void styleSelButton(QToolButton* b, bool on)
{
    const QString base = QStringLiteral(
        "QToolButton { border: 1px solid #ccc; border-radius: 4px; background: %1; font-weight: bold; "
        "min-width: 48px; min-height: 48px; }");
    b->setStyleSheet(base.arg(on ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff")));
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
    btnThree_->setText(tr("三点"));
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
    btnThree_->setChecked(m == ThreePoint);
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
        method_ = ThreePoint;
        syncButtons();
        emit methodChanged(ThreePoint);
    } else if (!btnCenter_->isChecked()) {
        updating_ = true;
        btnThree_->setChecked(true);
        updating_ = false;
    }
}

void SketchCircleModeDialog::syncButtons()
{
    styleSelButton(btnCenter_, method_ == CenterRadius);
    styleSelButton(btnThree_, method_ == ThreePoint);
}
