#include "mirror_view_window.h"

#include "main_window.h"
#include "mirror_view_state.h"
#include "rendering/model/model_display_style.h"
#include "rendering/pipeline/model_shape_pipeline.h"
#include "ui_main_window.h"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_ShapePicker.hxx>

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkCamera.h>
#include <vtkArrowSource.h>
#include <vtkSmartPointer.h>
#include <vtkAxesActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLineSource.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

MirrorRenderWindow::MirrorRenderWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QStringLiteral("建模视图 - 新窗口"));
    resize(960, 640);

    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    vtkWidget = new QVTKOpenGLNativeWidget(container);
    vtkWidget->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(vtkWidget);
    setCentralWidget(container);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkWidget->renderWindow()->AddRenderer(renderer);
    Widget::configureSceneLights(renderer);
    renderer->SetBackground(0.8, 0.8, 0.8);
    renderer->SetBackground2(0.9, 0.9, 0.9);
    renderer->GradientBackgroundOn();
    renderer->ResetCamera();
}

namespace {

vtkSmartPointer<vtkActor> buildMirrorActorFromHistory(const ModelingHistory& record,
                                                      const ModelGeometryState& geometryState,
                                                      const ModelRenderState& renderState)
{
    vtkSmartPointer<vtkPolyData> renderData = nullptr;

    if (!geometryState.occShape.IsNull()) {
        try {
            Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(geometryState.occShape);
            vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
                ModelShapePipeline::createShapeDataSource(shapeWrapper);
            renderData = ModelShapePipeline::copyShapeDataSourceOutput(shapeDataSource, true);
        } catch (...) {
            renderData = nullptr;
        }
    }

    if (!renderData && renderState.polyData) {
        renderData = vtkSmartPointer<vtkPolyData>::New();
        renderData->ShallowCopy(renderState.polyData);
    }

    if (!renderData) {
        return nullptr;
    }

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(renderData);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (renderState.actor) {
        const double* c = renderState.actor->GetProperty()->GetColor();
        actor->GetProperty()->SetColor(c[0], c[1], c[2]);
        actor->GetProperty()->SetOpacity(renderState.actor->GetProperty()->GetOpacity());
        actor->GetProperty()->SetInterpolation(renderState.actor->GetProperty()->GetInterpolation());
        actor->GetProperty()->SetSpecular(renderState.actor->GetProperty()->GetSpecular());
        actor->GetProperty()->SetSpecularPower(renderState.actor->GetProperty()->GetSpecularPower());
        actor->GetProperty()->SetAmbient(renderState.actor->GetProperty()->GetAmbient());
        actor->GetProperty()->SetDiffuse(renderState.actor->GetProperty()->GetDiffuse());
        actor->GetProperty()->SetEdgeVisibility(renderState.actor->GetProperty()->GetEdgeVisibility());
    } else {
        actor->GetProperty()->SetColor(record.color.redF(), record.color.greenF(), record.color.blueF());
        ModelDisplayStyle::applySolidActorMaterial(actor->GetProperty());
    }

    const bool visible = renderState.actor ? (renderState.actor->GetVisibility() != 0) : true;
    actor->SetVisibility(visible ? 1 : 0);
    return actor;
}

} // namespace

