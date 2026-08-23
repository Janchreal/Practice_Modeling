// SARibbon 功能区：替换顶部 dock 工具栏（文件/视图/建模 + 草图上下文）
#include "widget.h"
#include "ui_widget.h"
#include "SARibbon.h"

#include <QAbstractButton>
#include <QAction>
#include <QCheckBox>
#include <QIcon>
#include <QList>
#include <QPair>
#include <QToolButton>

namespace {

QAction* actionFromButton(QObject* parent, QAbstractButton* btn, const QString& text = QString())
{
    if (!btn)
        return nullptr;
    auto* act = new QAction(btn->icon(), text.isEmpty() ? btn->text() : text, parent);
    act->setToolTip(btn->toolTip().isEmpty() ? act->text() : btn->toolTip());
    act->setStatusTip(act->toolTip());

    if (btn->isCheckable()) {
        act->setCheckable(true);
        act->setChecked(btn->isChecked());
        QObject::connect(act, &QAction::toggled, btn, [btn](bool on) {
            if (btn->isChecked() != on)
                btn->setChecked(on);
        });
        QObject::connect(btn, &QAbstractButton::toggled, act, [act](bool on) {
            if (act->isChecked() != on)
                act->setChecked(on);
        });
    } else {
        QObject::connect(act, &QAction::triggered, btn, &QAbstractButton::click);
    }
    return act;
}

/** 小按钮仅图标，避免中文挤在 Compact 三行里重叠；文案放 tooltip */
QAction* iconActionFromButton(QObject* parent, QAbstractButton* btn, const QString& tip)
{
    QAction* act = actionFromButton(parent, btn, QString());
    if (!act)
        return nullptr;
    act->setText(QString());
    act->setIconText(QString());
    act->setToolTip(tip);
    act->setStatusTip(tip);
    return act;
}

void addLarge(SARibbonPanel* panel, QAction* act)
{
    if (panel && act)
        panel->addLargeAction(act);
}

void addSmall(SARibbonPanel* panel, QAction* act)
{
    if (panel && act)
        panel->addSmallAction(act);
}

void fillSnapPanel(SARibbonPanel* panel,
                   QObject* parent,
                   QAbstractButton* enableBtn,
                   QAbstractButton* endBtn,
                   QAbstractButton* midBtn,
                   QAbstractButton* intersectBtn,
                   QAbstractButton* centerBtn,
                   QAbstractButton* quadrantBtn,
                   QAbstractButton* nearestBtn = nullptr,
                   QAbstractButton* clearBtn = nullptr)
{
    if (!panel)
        return;
    addLarge(panel, actionFromButton(parent, enableBtn, QObject::tr("启用捕捉")));
    addSmall(panel, iconActionFromButton(parent, endBtn, QObject::tr("端点")));
    addSmall(panel, iconActionFromButton(parent, midBtn, QObject::tr("中点")));
    addSmall(panel, iconActionFromButton(parent, intersectBtn, QObject::tr("交点")));
    addSmall(panel, iconActionFromButton(parent, centerBtn, QObject::tr("圆心")));
    addSmall(panel, iconActionFromButton(parent, quadrantBtn, QObject::tr("象限点")));
    if (nearestBtn)
        addSmall(panel, iconActionFromButton(parent, nearestBtn, QObject::tr("最近点")));
    if (clearBtn)
        addSmall(panel, iconActionFromButton(parent, clearBtn, QObject::tr("清除捕捉")));
}

void addMenuFromButtons(SARibbonPanel* panel,
                        QObject* parent,
                        const QString& title,
                        const QIcon& icon,
                        const QList<QPair<QAbstractButton*, QString>>& items,
                        bool large = true)
{
    if (!panel || items.isEmpty())
        return;
    auto* menu = new SARibbonMenu(title, qobject_cast<QWidget*>(parent));
    for (const auto& it : items) {
        if (QAction* a = actionFromButton(parent, it.first, it.second))
            menu->addAction(a);
    }
    auto* act = new QAction(icon, title, parent);
    act->setMenu(menu);
    if (large)
        panel->addLargeAction(act, QToolButton::InstantPopup);
    else
        panel->addSmallAction(act, QToolButton::InstantPopup);
}

} // namespace

