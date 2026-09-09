#include "vector_dialog.h"
#include "ui_vector_dialog.h"

#include <QComboBox>
#include <QAction>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QToolButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <algorithm>

static void clearLayout(QLayout* layout)
{
    if (!layout) return;
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

vectordialog::vectordialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::vectordialog)
{
    ui->setupUi(this);

    reverse_ = false;
    rebuildVectorDefineArea(ui->comboBox->currentIndex());

    // 下拉模式切换：重建定义区域 + 通知 Widget
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                rebuildVectorDefineArea(idx);
                emit vectorModeChanged(idx);
            });

    // 反转按钮：切换状态并通知 Widget
    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        reverse_ = !reverse_;
        emit vectorReverseToggled(reverse_);
    });

    // 让外部在完成 connect 后也能拿到初始模式
    QTimer::singleShot(0, this, [this]() {
        emit vectorModeChanged(ui->comboBox->currentIndex());
    });
}

vectordialog::~vectordialog()
{
    delete ui;
}

void vectordialog::setModeIndex(int modeIndex)
{
    if (!ui || !ui->comboBox) return;
    ui->comboBox->setCurrentIndex(modeIndex);
}

int vectordialog::modeIndex() const
{
    if (!ui || !ui->comboBox) return 0;
    return ui->comboBox->currentIndex();
}

void vectordialog::setCurveTotalLength(double totalLength)
{
    curveTotalLength_ = std::max(0.0, totalLength);
    if (curveTotalLenLabel_) {
        curveTotalLenLabel_->setText(tr("总弧长：%1 mm").arg(curveTotalLength_, 0, 'f', 5));
    }
    if (curvePosValueSpin_ && curvePosModeCombo_) {
        const int mode = curvePosModeCombo_->currentIndex(); // 0% / 1mm
        if (mode == 1) {
            curvePosValueSpin_->setRange(0.0, curveTotalLength_ > 0.0 ? curveTotalLength_ : 1e9);
        }
    }
}

void vectordialog::setCurvePicked(bool picked)
{
    curvePicked_ = picked;
    // 位置“模式”允许在未选曲线时提前切换（仅数值输入与总弧长依赖曲线，需在选中后启用）
    if (curvePosModeCombo_) curvePosModeCombo_->setEnabled(true);
    if (curvePosValueSpin_) curvePosValueSpin_->setEnabled(picked);
    if (curveTotalLenLabel_) curveTotalLenLabel_->setEnabled(picked);
    if (!picked && curveTotalLenLabel_) {
        curveTotalLenLabel_->setText(tr("总弧长：未选择"));
    }
}

void vectordialog::setVectorDirDisplay(double x, double y, double z)
{
    if (!curveVectorDirLabel_) return;
    curveVectorDirLabel_->setText(tr("方向：(%1, %2, %3)")
                                      .arg(x, 0, 'f', 6)
                                      .arg(y, 0, 'f', 6)
                                      .arg(z, 0, 'f', 6));
}

void vectordialog::setReverseState(bool reversed)
{
    reverse_ = reversed;
}

void vectordialog::setTwoPointPointState(bool hasStart, bool hasEnd,
                                         bool selectingStart, bool selectingEnd)
{
    if (twoPointStartBtn_) {
        twoPointStartBtn_->setText(
            (hasStart || selectingStart) ? tr("重新指定起点")
                                         : tr("指定出发点"));
        twoPointStartBtn_->setEnabled(true);
    }
    if (twoPointEndBtn_) {
        twoPointEndBtn_->setText(
            (hasEnd || selectingEnd) ? tr("重新指定终点")
                                     : tr("指定目标点"));
        twoPointEndBtn_->setEnabled(hasStart || hasEnd || selectingEnd);
    }
}

int vectordialog::twoPointStartSnapKindForPick() const
{
    return twoPointStartSnapChosen_ ? twoPointStartSnapKind_ : -1;
}

int vectordialog::twoPointEndSnapKindForPick() const
{
    return twoPointEndSnapChosen_ ? twoPointEndSnapKind_ : -1;
}