void Widget::on_WindowpushButton_clicked()
{
    // 1. 创建菜单
    QMenu *windowMenu = new QMenu(this);

    // 2. 创建你需要的菜单项
    QAction *newWindowAction = new QAction("新建窗口", this);
    QAction *layoutAction = new QAction("窗口布局", this);
    QAction *resetLayoutAction = new QAction("重置布局", this);
    QAction *switchWindowAction = new QAction("切换窗口", this);
    QAction *changeDisplayAction = new QAction("更改显示部件", this);

    // 3. 设置快捷键
    //newWindowAction->setShortcut(QKeySequence("Ctrl+N"));
    //resetLayoutAction->setShortcut(QKeySequence("Ctrl+R"));

    // 4. 添加菜单项（保持你想要的顺序）
    windowMenu->addAction(newWindowAction);
    windowMenu->addSeparator();  // 分隔线
    windowMenu->addAction(layoutAction);
    windowMenu->addAction(resetLayoutAction);
    windowMenu->addSeparator();
    windowMenu->addAction(switchWindowAction);
    windowMenu->addAction(changeDisplayAction);

    // 5. 连接信号到对应的槽函数
    connect(newWindowAction, &QAction::triggered, this, &Widget::createNewWindow);
    connect(layoutAction, &QAction::triggered, this, &Widget::showWindowLayoutDialog);
    connect(resetLayoutAction, &QAction::triggered, this, &Widget::resetWindowLayout);
    connect(switchWindowAction, &QAction::triggered, this, &Widget::switchActiveWindow);
    connect(changeDisplayAction, &QAction::triggered, this, &Widget::changeDisplayWidget);

    // 6. 在按钮下方显示菜单
    QPoint menuPos = ui->WindowpushButton->mapToGlobal(QPoint(0, ui->WindowpushButton->height()));
    windowMenu->exec(menuPos);

    // 7. 清理内存
    windowMenu->deleteLater();
}

