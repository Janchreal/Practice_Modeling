// 本文件：构造/析构、窗口与鼠标事件、工作轴/基准平面、点选原点、历史形状访问等（VTK 左键路由见 main_window_vtk_click.cpp）。
// 头文件仅保留本翻译单元实际用到的类型（main_window.h 已含大量 OCC/VTK/Qt 公共依赖）。
#include "main_window.h"
#include "mirror_view_types.h"
#include "mirror_view_state.h"
#include "ui_main_window.h"
#include "primitive_geometry.h"
#include "sketch_geometry.h"

#include "main_menu_builder.h"
#include "cuboid_params_dialog.h"
#include "cylinder_dialog.h"
#include "cone_params_dialog.h"
#include "sphere_params_dialog.h"
#include "sketch_create_dialog.h"

#include <algorithm>
#include <cmath>

#include <QCheckBox>
#include <QDialog>
#include <QLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStringList>
#include <QStatusBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <Qt>
#include <QVTKOpenGLNativeWidget.h>

#include <Standard_Real.hxx>

#include <BRep_Tool.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

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

static QWidget* currentDialogParent(Widget* owner)
{
    if (!owner) return nullptr;
    QMainWindow* host = g_activeStatusWindowMap.value(owner, owner);
    if (host) return host;
    return owner;
}

static QString viewNameFromTriadRegion(double u, double v)
{
    // 简易 3x3 区域映射：补齐子窗口立方体点击切视图能力
    if (u > 0.33 && u < 0.67 && v > 0.33 && v < 0.67) return QStringLiteral("正三轴测视图");
    if (v >= 0.67 && u > 0.33 && u < 0.67) return QStringLiteral("俯视图");
    if (v <= 0.33 && u > 0.33 && u < 0.67) return QStringLiteral("仰视图");
    if (u <= 0.33 && v > 0.33 && v < 0.67) return QStringLiteral("左视图");
    if (u >= 0.67 && v > 0.33 && v < 0.67) return QStringLiteral("右视图");
    if (u <= 0.33 && v >= 0.67) return QStringLiteral("后视图");
    if (u >= 0.67 && v >= 0.67) return QStringLiteral("正视图");
    if (u <= 0.33 && v <= 0.33) return QStringLiteral("后视图");
    if (u >= 0.67 && v <= 0.33) return QStringLiteral("正视图");
    return QString();
}

static MirrorRenderContext* currentMirrorContextForWidget(const Widget* owner, QVTKOpenGLNativeWidget* currentWidget)
{
    if (!owner || !currentWidget || !g_mirrorRenderContextMap.contains(owner)) return nullptr;
    auto& map = g_mirrorRenderContextMap[owner];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().vtkWidget == currentWidget) {
            return &it.value();
        }
    }
    return nullptr;
}

MirrorRenderContext* Widget::mirrorContextForVtkWidget(QVTKOpenGLNativeWidget* w)
{
    return currentMirrorContextForWidget(this, w);
}