void vectordialog::rebuildVectorDefineArea(int modeIndex)
{
    // vector_dialog.ui 可能是“layout 直接挂载”（vectorDefineLayout），也可能是“容器 widget + layout”（vectorDefineContainer）。
    // 当前以 vectorDefineLayout 为准，避免 vectorDefineContainer 被 UI 修改后编译失败。
    QLayout* baseLayout = ui->vectorDefineLayout;
    if (!baseLayout) return;

    QWidget* parentForWidgets = baseLayout->parentWidget();
    if (!parentForWidgets) parentForWidgets = this;

    clearLayout(baseLayout);

    auto makeDisabledChooseButton = [&](const QString& text) {
        auto* btn = new QPushButton(text, parentForWidgets);
        btn->setEnabled(false);
        return btn;
    };

    auto makeSectionTitle = [&](const QString& title) {
        auto* label = new QLabel(title, parentForWidgets);
        label->setWordWrap(true);
        return label;
    };

    // 0 自动判断的矢量
    // 1 两点
    // 3 曲线/轴矢量
    // 4 曲线上矢量
    // 5 面/平面法向量
    // 6 面上点的矢量
    // 7~12 轴
    // 13 视图方向

    if (modeIndex == 0 || modeIndex == 2) {
        auto* title = makeSectionTitle(tr("要定义的矢量对象"));
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(title);

        static_cast<QVBoxLayout*>(baseLayout)->addWidget(makeDisabledChooseButton(tr("选择对象（悬停可自动识别）")));
        return;
    }

    if (modeIndex == 1) {
        // 两点
        twoPointStartBtn_ = nullptr;
        twoPointEndBtn_ = nullptr;
        twoPointStartSnapToolButton_ = nullptr;
        twoPointEndSnapToolButton_ = nullptr;
        twoPointStartSnapChosen_ = false;
        twoPointEndSnapChosen_ = false;

        static_cast<QVBoxLayout*>(baseLayout)->addWidget(makeSectionTitle(tr("通过点")));

        auto kindToLabel = [](int kind) -> QString {
            switch (kind) {
            case 1: return QObject::tr("端点");
            case 2: return QObject::tr("中点");
            case 5: return QObject::tr("象限点");
            case 6: return QObject::tr("圆弧中点");
            case 3: return QObject::tr("交点");
            default: return QObject::tr("端点");
            }
        };

        auto makeTwoPointSnapToolButton = [&](bool isStart) -> QToolButton* {
            auto* btn = new QToolButton(parentForWidgets);
            if (isStart) {
                btn->setText(twoPointStartSnapChosen_ ? kindToLabel(twoPointStartSnapKind_) : tr("任意点"));
            } else {
                btn->setText(twoPointEndSnapChosen_ ? kindToLabel(twoPointEndSnapKind_) : tr("任意点"));
            }
            btn->setPopupMode(QToolButton::InstantPopup);

            auto* menu = new QMenu(btn);
            // 顺序 端点/中点/象限点/圆弧中点/交点
            auto addAction = [&](const QString& label, int snapKind) {
                QAction* act = menu->addAction(label);
                QObject::connect(act, &QAction::triggered, btn, [this, isStart, snapKind, label, btn]() {
                    if (isStart) {
                        twoPointStartSnapKind_ = snapKind;
                        // snapKind=-1 表示“任意点”：不视为已显式选择捕捉类型
                        twoPointStartSnapChosen_ = (snapKind != -1);
                    } else {
                        twoPointEndSnapKind_ = snapKind;
                        twoPointEndSnapChosen_ = (snapKind != -1);
                    }
                    btn->setText(label);
                });
            };
            // 顶部添加“任意点”，用于撤销/切换到任意点模式
            addAction(QObject::tr("任意点"), -1);
            addAction(QObject::tr("端点"), 1);
            addAction(QObject::tr("中点"), 2);
            addAction(QObject::tr("象限点"), 5);
            addAction(QObject::tr("圆弧中点"), 6);
            addAction(QObject::tr("交点"), 3);

            btn->setMenu(menu);
            return btn;
        };

            auto* startRow = new QWidget(parentForWidgets);
        auto* startRowLayout = new QHBoxLayout(startRow);
        startRowLayout->setContentsMargins(0, 0, 0, 0);

        twoPointStartBtn_ = new QPushButton(tr("指定出发点"), startRow);
        twoPointStartSnapToolButton_ = makeTwoPointSnapToolButton(true);

        startRowLayout->addWidget(twoPointStartBtn_);
        startRowLayout->addWidget(twoPointStartSnapToolButton_);

            auto* endRow = new QWidget(parentForWidgets);
        auto* endRowLayout = new QHBoxLayout(endRow);
        endRowLayout->setContentsMargins(0, 0, 0, 0);

        twoPointEndBtn_ = new QPushButton(tr("指定目标点"), endRow);
        twoPointEndSnapToolButton_ = makeTwoPointSnapToolButton(false);
        setTwoPointPointState(false, false);

        endRowLayout->addWidget(twoPointEndBtn_);
        endRowLayout->addWidget(twoPointEndSnapToolButton_);

        static_cast<QVBoxLayout*>(baseLayout)->addWidget(startRow);
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(endRow);

        connect(twoPointStartBtn_, &QPushButton::clicked, this, [this]() {
            if (twoPointEndBtn_) twoPointEndBtn_->setEnabled(true);
            const int startKind = twoPointStartSnapChosen_ ? twoPointStartSnapKind_ : -1;
            const int endKind = twoPointEndSnapChosen_ ? twoPointEndSnapKind_ : -1;
            emit twoPointStartRequestedWithSnap(startKind, endKind);
        });
        connect(twoPointEndBtn_, &QPushButton::clicked, this, [this]() {
            const int endKind = twoPointEndSnapChosen_ ? twoPointEndSnapKind_ : -1;
            emit twoPointEndRequestedWithSnap(endKind);
        });
        return;
    }

    if (modeIndex == 3 || modeIndex == 4) {
        // 曲线/轴矢量、曲线上矢量
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(makeSectionTitle(tr("曲线")));
        auto* pickBtn = new QPushButton(tr("选择对象"), parentForWidgets);
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(pickBtn);
        connect(pickBtn, &QPushButton::clicked, this, [this]() {
            emit curveSelectionRequested();
        });

        // 仅“曲线上矢量”显示“曲线上的位置”
        curvePosModeCombo_ = nullptr;
        curvePosValueSpin_ = nullptr;
        curveTotalLenLabel_ = nullptr;
        curveVectorDirLabel_ = nullptr;
        curvePicked_ = false;
        curveTotalLength_ = 0.0;

        if (modeIndex == 4) {
            auto* posGroup = new QGroupBox(tr("曲线上的位置"), parentForWidgets);
            posGroup->setFlat(true);
            auto* form = new QFormLayout(posGroup);
            form->setContentsMargins(6, 6, 6, 6);
            form->setHorizontalSpacing(8);
            form->setVerticalSpacing(6);

            curvePosModeCombo_ = new QComboBox(posGroup);
            curvePosModeCombo_->addItem(tr("弧长百分比"));
            curvePosModeCombo_->addItem(tr("弧长"));

            curvePosValueSpin_ = new QDoubleSpinBox(posGroup);
            curvePosValueSpin_->setDecimals(5);
            curvePosValueSpin_->setRange(0.0, 100.0);
            curvePosValueSpin_->setSuffix(tr(" %"));
            curvePosValueSpin_->setSingleStep(1.0);

            curveTotalLenLabel_ = new QLabel(tr("总弧长：未选择"), posGroup);
            curveVectorDirLabel_ = new QLabel(tr("方向：(0, 0, 1)"), posGroup);

            form->addRow(tr("位置"), curvePosModeCombo_);
            form->addRow(tr("弧长/弧长百分比"), curvePosValueSpin_);
            form->addRow(QString(), curveTotalLenLabel_);
            form->addRow(QString(), curveVectorDirLabel_);

            static_cast<QVBoxLayout*>(baseLayout)->addWidget(posGroup);

            // 初始：未选曲线前禁用，避免用户误以为已生效
            setCurvePicked(false);

            connect(curvePosModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int idx) {
                        if (!curvePosValueSpin_) return;
                        if (idx == 0) {
                            curvePosValueSpin_->setSuffix(tr(" %"));
                            curvePosValueSpin_->setRange(0.0, 100.0);
                            if (curvePosValueSpin_->value() > 100.0) curvePosValueSpin_->setValue(100.0);
                        } else {
                            curvePosValueSpin_->setSuffix(tr(" mm"));
                            curvePosValueSpin_->setRange(0.0, curveTotalLength_ > 0.0 ? curveTotalLength_ : 1e9);
                        }
                        emit curveVectorPositionModeChanged(idx);
                        emit curveVectorPositionValueChanged(curvePosValueSpin_->value());
                    });

            connect(curvePosValueSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this](double v) {
                        emit curveVectorPositionValueChanged(v);
                    });
        }
        return;
    }

    if (modeIndex == 5 || modeIndex == 6) {
        // 面法向相关：点击“选择对象”后进入悬停拾取
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(makeSectionTitle(tr("要定义的矢量对象")));
        auto* pickBtn = new QPushButton(modeIndex == 6 ? tr("选择对象（悬浮实时显示）")
                                                       : tr("选择对象"),
                                        parentForWidgets);
        static_cast<QVBoxLayout*>(baseLayout)->addWidget(pickBtn);
        connect(pickBtn, &QPushButton::clicked, this, [this]() {
            emit curveSelectionRequested();
        });
        return;
    }

    // 轴 / 视图方向 / 其它：不需要额外输入区
    auto* info = makeSectionTitle(tr("已直接确定矢量方向（必要时可点击反转）。"));
    static_cast<QVBoxLayout*>(baseLayout)->addWidget(info);
}
