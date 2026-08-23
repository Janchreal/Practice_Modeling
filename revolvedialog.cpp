#include "revolvedialog.h"
#include "ui_revolvedialog.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString vectorModeTitle(int modeIndex)
{
    switch (modeIndex) {
    case 0: return QObject::tr("自动判断");
    case 1: return QObject::tr("两点");
    case 3: return QObject::tr("曲线/轴矢量");
    case 4: return QObject::tr("曲线上矢量");
    case 5: return QObject::tr("面/平面法向量");
    case 6: return QObject::tr("面上点的矢量");
    case 7: return QObject::tr("XC轴");
    case 8: return QObject::tr("YC轴");
    case 9: return QObject::tr("ZC轴");
    case 10: return QObject::tr("-XC轴");
    case 11: return QObject::tr("-YC轴");
    case 12: return QObject::tr("-ZC轴");
    case 13: return QObject::tr("视图方向");
    default: return QObject::tr("矢量");
    }
}

} // namespace

revolvedialog::revolvedialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::revolvedialog)
{
    ui->setupUi(this);

    ui->sectionTypeCombo->clear();
    ui->sectionTypeCombo->addItem(QStringLiteral("曲线"));
    ui->sectionTypeCombo->addItem(QStringLiteral("面片"));
    ui->sectionTypeCombo->setCurrentIndex(0);

    ui->startCombo->clear();
    ui->startCombo->addItem(QStringLiteral("值"));
    ui->startCombo->addItem(QStringLiteral("类型"));
    ui->startCombo->setCurrentIndex(0);

    ui->endCombo->clear();
    ui->endCombo->addItem(QStringLiteral("值"));
    ui->endCombo->addItem(QStringLiteral("类型"));
    ui->endCombo->setCurrentIndex(0);

    ui->startDistanceLineEdit->setText(QStringLiteral("0"));
    // 初始起止同角：球与箭头重合；拖终止箭头拉开扫掠角（避免默认 360° 卡在上限只能反方向拖）
    ui->endDistanceLineEdit->setText(QStringLiteral("0"));

    initializeUIState();
    updateSelectionDisplay();
    setupBooleanSection();

    connect(ui->geometrySelectorButton, &QPushButton::clicked,
            this, &revolvedialog::on_geometrySelectorButton_clicked);
    connect(ui->clearSelectionButton, &QPushButton::clicked,
            this, &revolvedialog::on_clearSelectionButton_clicked);
    connect(ui->sectionTypeCombo, &QComboBox::currentTextChanged,
            this, &revolvedialog::on_sectionTypeCombo_currentTextChanged);
    connect(ui->startCombo, &QComboBox::currentTextChanged,
            this, &revolvedialog::on_startCombo_currentTextChanged);
    connect(ui->endCombo, &QComboBox::currentTextChanged,
            this, &revolvedialog::on_endCombo_currentTextChanged);
    // previewButton 已由 setupUi 内 QMetaObject::connectSlotsByName 自动连接，勿重复 connect
    connect(ui->selectButton, &QPushButton::clicked,
            this, &revolvedialog::on_selectButton_clicked);
    connect(ui->pushButton, &QPushButton::clicked,
            this, &revolvedialog::on_pushButton_clicked);

    connect(ui->startDistanceLineEdit, &QLineEdit::textEdited, this, &revolvedialog::parametersChanged);
    connect(ui->endDistanceLineEdit, &QLineEdit::textEdited, this, &revolvedialog::parametersChanged);

    if (ui->buttonBox) {
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    if (ui->toolButton) {
        QMenu *modeMenu = new QMenu(this);
        auto addAct = [&](const QString& text, int modeIndex) {
            QAction* act = modeMenu->addAction(text);
            connect(act, &QAction::triggered, this, [this, modeIndex]() {
                setVectorModeIndex(modeIndex);
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
        setVectorModeLabel(0);
    }

    if (ui->pushButton_2) {
        connect(ui->pushButton_2, &QPushButton::clicked, this, [this]() {
            axisReversed_ = !axisReversed_;
            emit parametersChanged();
        });
    }

    if (ui->originSnapToolButton) {
        QMenu* originMenu = new QMenu(this);
        auto addAct = [&](const QString& text, int kind) {
            QAction* act = originMenu->addAction(text);
            connect(act, &QAction::triggered, this, [this, kind, text]() {
                originSnapKind_ = kind;
                ui->originSnapToolButton->setText(text);
            });
        };
        addAct(tr("最近点"), 0);
        addAct(tr("端点"), 1);
        addAct(tr("中点"), 2);
        addAct(tr("交点"), 3);
        addAct(tr("圆心"), 4);
        addAct(tr("象限点"), 5);

        ui->originSnapToolButton->setMenu(originMenu);
        ui->originSnapToolButton->setPopupMode(QToolButton::InstantPopup);
        ui->originSnapToolButton->setToolTip("旋转中心捕捉类型");
    }

    on_sectionTypeCombo_currentTextChanged(ui->sectionTypeCombo->currentText());
}

revolvedialog::~revolvedialog()
{
    delete ui;
}

void revolvedialog::setupBooleanSection()
{
    auto* rootLayout = qobject_cast<QVBoxLayout*>(ui->verticalLayout);
    if (!rootLayout) return;

    booleanGroup_ = new QGroupBox(tr("布尔"), this);
    // 插在「结果」groupBox_4 之前
    int insertAt = rootLayout->count();
    for (int i = 0; i < rootLayout->count(); ++i) {
        if (rootLayout->itemAt(i)->widget() == ui->groupBox_4) {
            insertAt = i;
            break;
        }
    }
    rootLayout->insertWidget(insertAt, booleanGroup_);

    booleanGroup_->setCheckable(true);
    booleanGroup_->setChecked(false);

    booleanContent_ = new QWidget(booleanGroup_);
    auto* contentLay = new QVBoxLayout(booleanContent_);
    contentLay->setContentsMargins(4, 4, 4, 4);
    contentLay->setSpacing(6);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("布尔"), booleanContent_));
    booleanCombo_ = new QComboBox(booleanContent_);
    booleanCombo_->addItem(tr("无"), -1);
    booleanCombo_->addItem(tr("合并"), 0);
    booleanCombo_->addItem(tr("减去"), 2);
    booleanCombo_->addItem(tr("求交"), 1);
    row1->addWidget(booleanCombo_, 1);
    contentLay->addLayout(row1);

    booleanSelectBodyBtn_ = new QPushButton(tr("选择体"), booleanContent_);
    booleanTargetLabel_ = new QLabel(tr("未选择目标体"), booleanContent_);
    booleanTargetLabel_->setStyleSheet(QStringLiteral("color: #666;"));
    contentLay->addWidget(booleanSelectBodyBtn_);
    contentLay->addWidget(booleanTargetLabel_);

    auto* groupLay = new QVBoxLayout(booleanGroup_);
    groupLay->setContentsMargins(6, 6, 6, 6);
    groupLay->addWidget(booleanContent_);

    booleanContent_->setVisible(false);
    refreshBooleanUi();

    connect(booleanGroup_, &QGroupBox::toggled, this, [this](bool on) {
        if (booleanContent_) booleanContent_->setVisible(on);
    });
    connect(booleanCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshBooleanUi();
        emit booleanModeChanged(booleanMode());
        emit parametersChanged();
    });
    connect(booleanSelectBodyBtn_, &QPushButton::clicked, this, &revolvedialog::startBooleanTargetSelection);
}

void revolvedialog::refreshBooleanUi()
{
    const bool needBody = booleanMode() >= 0;
    if (booleanSelectBodyBtn_) booleanSelectBodyBtn_->setVisible(needBody);
    if (booleanTargetLabel_) booleanTargetLabel_->setVisible(needBody);
}

void revolvedialog::setVectorModeLabel(int modeIndex)
{
    if (ui->toolButton) {
        ui->toolButton->setText(vectorModeTitle(modeIndex));
        ui->toolButton->setToolTip(tr("矢量模式: %1").arg(vectorModeTitle(modeIndex)));
    }
}

void revolvedialog::setVectorModeIndex(int modeIndex)
{
    vectorModeIndex_ = modeIndex;
    setVectorModeLabel(modeIndex);
    if (modeIndex != 0) {
        axisVectorSelected_ = true;
    }
    emit parametersChanged();
}

int revolvedialog::booleanMode() const
{
    if (!booleanCombo_) return -1;
    return booleanCombo_->currentData().toInt();
}

void revolvedialog::setBooleanTargetIndex(int index, const QString& name)
{
    booleanTargetIndex_ = index;
    if (booleanTargetLabel_) {
        if (index < 0) {
            booleanTargetLabel_->setText(tr("未选择目标体"));
        } else {
            booleanTargetLabel_->setText(name.isEmpty()
                ? tr("目标体 #%1").arg(index + 1)
                : tr("目标体: %1").arg(name));
        }
    }
    emit parametersChanged();
}

double revolvedialog::getStartAngle() const
{
    bool ok = false;
    double value = ui->startDistanceLineEdit->text().toDouble(&ok);
    if (!ok) value = 0.0;
    // 连续拖拽可能越过 ±360，放宽到 ±720；真正幅角由 |end-start|≤360 约束
    if (value < -720.0) value = -720.0;
    if (value > 720.0) value = 720.0;
    return value;
}

double revolvedialog::getEndAngle() const
{
    bool ok = false;
    double value = ui->endDistanceLineEdit->text().toDouble(&ok);
    if (!ok) value = 0.0;
    if (value < -720.0) value = -720.0;
    if (value > 720.0) value = 720.0;
    return value;
}

void revolvedialog::setStartAngle(double deg)
{
    if (!ui->startDistanceLineEdit) return;
    QSignalBlocker b(ui->startDistanceLineEdit);
    ui->startDistanceLineEdit->setText(QString::number(deg, 'f', 1));
}

void revolvedialog::setEndAngle(double deg)
{
    if (!ui->endDistanceLineEdit) return;
    QSignalBlocker b(ui->endDistanceLineEdit);
    ui->endDistanceLineEdit->setText(QString::number(deg, 'f', 1));
}

QString revolvedialog::getSectionType() const
{
    return ui->sectionTypeCombo->currentText();
}

QString revolvedialog::getSelectionMode() const
{
    QString sectionType = getSectionType();
    if (sectionType == QStringLiteral("曲线")) {
        return QStringLiteral("edge");
    } else if (sectionType == QStringLiteral("面片")) {
        return QStringLiteral("face");
    }
    return QStringLiteral("model");
}

void revolvedialog::setSelectedGeometryCount(int count)
{
    selectedCount = count;
    updateSelectionDisplay();
}

void revolvedialog::initializeUIState()
{
    ui->startDistanceLabel->setVisible(true);
    ui->startDistanceLineEdit->setVisible(true);
    ui->endDistanceLabel->setVisible(true);
    ui->endDistanceLineEdit->setVisible(true);
}

void revolvedialog::updateSelectionDisplay()
{
    ui->selectedCountLabel->setText(
        QStringLiteral("已选择: %1").arg(selectedCount));
}

void revolvedialog::on_geometrySelectorButton_clicked()
{
    emit startSelection();
}

void revolvedialog::on_clearSelectionButton_clicked()
{
    selectedCount = 0;
    updateSelectionDisplay();
    emit clearSelection();
}

void revolvedialog::on_sectionTypeCombo_currentTextChanged(const QString &text)
{
    if (text == QStringLiteral("曲线")) {
        emit selectionModeChanged(QStringLiteral("edge"));
    } else if (text == QStringLiteral("面片")) {
        emit selectionModeChanged(QStringLiteral("face"));
    }
}

void revolvedialog::on_startCombo_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void revolvedialog::on_endCombo_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void revolvedialog::applyResultPreviewUiLock(bool locked)
{
    // 只禁可交互控件；不禁用含预览按钮的 QGroupBox 容器
    const QList<QWidget*> interactive = findChildren<QWidget*>();
    for (QWidget* w : interactive) {
        if (!w || w == ui->previewButton) continue;
        if (qobject_cast<QAbstractButton*>(w)
            || qobject_cast<QComboBox*>(w)
            || qobject_cast<QLineEdit*>(w)
            || qobject_cast<QAbstractSpinBox*>(w)
            || qobject_cast<QDialogButtonBox*>(w)) {
            w->setEnabled(!locked);
        }
    }
    if (booleanGroup_) {
        booleanGroup_->setEnabled(!locked);
    }
    if (ui->previewButton) {
        ui->previewButton->setEnabled(true);
        ui->previewButton->setText(locked ? tr("取消预览结果") : tr("预览"));
    }
}

void revolvedialog::setResultPreviewActive(bool active)
{
    if (resultPreviewActive_ == active) return;
    resultPreviewActive_ = active;
    applyResultPreviewUiLock(active);
}

void revolvedialog::on_previewButton_clicked()
{
    if (resultPreviewActive_) {
        setResultPreviewActive(false);
        emit cancelPreviewRequested();
        return;
    }
    emit previewRequested();
}

void revolvedialog::on_selectButton_clicked()
{
    axisVectorSelected_ = true;
    emit vectorSelectionRequested();
}

void revolvedialog::on_pushButton_clicked()
{
    centerPointSelected_ = true;
    if (originSnapKind_ >= 0) {
        emit pointSelectionRequestedWithSnap(originSnapKind_);
    } else {
        emit pointSelectionRequested();
    }
}
