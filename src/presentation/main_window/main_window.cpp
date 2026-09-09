// 本文件：构造/析构与窗口初始化；事件与上下文桥接已拆到各自子文件。
#include "main_window.h"
#include "viewport/mirror/mirror_view_state.h"
#include "ui_main_window.h"

#include "presentation/main_window/main_menu_builder.h"

#include <QCheckBox>
#include <QLayout>
#include <QKeySequence>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QStringList>
#include <QStatusBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <Qt>

#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>

/** 递归收紧底部工具 Dock 内所有布局边距与间距（仅作用于 dockWidgetContents_3 子树）。 */
static void tightenDockToolLayouts(QLayout* layout, int margin, int spacing)
{
    if (!layout)
        return;
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (!item)
            continue;
        if (QLayout* sub = item->layout()) {
            tightenDockToolLayouts(sub, margin, spacing);
        } else if (QWidget* w = item->widget()) {
            if (w->layout())
                tightenDockToolLayouts(w->layout(), margin, spacing);
        }
    }
}

static void tightenDockToolLayouts(QWidget* root, int margin, int spacing)
{
    if (!root || !root->layout())
        return;
    tightenDockToolLayouts(root->layout(), margin, spacing);
}

static void applyCompactDockToolChrome(QWidget* dockContents, QTabWidget* tabWidget)
{
    if (dockContents) {
        tightenDockToolLayouts(dockContents, 3, 4);
        dockContents->setStyleSheet(QStringLiteral(
            "QGroupBox { margin-top: 4px; padding-top: 1px; padding-bottom: 2px; padding-left: 4px; padding-right: 4px; }"
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 4px; padding: 0px 2px; }"
            "QPushButton { padding: 1px 5px; }"
            "QToolButton { padding: 1px 4px; }"
            "QComboBox { padding: 1px 4px; min-height: 18px; }"
            "QCheckBox, QLabel { margin: 0px; padding: 0px; }"
            "QTabWidget::pane { margin: 0px; padding: 0px 2px 2px 2px; }"
            "QDoubleSpinBox { padding: 1px 2px; min-height: 18px; }"));
    }
    if (tabWidget)
        tabWidget->setDocumentMode(true);
}

// 初始化静态成员
int Widget::shapeIDCounter = 0;

