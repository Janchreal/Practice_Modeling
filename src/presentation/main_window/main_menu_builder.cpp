#include "main_menu_builder.h"

#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <Qt>

namespace {

QAction* act(QMenu* menu, const QString& text, const QKeySequence& shortcut = {}, bool enabled = true)
{
    QAction* a = menu->addAction(text);
    if (!shortcut.isEmpty())
        a->setShortcut(shortcut);
    a->setEnabled(enabled);
    return a;
}

void addPh(QMenu* sub)
{
    QAction* p = sub->addAction(QStringLiteral("（待补充）"));
    p->setEnabled(false);
}

void fillTaskMenu(QMenu* m)
{
    act(m, QStringLiteral("新建草图(&N)..."), QKeySequence(QStringLiteral("Ctrl+N")));
    act(m, QStringLiteral("打开草图(&O)..."), QKeySequence(QStringLiteral("Ctrl+O")));
    act(m, QStringLiteral("草图属性(&I)"));
    act(m, QStringLiteral("草图设置(&K)..."));
    m->addSeparator();
    act(m, QStringLiteral("保存部件(&S)"), QKeySequence(QStringLiteral("Ctrl+S")));
    act(m, QStringLiteral("仅保存工作部件(&W)"));
    m->addSeparator();
    act(m, QStringLiteral("打印(&P)..."));
    m->addSeparator();
    QMenu* util = m->addMenu(QStringLiteral("实用工具(&U)"));
    addPh(util);
    QMenu* exec = m->addMenu(QStringLiteral("执行(&I)"));
    addPh(exec);
    m->addSeparator();
    act(m, QStringLiteral("完成草图(&K)"), QKeySequence(QStringLiteral("Ctrl+Q")));
    act(m, QStringLiteral("退出草图(&E)"));
    m->addSeparator();
    act(m, QStringLiteral("退出 NX"));
}

void fillEditMenu(QMenu* m)
{
    act(m, QStringLiteral("撤消"), QKeySequence(QStringLiteral("Ctrl+Z")));
    act(m, QStringLiteral("重做(&R)"), QKeySequence(QStringLiteral("Ctrl+Y")), false);
    m->addSeparator();
    act(m, QStringLiteral("剪切(&T)"), QKeySequence(QStringLiteral("Ctrl+X")), false);
    act(m, QStringLiteral("复制(&C)"), QKeySequence(QStringLiteral("Ctrl+C")), false);
    act(m, QStringLiteral("复制显示(&S)"));
    act(m, QStringLiteral("粘贴(&P)"), QKeySequence(QStringLiteral("Ctrl+V")), false);
    act(m, QStringLiteral("删除(&D)..."), QKeySequence(QStringLiteral("Ctrl+D")));
    m->addSeparator();
    QMenu* sel = m->addMenu(QStringLiteral("选择(&L)"));
    addPh(sel);
    act(m, QStringLiteral("对象显示(&J)..."), QKeySequence(QStringLiteral("Ctrl+J")));
    QMenu* sh = m->addMenu(QStringLiteral("显示和隐藏(&H)"));
    addPh(sh);
    act(m, QStringLiteral("变换(&M)..."));
    act(m, QStringLiteral("移动对象(&O)..."), QKeySequence(QStringLiteral("Ctrl+T")));
    act(m, QStringLiteral("属性(&I)..."));
    act(m, QStringLiteral("设置(&S)..."));
    m->addSeparator();
    QMenu* curve = m->addMenu(QStringLiteral("曲线(&V)"));
    addPh(curve);
    act(m, QStringLiteral("编辑定义截面(&E)..."));
    act(m, QStringLiteral("草图参数(&A)..."));
}

void fillViewMenu(QMenu* m)
{
    QMenu* op = m->addMenu(QStringLiteral("操作(&O)"));
    addPh(op);
    QMenu* nav = m->addMenu(QStringLiteral("导航(&N)"));
    addPh(nav);
    QMenu* sec = m->addMenu(QStringLiteral("截面(&S)"));
    addPh(sec);
    m->addSeparator();
    act(m, QStringLiteral("信息窗口(&I)"), QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    act(m, QStringLiteral("当前对话框(&C)"), QKeySequence(QStringLiteral("F3")), false);
    QAction* res = act(m, QStringLiteral("显示资源条(&R)"));
    res->setCheckable(true);
    res->setChecked(true);
    act(m, QStringLiteral("最小化功能区(&Z)"));
    act(m, QStringLiteral("全屏显示(&F)"), QKeySequence(QStringLiteral("Alt+Return")));
    act(m, QStringLiteral("触控模式(&T)"));
    act(m, QStringLiteral("欢迎页面(&W)"));
    m->addSeparator();
    act(m, QStringLiteral("定向视图到草图(&K)"), QKeySequence(QStringLiteral("Shift+F8")));
    act(m, QStringLiteral("定向视图到模型(&W)"));
}

void fillInsertMenu(QMenu* m)
{
    QMenu* datum = m->addMenu(QStringLiteral("基准/点(&D)"));
    addPh(datum);
    QMenu* curve = m->addMenu(QStringLiteral("曲线(&C)"));
    addPh(curve);
    QMenu* fromC = m->addMenu(QStringLiteral("来自曲线集的曲线(&F)"));
    addPh(fromC);
    QMenu* recipe = m->addMenu(QStringLiteral("配方曲线(&U)"));
    addPh(recipe);
    m->addSeparator();
    QMenu* dim = m->addMenu(QStringLiteral("尺寸(&M)"));
    addPh(dim);
    act(m, QStringLiteral("几何约束(&I)..."), QKeySequence(Qt::Key_C));
    act(m, QStringLiteral("设为对称(&M)..."));
}

void fillFormatMenu(QMenu* m)
{
    act(m, QStringLiteral("图层设置(&S)..."), QKeySequence(QStringLiteral("Ctrl+L")));
    QMenu* grp = m->addMenu(QStringLiteral("组(&G)"));
    addPh(grp);
}

void fillToolsMenu(QMenu* m)
{
    act(m, QStringLiteral("表达式(&X)..."), QKeySequence(QStringLiteral("Ctrl+E")));
    act(m, QStringLiteral("显示尺寸标注为 PMI(&S)..."));
    QMenu* upd = m->addMenu(QStringLiteral("更新(&U)"));
    addPh(upd);
    m->addSeparator();
    act(m, QStringLiteral("单位管理器(&U)..."));
    act(m, QStringLiteral("单位转换器(&O)..."));
    m->addSeparator();
    QMenu* pn = m->addMenu(QStringLiteral("部件导航器(&P)"));
    addPh(pn);
    QMenu* an = m->addMenu(QStringLiteral("装配导航器(&A)"));
    addPh(an);
    QMenu* lib = m->addMenu(QStringLiteral("重用库(&Y)"));
    addPh(lib);
    m->addSeparator();
    QMenu* jr = m->addMenu(QStringLiteral("操作记录(&J)"));
    addPh(jr);
    QMenu* macro = m->addMenu(QStringLiteral("宏(&R)"));
    addPh(macro);
    QMenu* movie = m->addMenu(QStringLiteral("电影(&E)"));
    addPh(movie);
    act(m, QStringLiteral("定制(&Z)..."), QKeySequence(QStringLiteral("Ctrl+1")));
    QMenu* rep = m->addMenu(QStringLiteral("重复命令(&R)"));
    addPh(rep);
    m->addSeparator();
    QMenu* cst = m->addMenu(QStringLiteral("约束(&I)"));
    addPh(cst);
    act(m, QStringLiteral("重新附着草图(&H)..."));
    act(m, QStringLiteral("定位尺寸(&S)"));
}

void fillInformationMenu(QMenu* m)
{
    act(m, QStringLiteral("对象(&O)..."), QKeySequence(QStringLiteral("Ctrl+I")));
    act(m, QStringLiteral("点(&P)..."));
    act(m, QStringLiteral("样条(&S)..."));
    QMenu* expr = m->addMenu(QStringLiteral("表达式(&X)"));
    addPh(expr);
    m->addSeparator();
    act(m, QStringLiteral("单位信息(&N)"));
    act(m, QStringLiteral("部件中单位信息(&P)"));
}

void fillAnalysisMenu(QMenu* m)
{
    act(m, QStringLiteral("测量(&S)..."));
    act(m, QStringLiteral("截面惯性(&N)..."));
    m->addSeparator();
    QMenu* curve = m->addMenu(QStringLiteral("曲线(&C)"));
    addPh(curve);
    QMenu* shape = m->addMenu(QStringLiteral("形状(&H)"));
    addPh(shape);
    m->addSeparator();
    act(m, QStringLiteral("用曲线计算面积(&A)..."));
}

void fillPreferencesMenu(QMenu* m)
{
    act(m, QStringLiteral("草图(&S)..."));
    act(m, QStringLiteral("制图(&D)..."));
    m->addSeparator();
    act(m, QStringLiteral("用户界面(&I)..."), QKeySequence(QStringLiteral("Ctrl+2")));
    act(m, QStringLiteral("可视化(&V)..."), QKeySequence(QStringLiteral("Ctrl+Shift+V")));
    act(m, QStringLiteral("选择(&E)..."), QKeySequence(QStringLiteral("Ctrl+Shift+T")));
    m->addSeparator();
    act(m, QStringLiteral("资源板(&P)..."));
    m->addSeparator();
    act(m, QStringLiteral("对象(&O)..."), QKeySequence(QStringLiteral("Ctrl+Shift+J")));
    act(m, QStringLiteral("调色板(&C)..."));
    act(m, QStringLiteral("背景(&A)..."));
    act(m, QStringLiteral("栅格(&G)..."));
    act(m, QStringLiteral("草图着重(&M)"));
    act(m, QStringLiteral("将草图平面外的对象设为不可选(&B)"), QKeySequence(), false);
}

void fillWindowMenu(QMenu* m)
{
    act(m, QStringLiteral("重置布局(&R)"));
    QMenu* layout = m->addMenu(QStringLiteral("窗口布局"));
    addPh(layout);
    m->addSeparator();
    QAction* w1 = act(m, QStringLiteral("1. _model1.prt"));
    w1->setCheckable(true);
    w1->setChecked(true);
    m->addSeparator();
    act(m, QStringLiteral("切换窗口(&W)..."));
}

void fillHelpMenu(QMenu* m)
{
    act(m, QStringLiteral("上下文帮助(&C)..."), QKeySequence(QStringLiteral("F1")));
    m->addSeparator();
    act(m, QStringLiteral("NX 帮助(&H)..."));
    act(m, QStringLiteral("发行说明(&R)..."));
    act(m, QStringLiteral("新增功能指南(&W)..."));
    m->addSeparator();
    act(m, QStringLiteral("命令查找器(&F)..."));
    m->addSeparator();
    act(m, QStringLiteral("Learning Advantage"));
    m->addSeparator();
    act(m, QStringLiteral("捕捉事件报告数据(&I)"));
    act(m, QStringLiteral("生成 IR/PR 的支持日志(&G)"));
    act(m, QStringLiteral("日志文件(&L)"));
    QMenu* online = m->addMenu(QStringLiteral("在线技术支持(&O)"));
    addPh(online);
    QMenu* forum = m->addMenu(QStringLiteral("PLM 社区论坛"));
    addPh(forum);
    m->addSeparator();
    act(m, QStringLiteral("关于 NX(&A)"));
}

QMenu* buildMainMenu(QWidget* owner)
{
    QMenu* bar = new QMenu(owner);
    fillTaskMenu(bar->addMenu(QStringLiteral("任务(&K)")));
    fillEditMenu(bar->addMenu(QStringLiteral("编辑(&E)")));
    fillViewMenu(bar->addMenu(QStringLiteral("视图(&V)")));
    fillInsertMenu(bar->addMenu(QStringLiteral("插入(&S)")));
    fillFormatMenu(bar->addMenu(QStringLiteral("格式(&R)")));
    fillToolsMenu(bar->addMenu(QStringLiteral("工具(&T)")));
    fillInformationMenu(bar->addMenu(QStringLiteral("信息(&I)")));
    fillAnalysisMenu(bar->addMenu(QStringLiteral("分析(&L)")));
    fillPreferencesMenu(bar->addMenu(QStringLiteral("首选项(&P)")));
    fillWindowMenu(bar->addMenu(QStringLiteral("窗口(&O)")));
    fillHelpMenu(bar->addMenu(QStringLiteral("帮助(&H)")));
    return bar;
}

} // namespace

void setupNxMainApplicationMenuButton(QToolButton* button)
{
    if (!button)
        return;

    QMenu* menu = buildMainMenu(button);
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setText(QStringLiteral("菜单(&M)"));
    button->setAutoRaise(false);
}