void Widget::createNewWindow()
{
    auto* mirrorWindow = new MirrorRenderWindow();
    mirrorWindow->setWindowTitle(QStringLiteral("建模视图 - 窗口%1").arg(g_mirrorWindowCounter++));
    g_mirrorWindowMap[this].append(QPointer<MirrorRenderWindow>(mirrorWindow));

    QToolBar* modelToolbar = new QToolBar(QStringLiteral("建模"), mirrorWindow);
    modelToolbar->setMovable(false);
    mirrorWindow->addToolBar(Qt::TopToolBarArea, modelToolbar);
    mirrorWindow->statusBar()->showMessage(QStringLiteral("已创建独立建模视图窗口"), 2000);
    mirrorWindow->vtkWidget->installEventFilter(this);

    // 为该子窗口创建独立 picker 与交互样式
    vtkSmartPointer<IVtkTools_ShapePicker> mirrorPicker = vtkSmartPointer<IVtkTools_ShapePicker>::New();
    mirrorPicker->SetTolerance(0.05);
    mirrorPicker->SetRenderer(mirrorWindow->renderer);

    vtkSmartPointer<MouseInteractorStyle> mirrorStyle = vtkSmartPointer<MouseInteractorStyle>::New();
    mirrorStyle->SetInteractionContext(createMouseInteractionContext());
    mirrorStyle->SetPreEventHook([this, mirrorWindow]() { activateMirrorRenderContext(mirrorWindow); });
    mirrorWindow->vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(mirrorStyle);

    MirrorRenderContext mirrorCtx;
    mirrorCtx.vtkWidget = mirrorWindow->vtkWidget;
    mirrorCtx.renderer = mirrorWindow->renderer;
    mirrorCtx.shapePicker = mirrorPicker;

    // 子窗口独立三重轴（含可交互立方体）
    vtkSmartPointer<vtkAxesActor> mirrorAxesActor = vtkSmartPointer<vtkAxesActor>::New();
    mirrorAxesActor->SetTotalLength(0.7, 0.7, 0.7);
    vtkSmartPointer<vtkAnnotatedCubeActor> cubeActor = vtkSmartPointer<vtkAnnotatedCubeActor>::New();
    cubeActor->SetXPlusFaceText("+X");
    cubeActor->SetXMinusFaceText("-X");
    cubeActor->SetYPlusFaceText("+Y");
    cubeActor->SetYMinusFaceText("-Y");
    cubeActor->SetZPlusFaceText("+Z");
    cubeActor->SetZMinusFaceText("-Z");
    cubeActor->SetFaceTextScale(0.35);
    cubeActor->GetCubeProperty()->SetColor(0.96, 0.96, 0.96);
    cubeActor->GetTextEdgesProperty()->SetColor(0.12, 0.12, 0.12);

    // 子窗口与主窗口一致：L0 主场景 + L3 三重轴（L1/L2 预留给覆盖/参考，镜像暂空）
    mirrorWindow->vtkWidget->renderWindow()->SetNumberOfLayers(4);
    mirrorWindow->renderer->SetLayer(0);
    mirrorCtx.centerAxesRenderer = vtkSmartPointer<vtkRenderer>::New();
    mirrorCtx.centerAxesRenderer->SetLayer(3);
    mirrorCtx.centerAxesRenderer->SetViewport(0.0, 0.0, 0.2, 0.2);
    mirrorCtx.centerAxesRenderer->SetErase(0);
    mirrorWindow->vtkWidget->renderWindow()->AddRenderer(mirrorCtx.centerAxesRenderer);
    mirrorCtx.centerAxesRenderer->GetActiveCamera()->ParallelProjectionOn();
    mirrorCtx.centerAxesRenderer->GetActiveCamera()->SetPosition(0, 0, 5);
    mirrorCtx.centerAxesRenderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    mirrorCtx.centerAxesRenderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    mirrorCtx.centerAxesRenderer->GetActiveCamera()->SetParallelScale(1.2);

    auto makeMirrorAxisArrowActor = [this](AxisDirection axis, double r, double g, double b) -> vtkSmartPointer<vtkActor> {
        vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
        arrow->SetTipLength(0.35);
        arrow->SetTipRadius(0.10);
        arrow->SetShaftRadius(0.03);
        vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
        if (axis == AxisDirection::Y) t->RotateZ(90.0);
        else if (axis == AxisDirection::Z) t->RotateY(-90.0);
        t->Scale(centerAxesBaseScale, centerAxesBaseScale, centerAxesBaseScale);
        vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetTransform(t);
        tf->SetInputConnection(arrow->GetOutputPort());
        tf->Update();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetOpacity(0.4);
        actor->GetProperty()->SetAmbient(0.7);
        actor->GetProperty()->SetDiffuse(0.3);
        actor->SetPickable(1);
        return actor;
    };

    mirrorCtx.centerAxisXActor = makeMirrorAxisArrowActor(AxisDirection::X, 1.0, 0.2, 0.2);
    mirrorCtx.centerAxisYActor = makeMirrorAxisArrowActor(AxisDirection::Y, 0.2, 0.9, 0.2);
    mirrorCtx.centerAxisZActor = makeMirrorAxisArrowActor(AxisDirection::Z, 0.2, 0.4, 1.0);
    mirrorCtx.centerAxesRenderer->AddActor(mirrorCtx.centerAxisXActor);
    mirrorCtx.centerAxesRenderer->AddActor(mirrorCtx.centerAxisYActor);
    mirrorCtx.centerAxesRenderer->AddActor(mirrorCtx.centerAxisZActor);

    auto addMirrorAxisLabel = [&](const char* text, double x, double y, double z, double r, double g, double b) {
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(text);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(textSource->GetOutputPort());
        vtkSmartPointer<vtkActor> labelActor = vtkSmartPointer<vtkActor>::New();
        labelActor->SetMapper(mapper);
        labelActor->SetScale(0.25);
        labelActor->SetPosition(x, y, z);
        labelActor->GetProperty()->SetColor(r, g, b);
        labelActor->GetProperty()->SetAmbient(1.0);
        labelActor->GetProperty()->SetDiffuse(0.0);
        labelActor->SetPickable(0);
        mirrorCtx.centerAxesRenderer->AddActor(labelActor);
    };
    addMirrorAxisLabel("X", 1.05, 0.0, 0.0, 1.0, 0.2, 0.2);
    addMirrorAxisLabel("Y", 0.0, 1.05, 0.0, 0.2, 0.9, 0.2);
    addMirrorAxisLabel("Z", 0.0, 0.0, 1.05, 0.2, 0.4, 1.0);

    const double s = 0.42;
    const int faceIds[6] = {0,1,2,3,4,5};
    for (int idx = 0; idx < 6; ++idx) {
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        switch (faceIds[idx]) {
        case 0: plane->SetOrigin(-s, -s, s); plane->SetPoint1(s, -s, s); plane->SetPoint2(-s, s, s); break;
        case 1: plane->SetOrigin(-s, -s, -s); plane->SetPoint1(-s, s, -s); plane->SetPoint2(s, -s, -s); break;
        case 2: plane->SetOrigin(-s, -s, -s); plane->SetPoint1(-s, s, -s); plane->SetPoint2(-s, -s, s); break;
        case 3: plane->SetOrigin(s, -s, -s); plane->SetPoint1(s, -s, s); plane->SetPoint2(s, s, -s); break;
        case 4: plane->SetOrigin(-s, s, -s); plane->SetPoint1(s, s, -s); plane->SetPoint2(-s, s, s); break;
        case 5: plane->SetOrigin(-s, -s, -s); plane->SetPoint1(-s, -s, s); plane->SetPoint2(s, -s, -s); break;
        }
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0.85, 0.85, 0.85);
        actor->GetProperty()->SetOpacity(0.18);
        actor->GetProperty()->SetAmbient(0.9);
        actor->GetProperty()->SetDiffuse(0.1);
        actor->SetPickable(1);
        mirrorCtx.centerTriadFaceActors[idx] = actor;
        mirrorCtx.centerAxesRenderer->AddActor(actor);
    }

    const double v[8][3] = {
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s},
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}
    };
    const int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; ++i) {
        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(v[e[i][0]]);
        line->SetPoint2(v[e[i][1]]);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(line->GetOutputPort());
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0.35, 0.35, 0.35);
        actor->GetProperty()->SetLineWidth(2.2);
        actor->SetPickable(1);
        mirrorCtx.centerTriadEdgeActors[i] = actor;
        mirrorCtx.centerAxesRenderer->AddActor(actor);
    }

    mirrorCtx.centerTriadCurrentFace = triadFaceFromViewName(QStringLiteral("正三轴测视图"));
    mirrorCtx.triadWidget = nullptr;

    g_mirrorRenderContextMap[this][mirrorWindow] = mirrorCtx;
    activateMirrorRenderContext(mirrorWindow);
    refreshCenterTriadFaceStyle();
    syncCenterAxisCamera();
    activateMainRenderContext();

    const auto addActionWithSync = [this, mirrorWindow, modelToolbar](const QString& text, auto slotFunc) {
        QAction* action = modelToolbar->addAction(text);
        connect(action, &QAction::triggered, this, [this, mirrorWindow, slotFunc]() {
            // 建模数据始终在主上下文修改，避免子窗口上下文写入导致主窗口丢失/崩溃。
            syncMainCameraFromWindowRenderer(mirrorWindow->renderer);
            activateMainRenderContext();
            // 对话框父级应跟随当前子窗口，而不是主窗口。
            // activateMainRenderContext() 会把活动窗口重置为主窗口，这里显式改回子窗口。
            g_activeStatusWindowMap[this] = mirrorWindow;
            (this->*slotFunc)();
            // 命令发起后回到当前子窗口，保持交互连贯。
            activateMirrorRenderContext(mirrorWindow);
            if (vtkWidget) vtkWidget->setFocus();
        });
    };

    // 顺序对齐主界面“建模”页：基本建模 -> 基准特征 -> 细节特征 -> 几何运算 -> 工具 -> 草图 -> 历史流
    addActionWithSync(QStringLiteral("长方体"), &Widget::on_cuboid_clicked);
    addActionWithSync(QStringLiteral("球体"), &Widget::on_sphere_clicked);
    addActionWithSync(QStringLiteral("圆柱"), &Widget::on_cylinder_clicked);
    addActionWithSync(QStringLiteral("圆锥"), &Widget::on_cone_clicked);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("基准轴"), &Widget::on_pushButton_4_clicked);
    addActionWithSync(QStringLiteral("工作坐标系"), &Widget::on_workAxisButton_clicked);
    addActionWithSync(QStringLiteral("基准平面"), &Widget::on_datum_plane_Button_clicked);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("拉伸"), &Widget::on_extrude_clicked);
    addActionWithSync(QStringLiteral("阵列特征"), &Widget::on_patternFeature_clicked);
    addActionWithSync(QStringLiteral("圆角"), &Widget::on_fillet_clicked);
    addActionWithSync(QStringLiteral("旋转"), &Widget::on_revolve_clicked);
    addActionWithSync(QStringLiteral("倒角"), &Widget::on_chamfer_clicked);
    addActionWithSync(QStringLiteral("挖空"), &Widget::handleHollow);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("布尔"), &Widget::on_boolOperationButton_clicked);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("表达式"), &Widget::on_expressionBtn_clicked);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("创建草图"), &Widget::on_createSketchButton_clicked);
    addActionWithSync(QStringLiteral("轮廓"), &Widget::on_pushButton_7_clicked);
    addActionWithSync(QStringLiteral("直线"), &Widget::on_pushButton_40_clicked);
    addActionWithSync(QStringLiteral("长方体草图"), &Widget::on_pushButton_41_clicked);
    addActionWithSync(QStringLiteral("圆弧"), &Widget::on_pushButton_42_clicked);
    addActionWithSync(QStringLiteral("圆"), &Widget::on_pushButton_11_clicked);
    addActionWithSync(QStringLiteral("草图点"), &Widget::on_pushButton_12_clicked);
    addActionWithSync(QStringLiteral("二次曲线"), &Widget::on_pushButton_13_clicked);
    addActionWithSync(QStringLiteral("快速修剪"), &Widget::startSketchQuickTrim);
    addActionWithSync(QStringLiteral("快速延伸"), &Widget::startSketchQuickExtend);
    modelToolbar->addSeparator();
    addActionWithSync(QStringLiteral("撤销"), &Widget::on_undoButton_clicked);
    addActionWithSync(QStringLiteral("重做"), &Widget::on_redoButton_clicked);
    modelToolbar->addSeparator();

    const auto addViewAction = [this, mirrorWindow, modelToolbar](const QString& text, const QString& viewName) {
        QAction* action = modelToolbar->addAction(text);
        connect(action, &QAction::triggered, this, [this, mirrorWindow, viewName]() {
            activateMirrorRenderContext(mirrorWindow);
            switchToView(viewName);
        });
    };
    addViewAction(QStringLiteral("前"), QStringLiteral("正视图"));
    addViewAction(QStringLiteral("俯"), QStringLiteral("俯视图"));
    addViewAction(QStringLiteral("左"), QStringLiteral("左视图"));
    addViewAction(QStringLiteral("右"), QStringLiteral("右视图"));
    addViewAction(QStringLiteral("后"), QStringLiteral("后视图"));
    addViewAction(QStringLiteral("仰"), QStringLiteral("仰视图"));
    addViewAction(QStringLiteral("ISO"), QStringLiteral("正三轴测视图"));

    QAction* triadRightBottomAction = modelToolbar->addAction(QStringLiteral("三重轴右下角"));
    triadRightBottomAction->setCheckable(true);
    triadRightBottomAction->setChecked(false);
    connect(triadRightBottomAction, &QAction::toggled, this, [this, mirrorWindow](bool checked) {
        activateMirrorRenderContext(mirrorWindow);
        if (!g_mirrorRenderContextMap.contains(this)) return;
        auto& map = g_mirrorRenderContextMap[this];
        if (!map.contains(mirrorWindow)) return;
        auto triad = map[mirrorWindow].triadWidget;
        if (!triad) return;
        triad->SetViewport(checked ? 0.8 : 0.0, 0.0, checked ? 1.0 : 0.2, 0.2);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    });

    // 关闭时自动从管理列表移除
    connect(mirrorWindow, &QObject::destroyed, this, [this, mirrorWindow]() {
        auto& windows = g_mirrorWindowMap[this];
        windows.removeAll(QPointer<MirrorRenderWindow>(mirrorWindow));
        g_mirrorRenderContextMap[this].remove(mirrorWindow);
        g_activeStatusWindowMap[this] = this;
        activateMainRenderContext();
    });

    syncMirrorWindows();
    mirrorWindow->show();
}