void Widget::setupAppRibbon()
{
    if (!ui)
        return;

    // 顶部 Ribbon（不改继承，挂到 menuWidget；系统原生标题栏）
    auto* ribbon = new SARibbonBar(this);
    ribbon->setObjectName(QStringLiteral("appRibbonBar"));
    // 原生边框推荐 Compact；关闭换行避免小按钮中文竖排重叠
    ribbon->setRibbonStyle(SARibbonBar::RibbonStyleCompactThreeRow);
    ribbon->setEnableWordWrap(false);
    ribbon->setTitleVisible(false);
    setMenuWidget(ribbon);
    appRibbonBar_ = ribbon;

    // 主题挂在 Ribbon 自身，避免覆盖主窗口；并冲掉可能残留的全局 ToolButton 约束
    SA::applyRibbonTheme(ribbon, ribbon, SARibbonTheme::RibbonThemeOffice2013);
    const QString protect = QStringLiteral(
        "\nSARibbonToolButton, SARibbonApplicationButton {"
        "  min-width: 0px; min-height: 0px; padding: 0px; border-radius: 0px;"
        "}\n");
    ribbon->setStyleSheet(ribbon->styleSheet() + protect);

    // Application 按钮 =「文件」菜单（不再另建「文件」页签，避免双「文件」）
    if (auto* appBtn = qobject_cast<SARibbonApplicationButton*>(ribbon->applicationButton())) {
        appBtn->setText(tr("文件"));
        auto* fileMenu = new SARibbonMenu(this);
        if (auto* a = actionFromButton(this, ui->pushButton_47, tr("新建")))
            fileMenu->addAction(a);
        if (auto* a = actionFromButton(this, ui->pushButton_48, tr("打开")))
            fileMenu->addAction(a);
        if (auto* a = actionFromButton(this, ui->pushButton_46, tr("保存")))
            fileMenu->addAction(a);
        appBtn->setMenu(fileMenu);
        appBtn->setPopupMode(QToolButton::InstantPopup);
    }

    // 快捷访问栏：撤销/重做
    if (SARibbonQuickAccessBar* qab = ribbon->quickAccessBar()) {
        if (auto* a = actionFromButton(this, ui->undoButton, tr("撤销")))
            qab->addAction(a);
        if (auto* a = actionFromButton(this, ui->redoButton, tr("重做")))
            qab->addAction(a);
    }

    // ---------- 视图 ----------
    SARibbonCategory* catView = ribbon->addCategoryPage(tr("视图"));
    ribbonCatView_ = catView;
    {
        SARibbonPanel* panel = catView->addPanel(tr("定向视图"));
        addSmall(panel, actionFromButton(this, ui->pushButton, tr("正视图")));
        addSmall(panel, actionFromButton(this, ui->pushButton_2, tr("左视图")));
        addSmall(panel, actionFromButton(this, ui->pushButton_3, tr("右视图")));
        addSmall(panel, actionFromButton(this, ui->pushButton_29, tr("俯视图")));
        addSmall(panel, actionFromButton(this, ui->pushButton_17, tr("仰视图")));
        addSmall(panel, actionFromButton(this, ui->pushButton_27, tr("后视图")));
        addLarge(panel, actionFromButton(this, ui->pushButton_28, tr("正三轴测")));
    }
    {
        SARibbonPanel* panel = catView->addPanel(tr("窗口"));
        addLarge(panel, actionFromButton(this, ui->WindowpushButton, tr("新窗口")));
    }
    {
        SARibbonPanel* panel = catView->addPanel(tr("显示"));
        if (ui->checkBox) {
            auto* act = new QAction(tr("显示三重轴"), this);
            act->setCheckable(true);
            act->setChecked(ui->checkBox->isChecked());
            connect(act, &QAction::toggled, ui->checkBox, &QCheckBox::setChecked);
            connect(ui->checkBox, &QCheckBox::toggled, act, &QAction::setChecked);
            addLarge(panel, act);
        }
        if (ui->checkBox_2) {
            auto* act = new QAction(tr("三重轴右下角"), this);
            act->setCheckable(true);
            act->setChecked(ui->checkBox_2->isChecked());
            connect(act, &QAction::toggled, ui->checkBox_2, &QCheckBox::setChecked);
            connect(ui->checkBox_2, &QCheckBox::toggled, act, &QAction::setChecked);
            addSmall(panel, act);
        }
    }
    {
        SARibbonPanel* panel = catView->addPanel(tr("点捕捉"));
        fillSnapPanel(panel, this,
                      ui->Use_Capture,
                      ui->Capture_Endpoint,
                      ui->Capture_Midpoint,
                      ui->Capture_Insertsectionpoint,
                      ui->Capture_Arccenterpoint,
                      ui->Capture_Quadrantpoint,
                      ui->Capture_Closed,
                      ui->Capture_Clear);
    }

    // ---------- 建模 ----------
    SARibbonCategory* catModel = ribbon->addCategoryPage(tr("建模"));
    ribbonCatModel_ = catModel;
    {
        SARibbonPanel* panel = catModel->addPanel(tr("基本特征"));
        addLarge(panel, actionFromButton(this, ui->cuboid, tr("块")));
        addLarge(panel, actionFromButton(this, ui->sphere, tr("球体")));
        addLarge(panel, actionFromButton(this, ui->cylinder, tr("圆柱")));
        addLarge(panel, actionFromButton(this, ui->cone, tr("圆锥")));
        addLarge(panel, actionFromButton(this, ui->pushButton_72, tr("阵列")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("基准"));
        addLarge(panel, actionFromButton(this, ui->pushButton_4, tr("基准轴")));
        addLarge(panel, actionFromButton(this, ui->workAxisButton, tr("工作坐标系")));
        addLarge(panel, actionFromButton(this, ui->datum_plane_Button, tr("基准平面")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("细节特征"));
        addLarge(panel, actionFromButton(this, ui->extrude, tr("拉伸")));
        addLarge(panel, actionFromButton(this, ui->revolve, tr("旋转")));
        addLarge(panel, actionFromButton(this, ui->fillet, tr("圆角")));
        addLarge(panel, actionFromButton(this, ui->chamfer, tr("倒角")));
        addLarge(panel, actionFromButton(this, ui->createSketchButton, tr("草图")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("几何运算"));
        addLarge(panel, actionFromButton(this, ui->boolOperationButton, tr("布尔")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("工具"));
        addLarge(panel, actionFromButton(this, ui->expressionBtn, tr("表达式")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("历史"));
        addLarge(panel, actionFromButton(this, ui->undoButton, tr("撤销")));
        addLarge(panel, actionFromButton(this, ui->redoButton, tr("重做")));
    }
    {
        SARibbonPanel* panel = catModel->addPanel(tr("捕捉"));
        fillSnapPanel(panel, this,
                      ui->pushButton_71,
                      ui->pushButton_139,
                      ui->pushButton_143,
                      ui->pushButton_147,
                      ui->pushButton_141,
                      ui->pushButton_135);
    }

    // ---------- 草图（上下文：进入草图环境时显示） ----------
    sketchRibbonContext_ = ribbon->addContextCategory(tr("草图"), QColor(0x2B, 0x79, 0xB0));
    sketchRibbonCategory_ = sketchRibbonContext_->addCategoryPage(tr("草图"));
    {
        SARibbonPanel* panel = sketchRibbonCategory_->addPanel(tr("基本元素"));
        // 统一 Large，避免悬停底框一大一小、文字不在同一水平线
        addLarge(panel, actionFromButton(this, ui->pushButton_6, tr("完成草图")));
        addLarge(panel, actionFromButton(this, ui->pushButton_5, tr("任务草图")));
    }
    {
        SARibbonPanel* panel = sketchRibbonCategory_->addPanel(tr("操作"));
        addLarge(panel, actionFromButton(this, ui->pushButton_44, tr("修剪")));
        addLarge(panel, actionFromButton(this, ui->pushButton_45, tr("延伸")));
    }
    {
        SARibbonPanel* panel = sketchRibbonCategory_->addPanel(tr("曲线"));
        addLarge(panel, actionFromButton(this, ui->pushButton_7, tr("轮廓")));
        addLarge(panel, actionFromButton(this, ui->pushButton_41, tr("矩形")));
        addLarge(panel, actionFromButton(this, ui->pushButton_40, tr("直线")));
        addLarge(panel, actionFromButton(this, ui->pushButton_42, tr("圆弧")));
        addLarge(panel, actionFromButton(this, ui->pushButton_11, tr("圆")));
        addLarge(panel, actionFromButton(this, ui->pushButton_12, tr("点")));
        addMenuFromButtons(panel, this, tr("更多曲线"), QIcon(), {
            { ui->pushButton_8, tr("艺术样条") },
            { ui->pushButton_9, tr("多边形") },
            { ui->pushButton_10, tr("椭圆") },
            { ui->pushButton_13, tr("二次曲线") },
            { ui->pushButton_14, tr("偏置曲线") },
            { ui->pushButton_15, tr("阵列曲线") },
            { ui->pushButton_16, tr("镜像曲线") },
            { ui->pushButton_18, tr("交点") },
            { ui->pushButton_19, tr("相交曲线") },
            { ui->pushButton_20, tr("投影曲线") },
            { ui->pushButton_21, tr("派生曲线") },
            { ui->pushButton_22, tr("拟合曲线") },
        });
        addMenuFromButtons(panel, this, tr("编辑曲线"), QIcon(), {
            { ui->pushButton_23, tr("圆角") },
            { ui->pushButton_24, tr("倒斜角") },
            { ui->pushButton_25, tr("制作拐角") },
            { ui->pushButton_26, tr("修建配方曲线") },
            { ui->pushButton_31, tr("移动曲线") },
            { ui->pushButton_32, tr("偏置移动曲线") },
            { ui->pushButton_33, tr("缩放曲线") },
            { ui->pushButton_34, tr("调整曲线尺寸") },
            { ui->pushButton_35, tr("调整过倒斜角曲线尺寸") },
            { ui->pushButton_36, tr("删除曲线") },
        }, false);
    }
    {
        SARibbonPanel* panel = sketchRibbonCategory_->addPanel(tr("约束"));
        addLarge(panel, actionFromButton(this, ui->pushButton_37, tr("快速尺寸")));
        addLarge(panel, actionFromButton(this, ui->pushButton_38, tr("几何约束")));
        addLarge(panel, actionFromButton(this, ui->pushButton_39, tr("设置对称")));
        addLarge(panel, actionFromButton(this, ui->pushButton_43, tr("显示草图约束")));
    }
    {
        SARibbonPanel* panel = sketchRibbonCategory_->addPanel(tr("捕捉"));
        fillSnapPanel(panel, this,
                      ui->pushButton_70,
                      ui->pushButton_49,
                      ui->pushButton_50,
                      ui->pushButton_54,
                      ui->pushButton_55,
                      ui->pushButton_56);
    }

    ribbon->hideContextCategory(sketchRibbonContext_);
    ribbon->setCurrentIndex(1);  // 默认「建模」

    if (ui->dockWidget_3) {
        ui->dockWidget_3->setMinimumSize(0, 0);
        ui->dockWidget_3->hide();
    }
}

void Widget::applySketchRibbonMode(bool on)
{
    if (!appRibbonBar_)
        return;

    // 始终用 Ribbon，不再叠旧工具栏 dock
    if (menuWidget() != appRibbonBar_)
        setMenuWidget(appRibbonBar_);
    appRibbonBar_->show();
    if (ui->dockWidget_3)
        ui->dockWidget_3->hide();

    if (on) {
        if (ribbonCatModel_)
            appRibbonBar_->hideCategory(ribbonCatModel_);
        if (sketchRibbonContext_) {
            appRibbonBar_->showContextCategory(sketchRibbonContext_);
            if (sketchRibbonCategory_)
                appRibbonBar_->raiseCategory(sketchRibbonCategory_);
        }
    } else {
        if (sketchRibbonContext_)
            appRibbonBar_->hideContextCategory(sketchRibbonContext_);
        if (ribbonCatModel_) {
            appRibbonBar_->showCategory(ribbonCatModel_);
            appRibbonBar_->raiseCategory(ribbonCatModel_);
        }
    }
}
