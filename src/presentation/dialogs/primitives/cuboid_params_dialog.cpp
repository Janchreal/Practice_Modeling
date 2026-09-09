#include "cuboid_params_dialog.h"
#include "ui_cuboid_params_dialog.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QToolButton>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {

// 简易表达式求值：支持数字与 + - * / () ，满足“表达式项”测试变更
bool evalSimpleExpression(const QString& raw, double& out)
{
    QString s = raw.trimmed();
    if (s.endsWith(QStringLiteral("mm"), Qt::CaseInsensitive)) {
        s.chop(2);
        s = s.trimmed();
    }
    // 去掉空格
    s.remove(QLatin1Char(' '));
    if (s.isEmpty()) return false;

    int pos = 0;
    const int n = s.size();
    auto peek = [&]() -> QChar { return pos < n ? s.at(pos) : QChar(); };
    auto get = [&]() -> QChar { return pos < n ? s.at(pos++) : QChar(); };

    std::function<bool(double&)> parseExpr;
    std::function<bool(double&)> parseTerm;
    std::function<bool(double&)> parseFactor;

    parseFactor = [&](double& v) -> bool {
        QChar c = peek();
        if (c == QLatin1Char('+')) { get(); return parseFactor(v); }
        if (c == QLatin1Char('-')) { get(); if (!parseFactor(v)) return false; v = -v; return true; }
        if (c == QLatin1Char('(')) {
            get();
            if (!parseExpr(v)) return false;
            if (peek() != QLatin1Char(')')) return false;
            get();
            return true;
        }
        int start = pos;
        while (pos < n && (s.at(pos).isDigit() || s.at(pos) == QLatin1Char('.') || s.at(pos) == QLatin1Char('e')
                           || s.at(pos) == QLatin1Char('E'))) {
            ++pos;
        }
        if (pos == start) return false;
        bool ok = false;
        v = s.mid(start, pos - start).toDouble(&ok);
        return ok;
    };

    parseTerm = [&](double& v) -> bool {
        if (!parseFactor(v)) return false;
        while (true) {
            QChar c = peek();
            if (c != QLatin1Char('*') && c != QLatin1Char('/')) break;
            get();
            double rhs = 0.0;
            if (!parseFactor(rhs)) return false;
            if (c == QLatin1Char('*')) v *= rhs;
            else {
                if (std::abs(rhs) < 1e-15) return false;
                v /= rhs;
            }
        }
        return true;
    };

    parseExpr = [&](double& v) -> bool {
        if (!parseTerm(v)) return false;
        while (true) {
            QChar c = peek();
            if (c != QLatin1Char('+') && c != QLatin1Char('-')) break;
            get();
            double rhs = 0.0;
            if (!parseTerm(rhs)) return false;
            if (c == QLatin1Char('+')) v += rhs;
            else v -= rhs;
        }
        return true;
    };

    double value = 0.0;
    if (!parseExpr(value)) return false;
    if (pos != n) return false;
    if (!std::isfinite(value)) return false;
    out = value;
    return true;
}

} // namespace