void Widget::syncMainCameraFromWindowRenderer(vtkRenderer* sourceRenderer)
{
    if (!sourceRenderer || !renderer || !vtkWidget || !vtkWidget->renderWindow()) {
        return;
    }

    vtkCamera* sourceCamera = sourceRenderer->GetActiveCamera();
    vtkCamera* mainCamera = renderer->GetActiveCamera();
    if (!sourceCamera || !mainCamera) {
        return;
    }

    mainCamera->DeepCopy(sourceCamera);
    renderer->ResetCameraClippingRange();
    syncCenterAxisCamera();
    vtkWidget->renderWindow()->Render();
}

void Widget::activateMainRenderContext()
{
    if (!g_mainVtkWidgetMap.contains(this) || !g_mainRendererMap.contains(this) || !g_mainPickerMap.contains(this)) {
        return;
    }
    vtkWidget = g_mainVtkWidgetMap.value(this);
    renderer = g_mainRendererMap.value(this);
    shapePicker = g_mainPickerMap.value(this);
    g_activeStatusWindowMap[this] = this;
    // 切回主窗口后立即按主上下文刷新拾取绑定，避免仍残留子窗口 actor 绑定导致不可拾取。
    refreshShapePickerBindingsForCurrentContext(0.05, false);
}

void Widget::activateMirrorRenderContext(QObject* windowKey)
{
    if (!g_mirrorRenderContextMap.contains(this)) {
        return;
    }
    auto& map = g_mirrorRenderContextMap[this];
    if (!map.contains(windowKey)) {
        return;
    }
    const MirrorRenderContext& ctx = map.value(windowKey);
    if (!ctx.vtkWidget || !ctx.renderer || !ctx.shapePicker) {
        return;
    }

    vtkWidget = ctx.vtkWidget;
    renderer = ctx.renderer;
    shapePicker = ctx.shapePicker;
    if (auto* host = qobject_cast<QMainWindow*>(windowKey)) {
        g_activeStatusWindowMap[this] = host;
    }
    // 切换到子窗口时同步子窗口镜像 actor 的拾取绑定，保证多窗口来回切换稳定。
    refreshShapePickerBindingsForCurrentContext(0.05, false);
}

