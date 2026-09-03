#include "extrusion_dialog.h"
#include "ui_extrusion_dialog.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
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

ExtrusionDialog::ExtrusionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExtrusionDialog),
    selectedCount(0)
{
    ui->setupUi(this);

    ui->sectionTypeCombo->clear();
    ui->sectionTypeCombo->addItem("曲线");
    ui->sectionTypeCombo->addItem("面片");
    ui->sectionTypeCombo->setCurrentIndex(0);

    ui->startCombo->clear();
    ui->startCombo->addItem("值");
    ui->startCombo->addItem("类型");
    ui->startCombo->setCurrentIndex(0);

    ui->endCombo->clear();
    ui->endCombo->addItem("值");
    ui->endCombo->addItem("类型");
    ui->endCombo->setCurrentIndex(0);

    ui->startDistanceLineEdit->setText("0");
    ui->endDistanceLineEdit->setText("0"); // 初始起止同位置：球与可操作箭头重合，拖拽箭头再拉开距离

    initializeUIState();
    updateSelectionDisplay();
    setupBooleanSection();

    connect(ui->okButton, &QPushButton::clicked, this, &ExtrusionDialog::on_okButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ExtrusionDialog::on_cancelButton_clicked);
    connect(ui->geometrySelectorButton, &QPushButton::clicked, this, &ExtrusionDialog::on_geometrySelectorButton_clicked);
    connect(ui->clearSelectionButton, &QPushButton::clicked, this, &ExtrusionDialog::on_clearSelectionButton_clicked);
    connect(ui->sectionTypeCombo, &QComboBox::currentTextChanged, this, &ExtrusionDialog::on_sectionTypeCombo_currentTextChanged);
    connect(ui->startCombo, &QComboBox::currentTextChanged, this, &ExtrusionDialog::on_startCombo_currentTextChanged);
    connect(ui->endCombo, &QComboBox::currentTextChanged, this, &ExtrusionDialog::on_endCombo_currentTextChanged);
    // previewButton 已由 setupUi 内 QMetaObject::connectSlotsByName 自动连接，勿重复 connect
    connect(ui->selectButton, &QPushButton::clicked, this, &ExtrusionDialog::on_selectButton_clicked);

    connect(ui->startDistanceLineEdit, &QLineEdit::textEdited, this, &ExtrusionDialog::parametersChanged);
    connect(ui->endDistanceLineEdit, &QLineEdit::textEdited, this, &ExtrusionDialog::parametersChanged);

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

    if (ui->pushButton) {
        ui->pushButton->setCheckable(false);
        connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
            axisReversed_ = !axisReversed_;
            emit parametersChanged();
        });
    }

    on_sectionTypeCombo_currentTextChanged(ui->sectionTypeCombo->currentText());
}

ExtrusionDialog::~ExtrusionDialog()
{
    delete ui;
}

void ExtrusionDialog::setupBooleanSection()
{
    auto* rootLayout = qobject_cast<QVBoxLayout*>(ui->verticalLayout);
    if (!rootLayout) {
        if (auto* grid = qobject_cast<QGridLayout*>(layout())) {
            booleanGroup_ = new QGroupBox(tr("布尔"), this);
            grid->addWidget(booleanGroup_, grid->rowCount(), 0);
        } else {
            return;
        }
    } else {
        booleanGroup_ = new QGroupBox(tr("布尔"), this);
        // 插在「结果」之后（verticalLayout 末尾）；「设置」在外层 grid 另一行
        rootLayout->addWidget(booleanGroup_);
    }

    booleanGroup_->setCheckable(true);
    booleanGroup_->setChecked(false);
    booleanGroup_->setFlat(false);

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
    connect(booleanSelectBodyBtn_, &QPushButton::clicked, this, &ExtrusionDialog::startBooleanTargetSelection);
}

void ExtrusionDialog::refreshBooleanUi()
{
    const bool needBody = booleanMode() >= 0;
    if (booleanSelectBodyBtn_) booleanSelectBodyBtn_->setVisible(needBody);
    if (booleanTargetLabel_) booleanTargetLabel_->setVisible(needBody);
}

void ExtrusionDialog::setVectorModeLabel(int modeIndex)
{
    if (ui->toolButton) {
        ui->toolButton->setText(vectorModeTitle(modeIndex));
        ui->toolButton->setToolTip(tr("矢量模式: %1").arg(vectorModeTitle(modeIndex)));
    }
}

void ExtrusionDialog::setVectorModeIndex(int modeIndex)
{
    vectorModeIndex_ = modeIndex;
    setVectorModeLabel(modeIndex);
    if (modeIndex != 0) {
        axisVectorSelected_ = true;
    }
    emit parametersChanged();
}