CuboidParamsDialog::CuboidParamsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CuboidParamsDialog)
    , originX(0.0)
    , originY(0.0)
    , originZ(0.0)
    , hasOrigin(false)
{
    ui->setupUi(this);
    setWindowTitle(tr("块"));
    ensureShowResultButton();
    setupDimensionExpressionFields();

    // 默认创建方式：原点和边长
    if (ui->comboBox_3) {
        ui->comboBox_3->setCurrentIndex(0);
    }

    // originSnap toolButton 菜单：选择用于原点捕捉的类型
    if (ui->originSnapToolButton) {
        QMenu* menu = new QMenu(this);
        auto addAct = [&](const QString& text, int kind) -> QAction* {
            QAction* act = menu->addAction(text);
            connect(act, &QAction::triggered, this, [this, kind, text]() {
                originSnapKind_ = kind;
                ui->originSnapToolButton->setText(text);
            });
            return act;
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

    // 反向按钮：切换高度轴反向
    if (ui->pushButton) {
        connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
            axisReversed_ = !axisReversed_;
        });
    }

    // toolButton：矢量模式选择（用于打开 vectordialog）
    if (ui->toolButton) {
        QMenu* modeMenu = new QMenu(this);
        auto addAct = [&](const QString& text, int modeIndex) {
            QAction* act = modeMenu->addAction(text);
            connect(act, &QAction::triggered, this, [this, modeIndex]() {
                emit requestVectorMode(modeIndex);
            });
        };

        addAct(tr("自动判断"), 0);
        addAct(tr("两点"), 1);
        addAct(tr("曲线/轴矢量"), 3);
        addAct(tr("曲线上矢量"), 4);
        addAct(tr("面/平面法向量"), 5);
        addAct(tr("面上点的矢量"), 6);
        addAct(tr("XC轴"), 7);
        addAct(tr("YC轴"), 8);
        addAct(tr("ZC轴"), 9);
        addAct(tr("-XC轴"), 10);
        addAct(tr("-YC轴"), 11);
        addAct(tr("-ZC轴"), 12);
        addAct(tr("视图方向"), 13);

        ui->toolButton->setMenu(modeMenu);
        ui->toolButton->setPopupMode(QToolButton::InstantPopup);
        ui->toolButton->setToolTip("矢量模式");
    }
}

CuboidParamsDialog::~CuboidParamsDialog()
{
    delete ui;
}

void CuboidParamsDialog::setupDimensionExpressionFields()
{
    auto replaceSpin = [this](QDoubleSpinBox* spin, QHBoxLayout* lay, QLineEdit*& edit, double* cache) {
        if (!spin || !lay) return;
        *cache = spin->value();

        edit = new QLineEdit(this);
        edit->setObjectName(spin->objectName() + QStringLiteral("_expr"));
        edit->setText(QString::number(*cache, 'g', 10));
        edit->setPlaceholderText(tr("表达式"));
        edit->setToolTip(tr("尺寸表达式项，支持数字与 + - * / ()"));
        edit->setMinimumWidth(80);

        const int idx = lay->indexOf(spin);
        lay->removeWidget(spin);
        spin->hide();
        spin->setParent(this);
        if (idx >= 0) lay->insertWidget(idx, edit);
        else lay->addWidget(edit);

        connect(edit, &QLineEdit::editingFinished, this, [this, edit, cache]() {
            if (commitExpressionField(edit, cache)) {
                emit dimensionsChanged();
            }
        });
    };

    replaceSpin(ui->lengthSpinBox, ui->horizontalLayout_4, lengthExprEdit_, &lengthValue_);
    replaceSpin(ui->widthSpinBox, ui->horizontalLayout_3, widthExprEdit_, &widthValue_);
    replaceSpin(ui->heightSpinBox, ui->horizontalLayout_2, heightExprEdit_, &heightValue_);
}

bool CuboidParamsDialog::commitExpressionField(QLineEdit* edit, double* cachedValue) const
{
    if (!edit || !cachedValue) return false;
    double v = 0.0;
    if (!evalSimpleExpression(edit->text(), v) || v < 0.1) {
        edit->setText(QString::number(*cachedValue, 'g', 10));
        return false;
    }
    *cachedValue = v;
    edit->setText(edit->text().trimmed());
    return true;
}

double CuboidParamsDialog::getLength() const
{
    if (lengthExprEdit_) {
        double v = lengthValue_;
        evalSimpleExpression(lengthExprEdit_->text(), v);
        return std::max(0.1, v);
    }
    return ui->lengthSpinBox->value();
}

double CuboidParamsDialog::getWidth() const
{
    if (widthExprEdit_) {
        double v = widthValue_;
        evalSimpleExpression(widthExprEdit_->text(), v);
        return std::max(0.1, v);
    }
    return ui->widthSpinBox->value();
}