void Widget::prepareShapePickerBindingsForCurrentContext()
{
    if (!shapePicker) return;


    auto unbindMainActors = [this]() {
        for (int i = 0; i < historyList.size(); ++i) {
            ModelRenderState& state = renderStateFor(historyList[i]);
            if (state.actor) {
                state.actor->SetPickable(0);
                IVtkTools_ShapeObject::SetShapeSource(nullptr, state.actor);
            }
            if (state.profilePickActor) {
                state.profilePickActor->SetPickable(0);
                IVtkTools_ShapeObject::SetShapeSource(nullptr, state.profilePickActor);
            }
        }
    };

    auto unbindAllMirrorActors = [this]() {
        if (!g_mirrorRenderContextMap.contains(this)) return;
        auto& map = g_mirrorRenderContextMap[this];
        for (auto it = map.begin(); it != map.end(); ++it) {
            MirrorRenderContext& ctx = it.value();
            for (vtkActor* mirrorActor : ctx.modelActors) {
                if (!mirrorActor) continue;
                mirrorActor->SetPickable(0);
                IVtkTools_ShapeObject::SetShapeSource(nullptr, mirrorActor);
            }
        }
    };

    // 子窗口：仅绑定当前子窗口的镜像 actor（避免与主窗口 actor 抢同一 ShapeSource 绑定）。
    bool mirrorContextMatched = false;
    if (g_mirrorRenderContextMap.contains(this)) {
        auto& map = g_mirrorRenderContextMap[this];
        for (auto it = map.begin(); it != map.end(); ++it) {
            MirrorRenderContext& ctx = it.value();
            if (ctx.vtkWidget != vtkWidget) continue;
            mirrorContextMatched = true;

            // 先清空其它上下文绑定，只保留当前活动窗口的绑定。
            unbindMainActors();
            unbindAllMirrorActors();

            for (int i = 0; i < historyList.size(); ++i) {
                if (i >= ctx.modelActors.size()) continue;
                vtkActor* mirrorActor = ctx.modelActors[i];
                if (!mirrorActor || !renderStateFor(historyList[i]).shapeDataSource) continue;

                const bool visible = (renderStateFor(historyList[i]).actor && renderStateFor(historyList[i]).actor->GetVisibility() != 0);
                mirrorActor->SetVisibility(visible ? 1 : 0);
                mirrorActor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                        && historyList[i].type != DATUM_AXIS
                                        && historyList[i].type != WORK_CSYS
                                        && historyList[i].type != REFERENCE_CSYS);
                if (visible) {
                    IVtkTools_ShapeObject::SetShapeSource(renderStateFor(historyList[i]).shapeDataSource, mirrorActor);
                } else {
                    IVtkTools_ShapeObject::SetShapeSource(nullptr, mirrorActor);
                }
            }
            break;
        }
    }

    // 主窗口：仅绑定主窗口 actor。
    if (mirrorContextMatched) {
        return;
    }

    // 先解绑所有子窗口，避免和主窗口争抢 ShapeSource 绑定。
    unbindAllMirrorActors();
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor && renderStateFor(historyList[i]).shapeDataSource) {
            const bool visible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            renderStateFor(historyList[i]).actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, renderStateFor(historyList[i]).actor);
            }
            if (renderStateFor(historyList[i]).profilePickActor) {
                const bool profilePickContext =
                    extrusionDialog
                    && (currentSelectionMode == ExtrusionSelection
                        || currentSelectionMode == EdgeSelection
                        || currentSelectionMode == FaceSelection);
                const bool profilePickable =
                    visible && historyList[i].type == SKETCH && profilePickContext;
                renderStateFor(historyList[i]).profilePickActor->SetVisibility(
                    visible && historyList[i].type == SKETCH ? 1 : 0);
                renderStateFor(historyList[i]).profilePickActor->SetPickable(profilePickable ? 1 : 0);
                if (profilePickable) {
                    IVtkTools_ShapeObject::SetShapeSource(
                        renderStateFor(historyList[i]).profilePickShapeDataSource,
                        renderStateFor(historyList[i]).profilePickActor);
                } else {
                    IVtkTools_ShapeObject::SetShapeSource(
                        nullptr, renderStateFor(historyList[i]).profilePickActor);
                }
            }
        }
    }
}