Widget::Widget(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Widget)
    , historyList(modelDocument_.histories())
    , currentSelectedIndex(-1)
    , currentSelectionMode(None)
    , selectedTargetIndex(-1)
    , boolDialog(nullptr)
    , extrusionDialog(nullptr)
    , previewActor(nullptr)
    , currentPickedModelIndex(-1)
    , cuboidDialog(nullptr)
    , cylinderDialog(nullptr)
    , coneDialog(nullptr)
    , sphereDialog(nullptr)
    , tempOriginX(0.0)
    , tempOriginY(0.0)
    , tempOriginZ(0.0)
    , hasTempOrigin(false)
{
    // 初始化世界坐标
    lastWorldPoint[0] = 0.0;
    lastWorldPoint[1] = 0.0;
    lastWorldPoint[2] = 0.0;
    lastWorldPoint[3] = 1.0;

    ui->setupUi(this);

    setupNxMainApplicationMenuButton(ui->toolButton_nxMainMenu_modeling);
    setupNxMainApplicationMenuButton(ui->toolButton_nxMainMenu_sketch);

    applyCompactDockToolChrome(ui->dockWidgetContents_3, ui->tabWidget);

    setupToolbarModeStack();
    // 捕捉按钮需先设为 checkable，再挂到 Ribbon
    setupSnapPointUI();
    setupAppRibbon();  // SARibbon 顶部功能区（文件/视图/建模）
    wireSketchTabStackedPages();
    setupSketchCreationToggleButtons();

    // 旧 dock 工具栏仅草图环境使用；平时隐藏以把空间留给渲染区
    if (ui->dockWidget_3) {
        ui->dockWidget_3->setMinimumHeight(0);
        ui->dockWidget_3->setMinimumWidth(0);
        ui->dockWidget_3->setMinimumSize(0, 0);
        ui->dockWidget_3->hide();
    }
    if (ui->tabWidget) {
        ui->tabWidget->setMinimumSize(0, 0);
        ui->tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }

    // 已弃用 historyList（QListWidget），仅使用左侧特征树

    // 连接特征树的信号
    connect(ui->treeWidget, &QTreeWidget::itemClicked,
            this, &Widget::onFeatureTreeItemClicked);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &Widget::onFeatureTreeItemDoubleClicked);

    // 初始化特征树
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->setHeaderLabels(QStringList() << "名称" << "可见");
    ui->treeWidget->setColumnWidth(0, 150);
    ui->treeWidget->setColumnWidth(1, 50);

    // 特征树右键菜单（删除等）
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &Widget::onFeatureTreeContextMenuRequested);

    // 创建"模型历史"父节点
    QTreeWidgetItem* modelHistoryRoot = new QTreeWidgetItem(ui->treeWidget);
    modelHistoryRoot->setText(0, "模型历史");
    modelHistoryRoot->setExpanded(true);  // 默认展开
    ui->treeWidget->addTopLevelItem(modelHistoryRoot);

    // 连接undo状态变化信号
    connect(this, &Widget::undoStateChanged, this, &Widget::updateUndoRedoButtons);

    //设置VTK
    setupVTK();

    // 三重轴显示控制
    if (ui->checkBox) {
        ui->checkBox->setChecked(true);
        connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool checked) {
            if (centerAxesRenderer) {
                centerAxesRenderer->SetDraw(checked ? 1 : 0);
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
        });
    }
    if (ui->checkBox_2) {
        connect(ui->checkBox_2, &QCheckBox::toggled, this, [this](bool checked) {
            updateCenterTriadViewport(checked);
        });
        updateCenterTriadViewport(ui->checkBox_2->isChecked());
    }

    // 视图按钮/树项：统一切换并同步
    setupViewUiConnections();

    // 初始化拉伸选择列表
    extrusionSelectedIndices = QList<int>();
    extrusionSelectedFaces.clear();
    hasHoveredFace = false;
    // 初始化交互样式（setupVTK 内已创建并安装 m_interactorStyle）
    if (!m_interactorStyle) {
        m_interactorStyle = vtkSmartPointer<MouseInteractorStyle>::New();
        m_interactorStyle->SetPreEventHook([this]() { activateMainRenderContext(); });
    }
    m_interactorStyle->SetWidget(this);
    // 初始更新按钮状态
    updateUndoRedoButtons();
    // 添加快捷键
    QShortcut* undoShortcut = new QShortcut(QKeySequence::Undo, this);
    QShortcut* redoShortcut = new QShortcut(QKeySequence::Redo, this);
    QShortcut* quickTrimShortcut = new QShortcut(QKeySequence(Qt::Key_T), this);
    QShortcut* quickExtendShortcut = new QShortcut(QKeySequence(Qt::Key_E), this);

    connect(undoShortcut, &QShortcut::activated, this, &Widget::on_undoButton_clicked);
    connect(redoShortcut, &QShortcut::activated, this, &Widget::on_redoButton_clicked);
    connect(quickTrimShortcut, &QShortcut::activated, this, &Widget::startSketchQuickTrim);
    connect(quickExtendShortcut, &QShortcut::activated, this, &Widget::startSketchQuickExtend);
    QShortcut* finishSketchShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Q")), this);
    connect(finishSketchShortcut, &QShortcut::activated, this, [this]() {
        if (inSketchEnvironment_)
            on_pushButton_6_clicked();
    });

    // 设置主窗口的最小尺寸，允许窗口缩小到很小
    setMinimumSize(100, 100);

    // 设置 central widget 的最小尺寸为 0，允许它缩小
    ui->centralwidget->setMinimumSize(0, 0);

    // 设置 dock widget 的最小尺寸，允许它们缩小到150
    ui->dockWidget->setMinimumWidth(150);
    // 设置历史记录窗口的初始宽度为250
    // 使用resizeDocks来设置初始宽度
    resizeDocks({ui->dockWidget}, {250}, Qt::Horizontal);
    ui->dockWidget_3->setMinimumHeight(0);
    ui->dockWidget_3->setMinimumWidth(0);

    // 文件页中的“新建”按钮，复用窗口新建逻辑
    connect(ui->pushButton_47, &QPushButton::clicked, this, &Widget::createNewWindow);
    // 文件页中的“保存/打开”按钮
    connect(ui->pushButton_46, &QPushButton::clicked, this, &Widget::handleSaveFile);
    connect(ui->pushButton_48, &QPushButton::clicked, this, &Widget::handleOpenFile);
    // 草图操作按钮：快速修剪 / 快速延伸
    connect(ui->pushButton_44, &QPushButton::clicked, this, &Widget::startSketchQuickTrim);
    connect(ui->pushButton_45, &QPushButton::clicked, this, &Widget::startSketchQuickExtend);
    connect(ui->pushButton_72, &QPushButton::clicked, this, &Widget::on_patternFeature_clicked);
    if (ui->groupBox_22 && ui->gridLayout_20) {
        openRecentMenuButton_ = new QToolButton(ui->groupBox_22);
        openRecentMenuButton_->setText(QStringLiteral("▼"));
        openRecentMenuButton_->setToolTip(tr("最近文件"));
        openRecentMenuButton_->setAutoRaise(true);
        openRecentMenuButton_->setFixedWidth(18);
        openRecentMenu_ = new QMenu(openRecentMenuButton_);
        openRecentMenuButton_->setMenu(openRecentMenu_);
        openRecentMenuButton_->setPopupMode(QToolButton::InstantPopup);
        ui->gridLayout_20->addWidget(openRecentMenuButton_, 0, 3, 1, 1);
    }

    // 文件快捷键（UG/NX 常见习惯）
    QShortcut* newShortcut = new QShortcut(QKeySequence::New, this);
    QShortcut* openShortcut = new QShortcut(QKeySequence::Open, this);
    QShortcut* saveShortcut = new QShortcut(QKeySequence::Save, this);
    QShortcut* saveAsShortcut = new QShortcut(QKeySequence::SaveAs, this);
    connect(newShortcut, &QShortcut::activated, this, &Widget::handleNewFile);
    connect(openShortcut, &QShortcut::activated, this, &Widget::handleOpenFile);
    connect(saveShortcut, &QShortcut::activated, this, &Widget::handleSaveFile);
    connect(saveAsShortcut, &QShortcut::activated, this, &Widget::handleSaveFileAs);

    // 将主窗口状态栏消息同步到当前活动子窗口，避免多窗口操作时提示不可见。
    g_activeStatusWindowMap[this] = this;
    if (statusBar()) {
        connect(statusBar(), &QStatusBar::messageChanged, this, [this](const QString& msg) {
            QMainWindow* host = g_activeStatusWindowMap.value(this, this);
            if (!host || host == this) return;
            if (!host->statusBar()) return;
            if (msg.isEmpty()) host->statusBar()->clearMessage();
            else host->statusBar()->showMessage(msg);
        });
    }

    updateDocumentWindowTitle();
    loadRecentFilesFromSettings();
    setupRecentFilesUi();
    refreshRecentFilesUi();
    rebuildOpenRecentMenu();

    autoSaveTimer_ = new QTimer(this);
    autoSaveTimer_->setInterval(120000); // 2 分钟自动恢复快照
    connect(autoSaveTimer_, &QTimer::timeout, this, &Widget::saveAutoRecoverySnapshot);
    autoSaveTimer_->start();

    initDefaultReferenceCsys();
    markDocumentModified(false);

    QTimer::singleShot(0, this, [this]() { tryRecoverFromAutoSnapshotOnStartup(); });
}


Widget::~Widget()
{
    g_mirrorWindowMap.remove(this);
    g_mirrorRenderContextMap.remove(this);
    g_mainVtkWidgetMap.remove(this);
    g_mainRendererMap.remove(this);
    g_mainPickerMap.remove(this);
    g_activeStatusWindowMap.remove(this);
    delete ui;
}

// 更新历史列表的选中状态