double CuboidParamsDialog::getHeight() const
{
    if (heightExprEdit_) {
        double v = heightValue_;
        evalSimpleExpression(heightExprEdit_->text(), v);
        return std::max(0.1, v);
    }
    return ui->heightSpinBox->value();
}

QString CuboidParamsDialog::lengthExpression() const
{
    return lengthExprEdit_ ? lengthExprEdit_->text().trimmed() : QString::number(getLength());
}

QString CuboidParamsDialog::widthExpression() const
{
    return widthExprEdit_ ? widthExprEdit_->text().trimmed() : QString::number(getWidth());
}

QString CuboidParamsDialog::heightExpression() const
{
    return heightExprEdit_ ? heightExprEdit_->text().trimmed() : QString::number(getHeight());
}

void CuboidParamsDialog::setLength(double length)
{
    lengthValue_ = std::max(0.1, length);
    if (lengthExprEdit_) lengthExprEdit_->setText(QString::number(lengthValue_, 'g', 10));
    ui->lengthSpinBox->setValue(lengthValue_);
}

void CuboidParamsDialog::setWidth(double width)
{
    widthValue_ = std::max(0.1, width);
    if (widthExprEdit_) widthExprEdit_->setText(QString::number(widthValue_, 'g', 10));
    ui->widthSpinBox->setValue(widthValue_);
}

void CuboidParamsDialog::setHeight(double height)
{
    heightValue_ = std::max(0.1, height);
    if (heightExprEdit_) heightExprEdit_->setText(QString::number(heightValue_, 'g', 10));
    ui->heightSpinBox->setValue(heightValue_);
}

QString CuboidParamsDialog::creationModeText() const
{
    return ui->comboBox_3 ? ui->comboBox_3->currentText() : QStringLiteral("原点和边长");
}

void CuboidParamsDialog::on_okButton_clicked()
{
    accept();
}

void CuboidParamsDialog::on_cancelButton_clicked()
{
    reject();
}

bool CuboidParamsDialog::hasOriginPoint() const
{
    return hasOrigin;
}

void CuboidParamsDialog::getOriginPoint(double& x, double& y, double& z) const
{
    x = originX;
    y = originY;
    z = originZ;
}

void CuboidParamsDialog::setOriginPoint(double x, double y, double z)
{
    originX = x;
    originY = y;
    originZ = z;
    hasOrigin = true;

    ui->designated_point->setText(
        QString("指定点: (%1, %2, %3)").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2));
}

void CuboidParamsDialog::on_designated_point_clicked()
{
    if (originSnapKind_ >= 0) {
        emit requestPointSelectionWithSnap(originSnapKind_);
    } else {
        emit requestPointSelection();
    }
}

void CuboidParamsDialog::on_applyButton_clicked()
{
    emit applyRequested();
}

void CuboidParamsDialog::ensureShowResultButton()
{
    if (showResultButton_) return;
    showResultButton_ = new QPushButton(tr("显示结果"), this);
    showResultButton_->setObjectName(QStringLiteral("showResultButton"));
    if (auto* layout = ui->gridLayout_6) {
        layout->addWidget(showResultButton_, 1, 0, 1, 3);
    }
    connect(showResultButton_, &QPushButton::clicked, this, &CuboidParamsDialog::onShowResultButtonClicked);
}

void CuboidParamsDialog::applyResultPreviewUiLock(bool locked)
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
        showResultButton_->setText(locked ? tr("撤销结果") : tr("显示结果"));
    }
}

void CuboidParamsDialog::setResultPreviewActive(bool active)
{
    if (resultPreviewActive_ == active) return;
    resultPreviewActive_ = active;
    applyResultPreviewUiLock(active);
}

void CuboidParamsDialog::onShowResultButtonClicked()
{
    if (resultPreviewActive_) {
        setResultPreviewActive(false);
        emit cancelPreviewRequested();
        return;
    }
    emit previewRequested();
}