int Widget::resolveHistoryIndexByActor(vtkActor* actor) const
{
    if (!actor) return -1;
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor == actor
            || renderStateFor(historyList[i]).profilePickActor == actor) {
            return i;
        }
    }

    if (g_mirrorRenderContextMap.contains(this)) {
        const auto& map = g_mirrorRenderContextMap[this];
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it.value().modelActorToHistoryIndex.contains(actor)) {
                return it.value().modelActorToHistoryIndex.value(actor);
            }
        }
    }
    return -1;
}

bool Widget::handleMirrorTriadClick(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return false;
}

void Widget::syncMirrorWindows()
{
    if (!g_mirrorWindowMap.contains(this)) {
        return;
    }

    auto& windows = g_mirrorWindowMap[this];
    for (int i = windows.size() - 1; i >= 0; --i) {
        auto mirrorWindow = windows[i];
        if (!mirrorWindow) {
            windows.removeAt(i);
            continue;
        }

        if (!mirrorWindow->renderer || !mirrorWindow->vtkWidget || !mirrorWindow->vtkWidget->renderWindow()) {
            continue;
        }

        mirrorWindow->renderer->RemoveAllViewProps();
        if (g_mirrorRenderContextMap.contains(this) && g_mirrorRenderContextMap[this].contains(mirrorWindow)) {
            g_mirrorRenderContextMap[this][mirrorWindow].modelActors.clear();
            g_mirrorRenderContextMap[this][mirrorWindow].modelActorToHistoryIndex.clear();
        }

        // 子窗口使用独立渲染 actor，避免同一 actor 在多 renderer 间迁移导致空白/崩溃。
        int historyIndex = 0;
        for (const ModelingHistory& record : historyList) {
            auto actor = buildMirrorActorFromHistory(record, geometryStateFor(record), renderStateFor(record));
            if (actor) {
                const bool visible = (renderStateFor(record).actor && renderStateFor(record).actor->GetVisibility() != 0);
                const bool pickable = visible && record.type != DATUM_PLANE
                                        && record.type != DATUM_AXIS
                                        && record.type != WORK_CSYS
                                        && record.type != REFERENCE_CSYS;
                actor->SetVisibility(visible ? 1 : 0);
                actor->SetPickable(pickable ? 1 : 0);
                // 拾取绑定在 prepareShapePickerBindingsForCurrentContext() 中按“当前活动窗口”单路处理。
                IVtkTools_ShapeObject::SetShapeSource(nullptr, actor);

                mirrorWindow->renderer->AddActor(actor);
                if (g_mirrorRenderContextMap.contains(this) && g_mirrorRenderContextMap[this].contains(mirrorWindow)) {
                    g_mirrorRenderContextMap[this][mirrorWindow].modelActors.append(actor);
                    g_mirrorRenderContextMap[this][mirrorWindow].modelActorToHistoryIndex.insert(actor, historyIndex);
                }
            }
            ++historyIndex;
        }

        mirrorWindow->renderer->ResetCameraClippingRange();
        mirrorWindow->vtkWidget->renderWindow()->Render();
    }

    // 同步完成后立即刷新当前活动上下文的拾取绑定，避免“刚建模后首次点击无响应”。
    refreshShapePickerBindingsForCurrentContext(0.05, false);
}

void Widget::showWindowLayoutDialog()
{
    // 可以弹出一个对话框让用户选择布局方式
    // 例如：平铺、层叠、双视图等
    // QDialog *layoutDialog = new QDialog(this);
    // ... 设置布局选项
}

void Widget::resetWindowLayout()
{
    // 恢复窗口的默认布局
    // 例如：关闭所有子窗口，只保留主VTK窗口
    // resetToDefaultLayout();
}

void Widget::switchActiveWindow()
{
    // 在多窗口间切换焦点
    // 可以弹出一个窗口选择列表
    // showWindowList();
}

void Widget::changeDisplayWidget()
{
    // 更改当前窗口的显示内容
    // 例如：在3D视图、树状列表、属性编辑器之间切换
    // switchDisplayMode();
}

// Dock widget 位置交换处理函数（现在只有dockWidget需要处理）
void Widget::onDockWidgetLocationChanged(Qt::DockWidgetArea area)
{
    Q_UNUSED(area);
    // 3D视图已经移到centralwidget，不再需要位置交换逻辑
    // 保留此函数以防将来需要
}