int ExtrusionDialog::booleanMode() const
{
    if (!booleanCombo_) return -1;
    return booleanCombo_->currentData().toInt();
}

void ExtrusionDialog::setBooleanTargetIndex(int index, const QString& name)
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

QString ExtrusionDialog::getSelectionMode() const
{
    QString sectionType = getSectionType();
    if (sectionType == "曲线") {
        return "edge";
    } else if (sectionType == "面片") {
        return "face";
    }
    return "model";
}

void ExtrusionDialog::initializeUIState()
{
    ui->startDistanceLabel->setVisible(true);
    ui->startDistanceLineEdit->setVisible(true);
    ui->endDistanceLabel->setVisible(true);
    ui->endDistanceLineEdit->setVisible(true);
}

double ExtrusionDialog::getStartDistance() const
{
    bool ok = false;
    const double value = ui->startDistanceLineEdit->text().toDouble(&ok);
    return ok ? value : 0.0;
}

double ExtrusionDialog::getEndDistance() const
{
    bool ok = false;
    const double value = ui->endDistanceLineEdit->text().toDouble(&ok);
    return ok ? value : 10.0;
}

void ExtrusionDialog::setStartDistance(double v)
{
    if (!ui->startDistanceLineEdit) return;
    QSignalBlocker b(ui->startDistanceLineEdit);
    ui->startDistanceLineEdit->setText(QString::number(v, 'f', 3));
}

void ExtrusionDialog::setEndDistance(double v)
{
    if (!ui->endDistanceLineEdit) return;
    QSignalBlocker b(ui->endDistanceLineEdit);
    ui->endDistanceLineEdit->setText(QString::number(v, 'f', 3));
}

QString ExtrusionDialog::getSectionType() const
{
    return ui->sectionTypeCombo->currentText();
}

QString ExtrusionDialog::getBodyType() const
{
    return ui->comboBox->currentText();
}

bool ExtrusionDialog::isSheetBodyType() const
{
    return getBodyType() == QStringLiteral("片体");
}

void ExtrusionDialog::setSelectedGeometryCount(int count)
{
    selectedCount = count;
    updateSelectionDisplay();
}

void ExtrusionDialog::updateSelectionDisplay()
{
    ui->selectedCountLabel->setText(QString("已选择: %1").arg(selectedCount));
}

void ExtrusionDialog::on_okButton_clicked()
{
    accept();
}

void ExtrusionDialog::on_cancelButton_clicked()
{
    reject();
}

void ExtrusionDialog::on_geometrySelectorButton_clicked()
{
    emit startSelection();
}

void ExtrusionDialog::on_clearSelectionButton_clicked()
{
    selectedCount = 0;
    updateSelectionDisplay();
    emit clearSelection();
}

void ExtrusionDialog::on_sectionTypeCombo_currentTextChanged(const QString &text)
{
    if (text == "曲线") {
        emit selectionModeChanged("edge");
    } else if (text == "面片") {
        emit selectionModeChanged("face");
    }
}

void ExtrusionDialog::on_startCombo_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void ExtrusionDialog::on_endCombo_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void ExtrusionDialog::applyResultPreviewUiLock(bool locked)
{
    // 只禁可交互控件；不禁用 QGroupBox 容器，否则其子级预览按钮也会被连带禁用
    const QList<QWidget*> interactive = findChildren<QWidget*>();
    for (QWidget* w : interactive) {
        if (!w || w == ui->previewButton) continue;
        if (qobject_cast<QAbstractButton*>(w)
            || qobject_cast<QComboBox*>(w)
            || qobject_cast<QLineEdit*>(w)
            || qobject_cast<QAbstractSpinBox*>(w)) {
            w->setEnabled(!locked);
        }
    }
    if (booleanGroup_) {
        // 布尔分组本身可勾选，需单独灰化；内容控件已在上面循环处理
        booleanGroup_->setEnabled(!locked);
    }
    if (ui->previewButton) {
        ui->previewButton->setEnabled(true);
        ui->previewButton->setText(locked ? tr("取消预览结果") : tr("预览"));
    }
}

void ExtrusionDialog::setResultPreviewActive(bool active)
{
    if (resultPreviewActive_ == active) return;
    resultPreviewActive_ = active;
    applyResultPreviewUiLock(active);
}

void ExtrusionDialog::on_previewButton_clicked()
{
    if (resultPreviewActive_) {
        setResultPreviewActive(false);
        emit cancelPreviewRequested();
        return;
    }
    emit previewRequested();
}

void ExtrusionDialog::on_selectButton_clicked()
{
    axisVectorSelected_ = true;
    emit vectorSelectionRequested();
}