QWidget* Widget::dialogParentWidget() const
{
    QMainWindow* host = g_activeStatusWindowMap.value(this, const_cast<Widget*>(this));
    if (host) return host;
    return const_cast<Widget*>(this);
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

void Widget::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (ui && ui->checkBox_2) {
        updateCenterTriadViewport(ui->checkBox_2->isChecked());
    } else if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 重写鼠标点击事件
void Widget::mousePressEvent(QMouseEvent* event)
{
    QMainWindow::mousePressEvent(event);

    // 如果处于布尔运算选择模式，不处理常规鼠标点击
    if (isInSelectionMode()) {
        return;
    }

    // 只处理左键点击
    if (event->button() != Qt::LeftButton) {
        return;
    }

    // event->pos() 相对 QMainWindow；vtkWidget->geometry() 相对其父控件，不能直接 contains
    if (!vtkWidget) {
        return;
    }
    const QPoint inVtk = vtkWidget->mapFrom(this, event->pos());
    if (!vtkWidget->rect().contains(inVtk)) {
        currentSelectedIndex = -1;
        highlightModel(-1);
        updateHistoryListSelection();
    }
}

void Widget::on_workAxisButton_clicked()
{
    // 逻辑：若已捕捉到点，则直接在该点创建/移动工作坐标系；否则进入放置模式等待下一次点击
    if (hasSnapSelectedPoint_) {
        if (!renderer || !vtkWidget) return;
        const gp_Pnt& p = snapSelectedPoint_;
        if (!ensureWorkCsysActorsCreated()) return;
        const bool needCreate = (workCsysHistoryIndex_ < 0);

        workCsysTransform->Identity();
        workCsysTransform->Translate(p.X(), p.Y(), p.Z());
        hasWorkCsys = true;
        workCsysDragActive = false;
        setWorkCsysVisible(true);
        applyAxisDirectionHighlight(currentAxisDirection);
        currentSelectionMode = None;

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }

        if (needCreate) {
            addToHistory(WORK_CSYS, tr("工作坐标系"),
                         workCsysActor, QColor(255, 230, 80),
                         p.X(), p.Y(), p.Z(),
                         nullptr, TopoDS_Shape(), nullptr, nullptr);
            workCsysHistoryIndex_ = historyList.size() - 1;
        } else if (workCsysHistoryIndex_ >= 0 && workCsysHistoryIndex_ < historyList.size()
                   && historyList[workCsysHistoryIndex_].type == WORK_CSYS) {
            historyList[workCsysHistoryIndex_].param1 = p.X();
            historyList[workCsysHistoryIndex_].param2 = p.Y();
            historyList[workCsysHistoryIndex_].param3 = p.Z();
        }

        if (statusBar()) {
            statusBar()->showMessage(tr("已在捕捉点处创建/移动工作坐标系。"), 2000);
        }
        return;
    }

    // 未捕捉到点：进入“工作坐标系放置”模式（下一次点击将创建/移动工作坐标系）
    currentSelectionMode = WorkCsysPlacement;
    if (statusBar()) {
        statusBar()->showMessage(tr("工作坐标系：请在 3D 视图中点击一个位置来创建/移动。"), 4000);
    }
    // 确保 VTK 视图获得焦点，以接收鼠标事件
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

void Widget::setupToolbarModeStack()
{
    if (!ui->tabWidget || !ui->tab_4 || !ui->gridLayout_21)
        return;

    const int sketchTabIdx = ui->tabWidget->indexOf(ui->tab_4);
    if (sketchTabIdx >= 0)
        ui->tabWidget->removeTab(sketchTabIdx);

    const int modelingIdx = ui->tabWidget->indexOf(ui->tab_3);
    if (modelingIdx >= 0)
        ui->tabWidget->setCurrentIndex(modelingIdx);

    toolbarModeStack_ = new QStackedWidget(ui->dockWidgetContents_3);
    toolbarModeStack_->setObjectName(QStringLiteral("toolbarModeStack"));
    toolbarModeStack_->setMinimumSize(0, 0);
    toolbarModeStack_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    ui->gridLayout_21->removeWidget(ui->tabWidget);
    toolbarModeStack_->addWidget(ui->tabWidget);
    toolbarModeStack_->addWidget(ui->tab_4);
    ui->gridLayout_21->addWidget(toolbarModeStack_, 0, 0, 1, 1);
    toolbarModeStack_->setCurrentIndex(0);
    inSketchEnvironment_ = false;
    savedNormalTabIndex_ = ui->tabWidget->currentIndex();
}

void Widget::enterSketchEnvironment()
{
    if (inSketchEnvironment_)
        return;

    if (ui->tabWidget)
        savedNormalTabIndex_ = ui->tabWidget->currentIndex();
    if (toolbarModeStack_)
        toolbarModeStack_->setCurrentIndex(1);
    inSketchEnvironment_ = true;

    // 用 Ribbon 草图上下文页替换「建模」页，不再弹出旧 dock 工具栏
    applySketchRibbonMode(true);

    if (statusBar())
        statusBar()->showMessage(tr("已进入草图环境。"), 2000);
}

void Widget::exitSketchEnvironment()
{
    if (!inSketchEnvironment_)
        return;

    exitSketchCreationMode();
    if (activeSketchCreateDialog_) {
        activeSketchCreateDialog_->close();
        activeSketchCreateDialog_ = nullptr;
    }
    if (currentSelectionMode == SketchPlaneSelection)
        currentSelectionMode = None;

    closeSketchToolInput();
    closeSketchConicDialog();
    closeSketchPolygonDialog();
    closeSketchEllipseDialog();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchEditHover();
    endSketchBrushStroke();

    if (toolbarModeStack_)
        toolbarModeStack_->setCurrentIndex(0);
    if (ui->tabWidget && savedNormalTabIndex_ >= 0 && savedNormalTabIndex_ < ui->tabWidget->count())
        ui->tabWidget->setCurrentIndex(savedNormalTabIndex_);
    inSketchEnvironment_ = false;

    applySketchRibbonMode(false);

    if (vtkWidget && vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
    if (statusBar())
        statusBar()->showMessage(tr("已退出草图环境。"), 2000);
}

void Widget::on_createSketchButton_clicked()
{
    enterSketchEnvironment();
}

void Widget::on_pushButton_6_clicked()
{
    if (hasActiveSketch_ && activeSketch_.isValid()) {
        ensureSketchHistoryRecord();
        if (!activeSketch_.getGeometries().isEmpty())
            updateSketchHistoryShape();
    }
    exitSketchEnvironment();
}

void Widget::rebindHistoryShapeSource(ModelingHistory& history)
{
    if (!history.actor) return;
    IVtkTools_ShapeObject::SetShapeSource(history.shapeDataSource, history.actor);

    // 草图主显示优先使用构建好的折线 polyData，避免被 SetShapeSource 覆盖
    if (history.type == SKETCH && history.polyData) {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(history.polyData);
        mapper->ScalarVisibilityOff();
        history.actor->SetMapper(mapper);
        history.actor->GetProperty()->SetRepresentationToWireframe();
        history.actor->GetProperty()->SetLineWidth(2.0);
        history.actor->GetProperty()->SetLighting(false);
        history.actor->GetProperty()->SetRenderLinesAsTubes(1);
        history.actor->SetVisibility(1);
    }
}

// 处理点选择：在模型上支持“任意点”作为原点（FACE：射线求交；VERTEX/EDGE：原逻辑）
void Widget::handlePointSelection(vtkActor* selectedActor, int x, int y)
{
    if (!selectedActor || !shapePicker) {
        QMessageBox::warning(this, "提示", "请点击模型上的点/边/面来选择原点（当前未拾取到模型）！");
        return;
    }
    
    // 检查是否有活动的对话框
    // 除了长方体/圆柱/圆锥/球体外，这里也支持旋转对话框使用点选择
    QDialog* activeDialog = nullptr;
    if (cuboidDialog) activeDialog = cuboidDialog;
    else if (cylinderDialog) activeDialog = cylinderDialog;
    else if (coneDialog) activeDialog = coneDialog;
    else if (sphereDialog) activeDialog = sphereDialog;
    else if (revolveDialog) activeDialog = revolveDialog;
    else if (patternDialog_) activeDialog = patternDialog_;

    if (!activeDialog) {
        return;
    }
    
    // 获取形状数据源
    IVtkTools_ShapeDataSource* dataSource =
        IVtkTools_ShapeObject::GetShapeSource(selectedActor);
    
    if (!dataSource) {
        QMessageBox::warning(this, "错误", "无法获取形状数据源！");
        return;
    }
    
    Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
    if (shapeWrapper.IsNull()) {
        QMessageBox::warning(this, "错误", "无法获取形状包装器！");
        return;
    }
    
    IVtk_IdType shapeID = shapeWrapper->GetId();
    IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
    
    if (subShapeIds.IsEmpty()) {
        QMessageBox::warning(this, "提示", "未拾取到有效的点或边，请点击模型的顶点或棱！");
        return;
    }
    
    gp_Pnt selectedPoint;
    bool pointFound = false;
    QString pointType = "点";

    // 先构造鼠标屏幕点对应的世界射线（VTK 坐标：原点左下角）
    gp_Pnt rayOrigin;
    gp_Dir rayDir(0, 0, 1);
    bool hasRay = false;
    if (renderer) {
        double worldNear[4] = {0, 0, 0, 1};
        double worldFar[4]  = {0, 0, 1, 1};

        renderer->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 0.0);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(worldNear);

        renderer->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 1.0);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(worldFar);

        if (std::abs(worldNear[3]) > 1e-10) {
            worldNear[0] /= worldNear[3];
            worldNear[1] /= worldNear[3];
            worldNear[2] /= worldNear[3];
        }
        if (std::abs(worldFar[3]) > 1e-10) {
            worldFar[0] /= worldFar[3];
            worldFar[1] /= worldFar[3];
            worldFar[2] /= worldFar[3];
        }

        gp_Pnt p0(worldNear[0], worldNear[1], worldNear[2]);
        gp_Pnt p1(worldFar[0], worldFar[1], worldFar[2]);
        gp_Vec v(p0, p1);
        if (v.Magnitude() > Precision::Confusion()) {
            rayOrigin = p0;
            rayDir = gp_Dir(v);
            hasRay = true;
        }
    }
    
    // 1) 优先：如果拾取到 FACE，则用射线与面求交，得到“点击处任意点”
    if (hasRay) {
        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            IVtk_IdType subShapeId = sIt.Value();
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
            if (subShape.ShapeType() != TopAbs_FACE) continue;

            TopoDS_Face face = TopoDS::Face(subShape);
            gp_Lin ray(rayOrigin, rayDir);

            IntCurvesFace_ShapeIntersector intersector;
            intersector.Load(face, Precision::Confusion());
            // line parameter：0 表示从 rayOrigin 出发，给一个足够远的范围
            intersector.Perform(ray, 0.0, 1.0e9);

            if (intersector.NbPnt() <= 0) {
                continue;
            }

            // 选择最近的正向交点
            Standard_Real bestW = RealLast();
            gp_Pnt bestP;
            bool hasBest = false;
            for (int i = 1; i <= intersector.NbPnt(); ++i) {
                Standard_Real w = intersector.WParameter(i);
                if (w >= 0.0 && w < bestW) {
                    bestW = w;
                    bestP = intersector.Pnt(i);
                    hasBest = true;
                }
            }

            if (hasBest) {
                selectedPoint = bestP;
                pointType = "面点";
                pointFound = true;
                break;
            }
        }
    }
    
    // 如果未拾取到面，继续尝试顶点和边
    if (!pointFound) {
        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            IVtk_IdType subShapeId = sIt.Value();
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
            
            if (subShape.ShapeType() == TopAbs_VERTEX) {
                TopoDS_Vertex vertex = TopoDS::Vertex(subShape);
                selectedPoint = BRep_Tool::Pnt(vertex);
                pointType = "顶点";
                pointFound = true;
                break;
            } else if (subShape.ShapeType() == TopAbs_EDGE) {
                TopoDS_Edge edge = TopoDS::Edge(subShape);

                if (SketchGeometry::edgeMidPoint(edge, selectedPoint)) {
                    pointType = "中点";
                    pointFound = true;
                    break;
                }
            }
        }
    }
    
    if (!pointFound) {
        QMessageBox::warning(this, "提示", "未拾取到有效的点、边或面，请点击模型的顶点、棱或圆形面！");
        return;
    }
    
    // 将坐标传递给对话框
    if (cuboidDialog) {
        cuboidDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
        if (cuboidInteractiveActive_) {
            applyCuboidInteractiveOrigin(selectedPoint);
        }
    } else if (cylinderDialog) {
        cylinderDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (coneDialog) {
        coneDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (sphereDialog) {
        sphereDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (patternDialog_) {
        patternDialog_->setRotationCenter(selectedPoint, true);
        if (patternDialog_->hasDirection1()) {
            updatePatternRotationAxisArrow();
        }
    }
    
    // 保存选中的点并显示
    selectedOriginPoint = selectedPoint;
    hasSelectedOriginPoint = true;
    
    QString label = QString("%1\n(%2, %3, %4)").arg(pointType)
                                                .arg(selectedPoint.X(), 0, 'f', 2)
                                                .arg(selectedPoint.Y(), 0, 'f', 2)
                                                .arg(selectedPoint.Z(), 0, 'f', 2);
    
    // 显示选中的点（放大显示）
    showSelectedPoint(selectedPoint, label);
    
    // 清除悬停提示
    clearPointSelectionHover();
    
    // 退出点选择模式后：块特征保持可继续指定点（A→a），同时仍可拖拽尺寸手柄
    if (cuboidInteractiveActive_) {
        currentSelectionMode = PointSelection;
        updateCuboidInteractivePreview();
    } else if (patternDialog_) {
        restorePatternPitchInteractiveAfterOriginPick();
    } else {
        currentSelectionMode = None;
    }
}

// 获取最近一次点击的世界坐标
void Widget::getLastWorldPoint(double worldPoint[4]) const
{
    worldPoint[0] = lastWorldPoint[0];
    worldPoint[1] = lastWorldPoint[1];
    worldPoint[2] = lastWorldPoint[2];
    worldPoint[3] = lastWorldPoint[3];
}
// 更新历史列表的选中状态
void Widget::updateHistoryListSelection()
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    return;
}

QString Widget::getTypeName(ModelType type)
{
    switch (type) {
    case CYLINDER: return "圆柱体";
    case CONE: return "圆锥体";
    case SPHERE: return "球体";
    case CUBOID: return "长方体";
    case BOOLEAN_RESULT: return "布尔运算";
    case EXTRUSION: return "拉伸";
    case REVOLUTION: return "旋转";
    case FILLET: return "倒角";
    case HOLLOW: return "挖空";
    case DATUM_PLANE: return "基准平面";
    case DATUM_AXIS: return "基准轴";
    case WORK_CSYS: return "工作坐标系";
    case REFERENCE_CSYS: return "基准坐标系";
    case SKETCH: return "草图";
    case PATTERN: return "阵列特征";
    default: return "未知";
    }
}

// 获取选中的 OpenCASCADE 形状
TopoDS_Shape Widget::getSelectedOccShape()
{
    if (currentSelectedIndex < 0 || currentSelectedIndex >= historyList.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个模型！");
        return TopoDS_Shape();
    }

    TopoDS_Shape shape = getShapeFromHistory(currentSelectedIndex);
    if (shape.IsNull()) {
        QMessageBox::warning(this, "警告", "该类型的模型不支持此操作！");
    }
    return shape;
}
// 从历史记录中获取OpenCASCADE形状
TopoDS_Shape Widget::getShapeFromHistory(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return TopoDS_Shape();
    }

    const ModelingHistory& record = historyList[index];

    // 如果历史记录中直接存储了形状，则返回
    if (!record.occShape.IsNull()) {
        return record.occShape;
    }

    // 否则根据类型和参数重新创建形状
    switch (record.type) {
    case CUBOID:
        return PrimitiveGeometry::buildCuboidShape(record.param1, record.param2, record.param3,
                                                   record.hasOrigin, record.originX, record.originY, record.originZ,
                                                   record.axisDirection,
                                                   record.axisReversed,
                                                   record.hasCustomVectorDir,
                                                   record.customVectorDir);
    case CYLINDER:
        return PrimitiveGeometry::buildCylinderShape(record.param1, record.param2,
                                                     record.hasOrigin, record.originX, record.originY, record.originZ,
                                                     record.axisDirection,
                                                     record.axisReversed,
                                                     record.hasCustomVectorDir,
                                                     record.customVectorDir);
    case CONE:
        return PrimitiveGeometry::buildConeShape(record.param1, record.param2, record.param3,
                                                 record.hasOrigin, record.originX, record.originY, record.originZ,
                                                 record.axisDirection,
                                                 record.axisReversed,
                                                 record.hasCustomVectorDir,
                                                 record.customVectorDir);
    case SPHERE:
        return PrimitiveGeometry::buildSphereShape(record.param1, record.hasOrigin, record.originX, record.originY, record.originZ);
    case BOOLEAN_RESULT:
        // 对于布尔运算结果，我们期望occShape已经被存储
        return record.occShape;
    default:
        return TopoDS_Shape();
    }
}

// 更新历史列表的可见性显示
void Widget::updateHistoryListVisibility()
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    return;
}

// 显示所有模型
void Widget::showAllModels()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor) {
            historyList[i].actor->SetVisibility(true);
            historyList[i].actor->SetPickable(true);
        }
        // 关键修改：同时显示所有轮廓线
        if (historyList[i].outlineActor) {
            historyList[i].outlineActor->SetVisibility(true);
        }
    }

    // 更新渲染和列表显示
    vtkWidget->renderWindow()->Render();
    updateHistoryListVisibility();

}
