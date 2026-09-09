// Main VTK viewport setup, oriented views, camera helpers, and picking entry points.
#include "main_window.h"
#include "mirror_view_types.h"
#include "mirror_view_state.h"
#include "ui_main_window.h"

#include <gp_Dir.hxx>
#include <gp_Vec.hxx>

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLight.h>
#include <vtkMapper.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLState.h>
#include <vtkProperty.h>
#include <vtkPropPicker.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include "geometry/sketch/sketch_geometry.h"

#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif

#include <BRep_Tool.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include "common/modeltype.h"

void Widget::setupViewUiConnections()
{
    // TabWidget 里的“定向视图”按钮（main_window.ui 内的 objectName）
    // 这里统一用按钮 text 作为 viewName，避免重复写死映射关系。
    auto connectViewButton = [this](QPushButton* btn) {
        if (!btn) return;
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            const QString viewName = btn->text();
            switchToView(viewName);
            syncViewSelectionInTree(viewName);
        });
    };

    connectViewButton(ui->pushButton);     // 正视图
    connectViewButton(ui->pushButton_2);   // 左视图
    connectViewButton(ui->pushButton_3);   // 右视图
    connectViewButton(ui->pushButton_29);  // 俯视图
    connectViewButton(ui->pushButton_17);  // 仰视图（或 UI 上的文字）
    connectViewButton(ui->pushButton_27);  // 后视图
    connectViewButton(ui->pushButton_28);  // 正三轴视图
}

void Widget::syncViewSelectionInTree(const QString& viewName)
{
    if (!ui || !ui->treeWidget) return;

    // 规范化名称，避免 ui/树里存在别名或错别字导致无法匹配
    QString name = viewName.trimmed();
    if (name == "正视图") name = "前视图";
    if (name == "正三轴视图" || name == "正三轴侧视图") name = "正三轴测视图";
    if (name == "正等测图") name = "正三轴测视图";

    const QSignalBlocker blocker(ui->treeWidget);

    auto selectChildIfMatch = [&](const QString& parentName) {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* top = ui->treeWidget->topLevelItem(i);
            if (!top || top->text(0) != parentName) continue;
            for (int c = 0; c < top->childCount(); ++c) {
                QTreeWidgetItem* child = top->child(c);
                if (!child) continue;
                QString t = child->text(0).trimmed();
                if (t == "正视图") t = "前视图";
                if (t == "正三轴视图" || t == "正三轴侧视图") t = "正三轴测视图";
                if (t == "正等测图") t = "正三轴测视图";
                if (t == name) {
                    ui->treeWidget->setCurrentItem(child, 0, QItemSelectionModel::ClearAndSelect);
                    return true;
                }
            }
        }
        return false;
    };

    // 只同步“模型视图”，避免 setCurrentItem(child) 自动展开“摄像机”子树
    selectChildIfMatch("模型视图");

    // 同时确保“摄像机”顶层项保持折叠（仅允许用户手动点击展开）
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* top = ui->treeWidget->topLevelItem(i);
        if (top && top->text(0) == "摄像机") {
            top->setExpanded(false);
            break;
        }
    }
}

void Widget::configureSceneLights(vtkRenderer* ren)
{
    if (!ren) return;

    // 参考 UG View 典型三点光：Ambient 0.2 + Key 1.0（约 45°）+ Fill 0.3（对侧）
    ren->AutomaticLightCreationOff();
    ren->RemoveAllLights();

    auto key = vtkSmartPointer<vtkLight>::New();
    key->SetLightTypeToCameraLight();
    // 相对视线约 45°：侧上方主光
    key->SetPosition(0.55, 0.55, 1.0);
    key->SetFocalPoint(0.0, 0.0, 0.0);
    key->SetColor(1.0, 1.0, 1.0);
    key->SetIntensity(1.0);
    ren->AddLight(key);

    auto fill = vtkSmartPointer<vtkLight>::New();
    fill->SetLightTypeToCameraLight();
    // 对侧补光，软化阴影
    fill->SetPosition(-0.55, -0.25, 0.85);
    fill->SetFocalPoint(0.0, 0.0, 0.0);
    fill->SetColor(1.0, 1.0, 1.0);
    fill->SetIntensity(0.3);
    ren->AddLight(fill);

    auto ambient = vtkSmartPointer<vtkLight>::New();
    ambient->SetLightTypeToHeadlight();
    ambient->SetColor(1.0, 1.0, 1.0);
    ambient->SetIntensity(0.2);
    ren->AddLight(ambient);

    ren->SetTwoSidedLighting(true);
}

void Widget::applyMarkerSphereMaterial(vtkProperty* prop, MarkerSphereStyle style)
{
    if (!prop) return;
    prop->SetInterpolationToPhong();
    prop->SetLighting(true);
    prop->SetRepresentationToSurface();
    prop->SetEdgeVisibility(0);
    prop->SetSpecularColor(1.0, 1.0, 1.0);

    if (style == MarkerSphereStyle::ConfirmedGreen) {
        // 翠绿玻璃珠：高光更“水润”
        prop->SetColor(0.12, 0.78, 0.42);
        prop->SetAmbient(0.22);
        prop->SetDiffuse(0.70);
        prop->SetSpecular(0.92);
        prop->SetSpecularPower(72);
        prop->SetOpacity(0.96);
    } else {
        // 琥珀悬停珠
        prop->SetColor(1.0, 0.82, 0.18);
        prop->SetAmbient(0.24);
        prop->SetDiffuse(0.68);
        prop->SetSpecular(0.88);
        prop->SetSpecularPower(64);
        prop->SetOpacity(0.94);
    }
}

void Widget::configureMarkerSphereSource(vtkSphereSource* sphere, double radius)
{
    if (!sphere) return;
    sphere->SetCenter(0.0, 0.0, 0.0);
    sphere->SetRadius(radius);
    sphere->SetPhiResolution(48);
    sphere->SetThetaResolution(48);
    sphere->LatLongTessellationOff();
}

// （移除）不要重建 shapePicker：工程中已知会导致只识别最后绑定的模型
// 设置VTK
void Widget::setupVTK()
{
    activateMainRenderContext();
    // VTK窗口现在在centralwidget中，直接使用widget_vtkContainer
    vtkWidget = ui->widget_vtkContainer;
    vtkWidget->setFocusPolicy(Qt::StrongFocus);  // 确保可以接收焦点
    vtkWidget->installEventFilter(this);

    // 创建渲染器
    renderer = vtkRenderer::New();
    vtkWidget->renderWindow()->AddRenderer(renderer);
    configureSceneLights(renderer);
    renderPipeline_.initialize(vtkWidget->renderWindow(), renderer,
                               [](vtkRenderer* ren) { Widget::configureSceneLights(ren); });

    // 全局 coincident：factor 必须为 0（factor*DZ 随拉远变大 → 背面轮廓“越远越完整”）。
    // 需要微调时只用 units；各 mapper 再用 Relative* 区分面/线。
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
    vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(0.0, 0.0);
    vtkMapper::SetResolveCoincidentTopologyLineOffsetParameters(0.0, 0.0);

    // 默认关闭 Depth Peeling：开启时 OpenGL 线轮廓极易穿透不透明实体。
    // 仅在幽灵/半透明特征预览时再临时打开（见 setFeatureOperationGhostMode）。
    // Depth peeling 与 MSAA 互斥；勿对主场景开 FXAA——半透明轮廓会被磨成波浪锯齿。
    vtkWidget->renderWindow()->SetAlphaBitPlanes(1);
    vtkWidget->renderWindow()->SetMultiSamples(0);
    renderer->SetUseDepthPeeling(0);
    renderer->SetUseFXAA(false);

    // 设置渐变背景（冷灰→浅灰，衬托实体高光）
    renderer->SetBackground(0.72, 0.75, 0.79);     // 底部
    renderer->SetBackground2(0.90, 0.92, 0.94);    // 顶部
    renderer->GradientBackgroundOn();           // 启用渐变背景
    renderer->ResetCamera();
    vtkWidget->renderWindow()->Render();

    // 初始化 VIS ShapePicker
    shapePicker = vtkSmartPointer<IVtkTools_ShapePicker>::New();
    shapePicker->SetTolerance(0.05);  // 增加容差以提高拾取精度（不起作用）
    shapePicker->SetRenderer(renderer);
    g_mainVtkWidgetMap[this] = vtkWidget;
    g_mainRendererMap[this] = renderer;
    g_mainPickerMap[this] = shapePicker;

    // 设置自定义交互器（统一用 m_interactorStyle，避免多实例导致滚轮缩放不同步）
    m_interactorStyle = vtkSmartPointer<MouseInteractorStyle>::New();
    m_interactorStyle->SetInteractionContext(createMouseInteractionContext());
    m_interactorStyle->SetPreEventHook([this]() { activateMainRenderContext(); });
    vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);

    // 添加基准坐标系
    setupCoordinateAxes();

    // 添加屏幕中心矢量选择器（可点击X/Y/Z）
    setupCenterAxisSelector();

    // 初始时同步一次相机，使中心轴方向与当前视图一致
    syncCenterAxisCamera();

    // 初始渲染
    vtkWidget->renderWindow()->Render();

}

MouseInteractionContext Widget::createMouseInteractionContext()
{
    MouseInteractionContext mouseContext;
    mouseContext.renderer = [this]() { return renderer.GetPointer(); };
    mouseContext.selectionMode = [this]() { return currentSelectionMode; };
    mouseContext.isInSelectionMode = [this]() { return isInSelectionMode(); };
    mouseContext.hasExtrusionHandleContext = [this]() { return extrusionDialog != nullptr; };
    mouseContext.hasRevolveHandleContext = [this]() { return revolveDialog != nullptr; };
    mouseContext.hasChamferHandleContext = [this]() {
        return chamferDialog != nullptr && chamferAsymHandleSide1Actor_ != nullptr;
    };
    mouseContext.hasFilletHandleContext = [this]() {
        return filletDialog != nullptr && filletRadiusHandleSide1Actor_ != nullptr;
    };
    mouseContext.isVectorTwoPointDialogActive = [this]() {
        return isVectorTwoPointDialogActive();
    };
    mouseContext.isCuboidInteractivePointSelectionActive = [this]() {
        return cuboidDialog != nullptr && cuboidInteractiveActive_;
    };
    mouseContext.isCuboidDragActive = [this]() { return cuboidDragActive_; };
    mouseContext.snapArmed = [this]() { return snap_.armed; };

    mouseContext.syncCenterAxisCamera = [this]() { syncCenterAxisCamera(); };
    mouseContext.stopViewTransitionAnimation = [this]() { stopViewTransitionAnimation(); };
    mouseContext.refreshCameraClippingRange = [this]() { refreshCameraClippingRange(); };
    mouseContext.refreshOverlayScreenScale = [this]() { refreshOverlayScreenScale(); };
    mouseContext.updateCenterTriadHover = [this](int x, int y) { updateCenterTriadHover(x, y); };
    mouseContext.clearModelHoverHighlight = [this]() { clearModelHoverHighlight(); };
    mouseContext.handleVtkMouseClick = [this](int x, int y) { handleVtkMouseClick(x, y); };
    mouseContext.handleVtkMouseMove = [this](int x, int y) { handleVtkMouseMove(x, y); };
    mouseContext.handleExtrusionHandleMouseDown =
        [this](int x, int y) { handleExtrusionHandleMouseDown(x, y); };
    mouseContext.handleExtrusionHandleMouseUp =
        [this](int x, int y) { handleExtrusionHandleMouseUp(x, y); };
    mouseContext.handleRevolveHandleMouseDown =
        [this](int x, int y) { handleRevolveHandleMouseDown(x, y); };
    mouseContext.handleRevolveHandleMouseUp =
        [this](int x, int y) { handleRevolveHandleMouseUp(x, y); };
    mouseContext.handleChamferAsymHandleMouseDown =
        [this](int x, int y) { handleChamferAsymHandleMouseDown(x, y); };
    mouseContext.handleChamferAsymHandleMouseUp =
        [this](int x, int y) { handleChamferAsymHandleMouseUp(x, y); };
    mouseContext.handleFilletRadiusHandleMouseDown =
        [this](int x, int y) { handleFilletRadiusHandleMouseDown(x, y); };
    mouseContext.handleFilletRadiusHandleMouseUp =
        [this](int x, int y) { handleFilletRadiusHandleMouseUp(x, y); };
    mouseContext.handleVectorTwoPointHandleMouseDown =
        [this](int x, int y) { handleVectorTwoPointHandleMouseDown(x, y); };
    mouseContext.handleVectorTwoPointHandleMouseUp =
        [this](int x, int y) { handleVectorTwoPointHandleMouseUp(x, y); };
    mouseContext.handleSketchConicDragMouseUp =
        [this](int x, int y) { handleSketchConicDragMouseUp(x, y); };
    mouseContext.handleSketchEllipseAdjustMouseUp =
        [this](int x, int y) { handleSketchEllipseAdjustMouseUp(x, y); };
    mouseContext.handleCuboidInteractiveMouseDown =
        [this](int x, int y) { handleCuboidInteractiveMouseDown(x, y); };
    mouseContext.handleCuboidInteractiveMouseUp =
        [this](int x, int y) { handleCuboidInteractiveMouseUp(x, y); };
    mouseContext.handlePatternPitchMouseDown =
        [this](int x, int y) { handlePatternPitchMouseDown(x, y); };
    mouseContext.handlePatternPitchMouseUp =
        [this](int x, int y) { handlePatternPitchMouseUp(x, y); };
    mouseContext.beginSketchBrushStroke = [this]() { beginSketchBrushStroke(); };
    mouseContext.endSketchBrushStroke = [this]() { endSketchBrushStroke(); };
    mouseContext.handleVectorTwoPointArrowDoubleClick =
        [this](int x, int y) { handleVectorTwoPointArrowDoubleClick(x, y); };
    mouseContext.pickModelAtPosition = [this](int x, int y) { return pickModelAtPosition(x, y); };
    mouseContext.showContextMenu =
        [this](int x, int y, int modelIndex, const QPoint& globalPos) {
            showContextMenu(x, y, modelIndex, globalPos);
        };
    return mouseContext;
}

void Widget::configureReferenceOverlayAlwaysOnTop()
{
    renderPipeline_.configureReferenceAlwaysOnTop();
}

void Widget::configureFeatureSelectionOverlayAlwaysOnTop()
{
    renderPipeline_.configureAppearanceAlwaysOnTop();
}

vtkRenderer* Widget::appearanceOverlay()
{
    return renderPipeline_.appearanceOverlay();
}

vtkRenderer* Widget::referenceOverlay()
{
    return renderPipeline_.referenceOverlay();
}

void Widget::addAppearanceActor(vtkProp* prop)
{
    renderPipeline_.addAppearanceProp(prop);
}

void Widget::addReferenceActor(vtkProp* prop)
{
    renderPipeline_.addReferenceProp(prop);
}

void Widget::removeSceneActor(vtkProp* prop)
{
    renderPipeline_.removeProp(prop);
}

double Widget::overlayWorldScaleAt(double x, double y, double z) const
{
    if (!renderer) return 1.0;
    vtkCamera* cam = renderer->GetActiveCamera();
    if (!cam) return 1.0;

    // 正交投影：屏幕尺寸由 ParallelScale 决定，与点深度无关
    if (cam->GetParallelProjection()) {
        const double ps = cam->GetParallelScale();
        if (ps < 1e-9 || overlayScaleRefParallel_ < 1e-9) return 1.0;
        return ps / overlayScaleRefParallel_;
    }

    // 透视：用视线方向上的深度（非欧氏距离），保证任意位置叠加物像素大小恒定
    double camPos[3] = {0, 0, 0};
    double fp[3] = {0, 0, 0};
    cam->GetPosition(camPos);
    cam->GetFocalPoint(fp);
    double vpn[3] = {fp[0] - camPos[0], fp[1] - camPos[1], fp[2] - camPos[2]};
    const double vpnLen = std::sqrt(vpn[0] * vpn[0] + vpn[1] * vpn[1] + vpn[2] * vpn[2]);
    if (vpnLen < 1e-12 || overlayScaleRefDistance_ < 1e-9) return 1.0;
    vpn[0] /= vpnLen;
    vpn[1] /= vpnLen;
    vpn[2] /= vpnLen;

    const double depth = (x - camPos[0]) * vpn[0]
                       + (y - camPos[1]) * vpn[1]
                       + (z - camPos[2]) * vpn[2];
    // 点在相机后方时退回焦点距离，避免负缩放
    const double d = (depth > 1e-6) ? depth : vpnLen;
    return d / overlayScaleRefDistance_;
}

double Widget::overlayWorldScale() const
{
    if (!renderer) return 1.0;
    vtkCamera* cam = renderer->GetActiveCamera();
    if (!cam) return 1.0;
    double fp[3] = {0, 0, 0};
    cam->GetFocalPoint(fp);
    return overlayWorldScaleAt(fp[0], fp[1], fp[2]);
}

void Widget::refreshOverlayScreenScale()
{
    // 滚轮/旋转后先同步覆盖层相机位姿，再按主相机深度算手柄屏幕尺寸
    syncOverlayCameras();

    auto applySphereScale = [this](vtkActor* actor, vtkFollower* text,
                                   double textScale, double textOffset) {
        if (!actor) return;
        double p[3] = {0, 0, 0};
        actor->GetPosition(p);
        const double s = overlayWorldScaleAt(p[0], p[1], p[2]);
        actor->SetScale(s, s, s);
        if (text) {
            text->SetPosition(p[0] + textOffset * s,
                              p[1] + textOffset * s,
                              p[2] + textOffset * s);
            text->SetScale(textScale * s, textScale * s, textScale * s);
        }
    };

    applySphereScale(selectedPointActor, selectedTextActor, 0.06, 0.15);
    applySphereScale(hoverPointActor, hoverTextActor, 0.05, 0.10);
    applySphereScale(snapHoverPointActor_, snapHoverTextActor_, 0.05, 0.10);

    for (auto& a : snapPersistentPointActors_) {
        if (!a) continue;
        double p[3] = {0, 0, 0};
        a->GetPosition(p);
        const double s = overlayWorldScaleAt(p[0], p[1], p[2]);
        a->SetScale(s, s, s);
    }

    if (vectorDialogArrowActor_ && vectorDialogArrowActor_->GetVisibility()
        && hasVectorDialogArrowOrigin_) {
        gp_Dir d = hasCustomVectorDir_ ? customVectorDir_ : gp_Dir(1, 0, 0);
        updateVectorDialogArrow(d, vectorDialogArrowOrigin_);
    }

    refreshReferenceCsysScreenScale();

    if (extrusionDialog) updateExtrusionHandles();
    if (revolveDialog) updateRevolveHandles();
    if (cuboidInteractiveActive_) {
        updateCuboidInteractivePreview();
    }

}

void Widget::setupInteractor()
{
    if (vtkWidget && vtkWidget->renderWindow() && vtkWidget->renderWindow()->GetInteractor()) {
        vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
    }
}

namespace {

double lerpScalar(double a, double b, double t)
{
    return a + (b - a) * t;
}

void lerp3(const double a[3], const double b[3], double t, double out[3])
{
    out[0] = lerpScalar(a[0], b[0], t);
    out[1] = lerpScalar(a[1], b[1], t);
    out[2] = lerpScalar(a[2], b[2], t);
}

double vecLen3(const double v[3])
{
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalize3(double v[3])
{
    const double len = vecLen3(v);
    if (len < 1e-12) {
        v[0] = 0.0; v[1] = 1.0; v[2] = 0.0;
        return;
    }
    v[0] /= len; v[1] /= len; v[2] /= len;
}

void cross3(const double a[3], const double b[3], double out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

/** 单位方向球面插值；近反向时绕稳定轴旋转，避免穿模 */
void slerpDir3(const double aIn[3], const double bIn[3], double t, double out[3])
{
    double a[3] = {aIn[0], aIn[1], aIn[2]};
    double b[3] = {bIn[0], bIn[1], bIn[2]};
    normalize3(a);
    normalize3(b);
    double dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    if (dot > 0.9995) {
        lerp3(a, b, t, out);
        normalize3(out);
        return;
    }
    if (dot < -0.9995) {
        double axis[3];
        const double yAxis[3] = {0.0, 1.0, 0.0};
        const double xAxis[3] = {1.0, 0.0, 0.0};
        cross3(a, yAxis, axis);
        if (vecLen3(axis) < 1e-8) {
            cross3(a, xAxis, axis);
        }
        normalize3(axis);
        const double angle = 3.14159265358979323846 * t;
        const double c = std::cos(angle);
        const double s = std::sin(angle);
        // Rodrigues: a*c + (axis×a)*s + axis*(axis·a)*(1-c)
        double axa[3];
        cross3(axis, a, axa);
        const double ada = axis[0] * a[0] + axis[1] * a[1] + axis[2] * a[2];
        out[0] = a[0] * c + axa[0] * s + axis[0] * ada * (1.0 - c);
        out[1] = a[1] * c + axa[1] * s + axis[1] * ada * (1.0 - c);
        out[2] = a[2] * c + axa[2] * s + axis[2] * ada * (1.0 - c);
        normalize3(out);
        return;
    }
    if (dot < -1.0) dot = -1.0;
    if (dot > 1.0) dot = 1.0;
    const double theta = std::acos(dot);
    const double sinTheta = std::sin(theta);
    const double w0 = std::sin((1.0 - t) * theta) / sinTheta;
    const double w1 = std::sin(t * theta) / sinTheta;
    out[0] = a[0] * w0 + b[0] * w1;
    out[1] = a[1] * w0 + b[1] * w1;
    out[2] = a[2] * w0 + b[2] * w1;
    normalize3(out);
}

double easeInOutCubic(double t)
{
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    return (t < 0.5) ? (4.0 * t * t * t)
                     : (1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0);
}

} // namespace

void Widget::captureCameraPose(vtkCamera* camera, ViewCameraPose& out) const
{
    if (!camera) return;
    camera->GetPosition(out.pos);
    camera->GetFocalPoint(out.fp);
    camera->GetViewUp(out.up);
    out.parallel = camera->GetParallelProjection() != 0;
    out.parallelScale = camera->GetParallelScale();
}

void Widget::applyCameraPose(vtkCamera* camera, const ViewCameraPose& pose) const
{
    if (!camera) return;
    camera->SetPosition(pose.pos);
    camera->SetFocalPoint(pose.fp);
    camera->SetViewUp(pose.up);
    camera->OrthogonalizeViewUp();
    if (pose.parallel) {
        camera->ParallelProjectionOn();
        camera->SetParallelScale(std::max(1e-9, pose.parallelScale));
    }
}

void Widget::stopViewTransitionAnimation()
{
    viewTransitionAnimating_ = false;
    if (viewAnimTimer_) {
        viewAnimTimer_->stop();
    }
}

void Widget::beginViewTransitionAnimation(const ViewCameraPose& from, const ViewCameraPose& to)
{
    viewAnimFrom_ = from;
    viewAnimTo_ = to;
    viewTransitionAnimating_ = true;

    if (!viewAnimTimer_) {
        viewAnimTimer_ = new QTimer(this);
        viewAnimTimer_->setInterval(16);
        connect(viewAnimTimer_, &QTimer::timeout, this, &Widget::onViewTransitionAnimTick);
    }
    viewAnimClock_.restart();
    viewAnimTimer_->start();
    onViewTransitionAnimTick();
}

void Widget::onViewTransitionAnimTick()
{
    if (!renderer || !vtkWidget) {
        stopViewTransitionAnimation();
        return;
    }
    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) {
        stopViewTransitionAnimation();
        return;
    }

    const double rawT = static_cast<double>(viewAnimClock_.elapsed())
        / static_cast<double>(kViewAnimDurationMs_);
    const bool finished = rawT >= 1.0;
    const double t = easeInOutCubic(finished ? 1.0 : rawT);

    ViewCameraPose pose;
    lerp3(viewAnimFrom_.fp, viewAnimTo_.fp, t, pose.fp);

    double fromOff[3] = {
        viewAnimFrom_.pos[0] - viewAnimFrom_.fp[0],
        viewAnimFrom_.pos[1] - viewAnimFrom_.fp[1],
        viewAnimFrom_.pos[2] - viewAnimFrom_.fp[2]
    };
    double toOff[3] = {
        viewAnimTo_.pos[0] - viewAnimTo_.fp[0],
        viewAnimTo_.pos[1] - viewAnimTo_.fp[1],
        viewAnimTo_.pos[2] - viewAnimTo_.fp[2]
    };
    const double fromDist = std::max(1e-9, vecLen3(fromOff));
    const double toDist = std::max(1e-9, vecLen3(toOff));
    double fromDir[3] = {fromOff[0] / fromDist, fromOff[1] / fromDist, fromOff[2] / fromDist};
    double toDir[3] = {toOff[0] / toDist, toOff[1] / toDist, toOff[2] / toDist};
    double dir[3];
    slerpDir3(fromDir, toDir, t, dir);
    const double dist = lerpScalar(fromDist, toDist, t);
    pose.pos[0] = pose.fp[0] + dir[0] * dist;
    pose.pos[1] = pose.fp[1] + dir[1] * dist;
    pose.pos[2] = pose.fp[2] + dir[2] * dist;

    lerp3(viewAnimFrom_.up, viewAnimTo_.up, t, pose.up);
    normalize3(pose.up);
    pose.parallel = viewAnimTo_.parallel;
    pose.parallelScale = lerpScalar(viewAnimFrom_.parallelScale, viewAnimTo_.parallelScale, t);

    applyCameraPose(camera, finished ? viewAnimTo_ : pose);
    refreshCameraClippingRange();
    syncCenterAxisCamera();
    if (finished) {
        stopViewTransitionAnimation(); // 先结束，使 refreshOverlay 能重建轮廓
    }
    refreshOverlayScreenScale();

    if (vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::animateCameraToPose(const std::function<void()>& applyTargetImmediate)
{
    if (!renderer || !vtkWidget || !applyTargetImmediate) return;
    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) return;

    stopViewTransitionAnimation();

    ViewCameraPose from;
    captureCameraPose(camera, from);

    applyTargetImmediate();
    ViewCameraPose to;
    captureCameraPose(camera, to);

    // 先恢复起点，再过渡，避免闪一帧目标姿态
    applyCameraPose(camera, from);

    auto near3 = [](const double u[3], const double v[3], double eps) {
        return std::abs(u[0] - v[0]) < eps
            && std::abs(u[1] - v[1]) < eps
            && std::abs(u[2] - v[2]) < eps;
    };
    const bool same = near3(from.pos, to.pos, 1e-5)
        && near3(from.fp, to.fp, 1e-5)
        && near3(from.up, to.up, 1e-4)
        && std::abs(from.parallelScale - to.parallelScale) < 1e-5
        && from.parallel == to.parallel;

    if (same) {
        applyCameraPose(camera, to);
        refreshCameraClippingRange();
        syncCenterAxisCamera();
        refreshOverlayScreenScale();
        if (vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        return;
    }

    beginViewTransitionAnimation(from, to);
}

void Widget::applyStandardViewCameraImmediate(const QString& normalizedViewName)
{
    if (!renderer) return;
    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) return;

    const QString& vn = normalizedViewName;

    double bounds[6];
    bool hasBounds = false;
    if (historyList.size() > 0) {
        double minX = 1e10, maxX = -1e10;
        double minY = 1e10, maxY = -1e10;
        double minZ = 1e10, maxZ = -1e10;

        for (const auto& record : historyList) {
            if (renderStateFor(record).actor && renderStateFor(record).actor->GetVisibility()) {
                double actorBounds[6];
                renderStateFor(record).actor->GetBounds(actorBounds);
                minX = qMin(minX, actorBounds[0]);
                maxX = qMax(maxX, actorBounds[1]);
                minY = qMin(minY, actorBounds[2]);
                maxY = qMax(maxY, actorBounds[3]);
                minZ = qMin(minZ, actorBounds[4]);
                maxZ = qMax(maxZ, actorBounds[5]);
                hasBounds = true;
            }
        }

        if (hasBounds) {
            bounds[0] = minX; bounds[1] = maxX;
            bounds[2] = minY; bounds[3] = maxY;
            bounds[4] = minZ; bounds[5] = maxZ;
        }
    }

    if (!hasBounds) {
        bounds[0] = -1.0; bounds[1] = 1.0;
        bounds[2] = -1.0; bounds[3] = 1.0;
        bounds[4] = -1.0; bounds[5] = 1.0;
    }

    const double centerX = (bounds[0] + bounds[1]) / 2.0;
    const double centerY = (bounds[2] + bounds[3]) / 2.0;
    const double centerZ = (bounds[4] + bounds[5]) / 2.0;
    const double sizeX = bounds[1] - bounds[0];
    const double sizeY = bounds[3] - bounds[2];
    const double sizeZ = bounds[5] - bounds[4];
    const double maxSize = qMax(qMax(sizeX, sizeY), sizeZ);
    const double distance = maxSize * 2.0;

    if (vn == "前视图") {
        camera->SetPosition(centerX, centerY, centerZ + distance);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 1, 0);
    } else if (vn == "俯视图") {
        camera->SetPosition(centerX, centerY + distance, centerZ);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 0, -1);
    } else if (vn == "仰视图") {
        camera->SetPosition(centerX, centerY - distance, centerZ);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 0, 1);
    } else if (vn == "左视图") {
        camera->SetPosition(centerX - distance, centerY, centerZ);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 0, 1);
    } else if (vn == "右视图") {
        camera->SetPosition(centerX + distance, centerY, centerZ);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 0, 1);
    } else if (vn == "后视图") {
        camera->SetPosition(centerX, centerY, centerZ - distance);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(0, 1, 0);
    } else if (vn == "正三轴测视图") {
        const double isoDistance = distance * 1.2;
        const double sqrt3 = std::sqrt(3.0);
        camera->SetPosition(centerX + isoDistance / sqrt3,
                            centerY + isoDistance / sqrt3,
                            centerZ + isoDistance / sqrt3);
        camera->SetFocalPoint(centerX, centerY, centerZ);
        camera->SetViewUp(-1, 1, 0);
        camera->OrthogonalizeViewUp();
    } else {
        return;
    }

    renderer->ResetCamera();
    refreshCameraClippingRange();
}

void Widget::switchToView(const QString& viewName)
{
    if (!renderer || !vtkWidget) return;

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) return;

    QString vn = viewName.trimmed();
    if (vn == "正视图") vn = "前视图";
    if (vn == "正三轴视图" || vn == "正三轴侧视图") vn = "正三轴测视图";
    if (vn == "正等测图") vn = "正三轴测视图";

    const bool known = (vn == "前视图" || vn == "俯视图" || vn == "仰视图"
                        || vn == "左视图" || vn == "右视图" || vn == "后视图"
                        || vn == "正三轴测视图");
    if (!known) return;

    centerTriadCurrentFace_ = triadFaceFromViewName(vn);
    refreshCenterTriadFaceStyle();

    animateCameraToPose([this, vn]() {
        applyStandardViewCameraImmediate(vn);
    });
}

void Widget::applyInitialSceneView()
{
    if (!renderer || !vtkWidget)
        return;

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera)
        return;

    // 初始/空场景时不用 ResetCamera 贴边放大，固定到较远的正等轴测视角
    constexpr double kInitialDistance = 14.0;
    const double sqrt3 = std::sqrt(3.0);
    const double iso = kInitialDistance / sqrt3;
    camera->SetPosition(iso, iso, iso);
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetViewUp(-1.0, 1.0, 0.0);
    camera->OrthogonalizeViewUp();

    // 参考视距与初始视角对齐，使默认缩放下叠加物系数为 1
    overlayScaleRefDistance_ = kInitialDistance;
    if (camera->GetParallelProjection()) {
        overlayScaleRefParallel_ = std::max(1e-6, camera->GetParallelScale());
    }

    centerTriadCurrentFace_ = triadFaceFromViewName(QStringLiteral("正三轴测视图"));
    refreshCenterTriadFaceStyle();

    refreshCameraClippingRange();
    syncCenterAxisCamera();
    refreshOverlayScreenScale();

    if (vtkWidget->renderWindow())
        vtkWidget->renderWindow()->Render();
}

void Widget::syncOverlayCameras()
{
    renderPipeline_.syncCameras();
}

void Widget::refreshCameraClippingRange()
{
    if (!renderer)
        return;

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera)
        return;

    // 主场景只按模型紧裁切。长预览/手柄在覆盖层独立相机上处理，
    // 绝不能并进主 near/far，否则幽灵半透明面会出现波浪状内部锯齿。
    renderer->ResetCameraClippingRange();

    if (featureOperationGhostMode_) {
        double cr[2] = {0, 0};
        camera->GetClippingRange(cr);
        const double span = std::max(1e-6, cr[1] - cr[0]);
        const double nearPlane = std::max(1e-4, cr[0] - 0.05 * span);
        const double farPlane = cr[1] + 0.25 * span;
        camera->SetClippingRange(nearPlane, farPlane);
    }

    syncOverlayCameras();
}

void Widget::alignViewToSketchPlane(const gp_Pln& plane)
{
    if (!renderer || !vtkWidget)
        return;

    animateCameraToPose([this, plane]() {
        vtkCamera* camera = renderer->GetActiveCamera();
        if (!camera)
            return;

        const gp_Ax3 ax = plane.Position();
        const gp_Pnt origin = ax.Location();
        const gp_Dir normal = ax.Direction();
        const gp_Dir xDir = ax.XDirection();

        double bounds[6];
        bool hasBounds = false;
        if (historyList.size() > 0) {
            double minX = 1e10, maxX = -1e10;
            double minY = 1e10, maxY = -1e10;
            double minZ = 1e10, maxZ = -1e10;

            for (const auto& record : historyList) {
                if (renderStateFor(record).actor && renderStateFor(record).actor->GetVisibility()) {
                    double actorBounds[6];
                    renderStateFor(record).actor->GetBounds(actorBounds);
                    minX = qMin(minX, actorBounds[0]);
                    maxX = qMax(maxX, actorBounds[1]);
                    minY = qMin(minY, actorBounds[2]);
                    maxY = qMax(maxY, actorBounds[3]);
                    minZ = qMin(minZ, actorBounds[4]);
                    maxZ = qMax(maxZ, actorBounds[5]);
                    hasBounds = true;
                }
            }

            if (hasBounds) {
                bounds[0] = minX; bounds[1] = maxX;
                bounds[2] = minY; bounds[3] = maxY;
                bounds[4] = minZ; bounds[5] = maxZ;
            }
        }

        if (!hasBounds) {
            bounds[0] = -1.0; bounds[1] = 1.0;
            bounds[2] = -1.0; bounds[3] = 1.0;
            bounds[4] = -1.0; bounds[5] = 1.0;
        }

        const double sizeX = bounds[1] - bounds[0];
        const double sizeY = bounds[3] - bounds[2];
        const double sizeZ = bounds[5] - bounds[4];
        const double maxSize = qMax(qMax(sizeX, sizeY), sizeZ);
        double distance = maxSize * 2.0;
        if (distance < 1.0)
            distance = 10.0;

        const gp_Vec nVec(normal);
        const gp_Pnt camPos = origin.Translated(nVec.Multiplied(distance));

        camera->SetPosition(camPos.X(), camPos.Y(), camPos.Z());
        camera->SetFocalPoint(origin.X(), origin.Y(), origin.Z());
        camera->SetViewUp(xDir.X(), xDir.Y(), xDir.Z());
        refreshCameraClippingRange();
    });
}

// 右键/左键共用：稳健拾取 history 实体
// 注意：禁止大半径屏幕“邻近吸附”——那会造成“点上方空白却选中模型”的假偏移感。
// 以渲染可见性（PropPicker）+ 网格（CellPicker）为准，OCC 射线与小半径棱角辅助为辅。
int Widget::pickHistoryModelFallback(int x, int y) const
{
    if (!renderer) return -1;

    auto isSelectable = [](ModelType t) {
        return t != DATUM_PLANE && t != DATUM_AXIS && t != WORK_CSYS && t != REFERENCE_CSYS;
    };
    auto isSolidBody = [&](ModelType t) {
        return isSelectable(t) && t != SKETCH;
    };

    QVector<vtkActor*> actors;
    QVector<int> pickableRestore;
    for (int i = 0; i < historyList.size(); ++i) {
        const ModelingHistory& rec = historyList[i];
        if (!renderStateFor(rec).actor || renderStateFor(rec).actor->GetVisibility() == 0) continue;
        if (!isSolidBody(rec.type)) continue;
        actors.append(renderStateFor(rec).actor);
        pickableRestore.append(renderStateFor(rec).actor->GetPickable());
        renderStateFor(rec).actor->SetPickable(1);
    }

    auto restorePickable = [&]() {
        for (int i = 0; i < actors.size(); ++i) {
            actors[i]->SetPickable(pickableRestore[i]);
        }
    };

    // 像素螺旋：抵消轻微亚像素/DPI 误差，但绝不超过数像素（避免假偏移）
    static const int kSpiral[13][2] = {
        {0, 0},
        {2, 0}, {-2, 0}, {0, 2}, {0, -2},
        {4, 0}, {-4, 0}, {0, 4}, {0, -4},
        {3, 3}, {-3, 3}, {3, -3}, {-3, -3}
    };

    // 1) CellPicker：软件网格拾取（Depth Peeling 下比 PropPicker 更稳）
    {
        vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
        // Tolerance 是窗口对角线比例；0.05≈5% 过宽会导致“点偏也能中 / 命中发飘”
        cellPicker->SetTolerance(0.008);
        cellPicker->PickFromListOn();
        for (vtkActor* a : actors) cellPicker->AddPickList(a);
        for (const auto& o : kSpiral) {
            if (cellPicker->Pick(static_cast<double>(x + o[0]), static_cast<double>(y + o[1]), 0.0, renderer)) {
                const int hit = resolveHistoryIndexByActor(cellPicker->GetActor());
                if (hit >= 0) {
                    restorePickable();
                    return hit;
                }
            }
        }
    }

    // 2) PropPicker：按当前帧可见像素拾取
    {
        vtkSmartPointer<vtkPropPicker> propPicker = vtkSmartPointer<vtkPropPicker>::New();
        propPicker->PickFromListOn();
        for (vtkActor* a : actors) propPicker->AddPickList(a);
        for (const auto& o : kSpiral) {
            if (propPicker->Pick(static_cast<double>(x + o[0]), static_cast<double>(y + o[1]), 0.0, renderer)) {
                const int hit = resolveHistoryIndexByActor(propPicker->GetActor());
                if (hit >= 0) {
                    restorePickable();
                    return hit;
                }
            }
        }
    }

    restorePickable();

    // 3) OCC 射线-面求交（几何精确；擦边时可能失败，故放后面）
    {
        gp_Lin pickRay;
        bool hasRay = false;
        double worldNear[4] = {0, 0, 0, 1};
        double worldFar[4] = {0, 0, 1, 1};
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
            pickRay = gp_Lin(p0, gp_Dir(v));
            hasRay = true;
        }

        if (hasRay) {
            Standard_Real bestW = RealLast();
            int bestIdx = -1;
            for (int i = 0; i < historyList.size(); ++i) {
                const ModelingHistory& rec = historyList[i];
                if (!renderStateFor(rec).actor || renderStateFor(rec).actor->GetVisibility() == 0) continue;
                if (!isSolidBody(rec.type)) continue;
                if (geometryStateFor(rec).occShape.IsNull()) continue;
                try {
                    IntCurvesFace_ShapeIntersector intersector;
                    intersector.Load(geometryStateFor(rec).occShape, 1.0e-4);
                    intersector.Perform(pickRay, -1.0e-3, 1.0e9);
                    for (int k = 1; k <= intersector.NbPnt(); ++k) {
                        const Standard_Real w = intersector.WParameter(k);
                        if (w >= -1.0e-3 && w < bestW) {
                            bestW = w;
                            bestIdx = i;
                        }
                    }
                } catch (...) {
                    continue;
                }
            }
            if (bestIdx >= 0) return bestIdx;
        }
    }

    // 4) 仅棱角小半径辅助（≤12px）。禁止 110px 大吸附，否则点空白也会“选中”。
    {
        constexpr double kCornerPx = 12.0;
        const double maxD2 = kCornerPx * kCornerPx;
        double bestD2 = maxD2;
        int bestIdx = -1;

        auto consider = [&](const gp_Pnt& p, int idx) {
            renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
            renderer->WorldToDisplay();
            double d[3];
            renderer->GetDisplayPoint(d);
            // 丢弃在相机后方/裁剪外的投影点
            if (d[2] < 0.0 || d[2] > 1.0) return;
            const double dx = d[0] - static_cast<double>(x);
            const double dy = d[1] - static_cast<double>(y);
            const double d2 = dx * dx + dy * dy;
            if (d2 < bestD2) {
                bestD2 = d2;
                bestIdx = idx;
            }
        };

        for (int i = 0; i < historyList.size(); ++i) {
            const ModelingHistory& rec = historyList[i];
            if (!renderStateFor(rec).actor || renderStateFor(rec).actor->GetVisibility() == 0) continue;
            if (!isSolidBody(rec.type)) continue;
            if (geometryStateFor(rec).occShape.IsNull()) continue;

            for (TopExp_Explorer ex(geometryStateFor(rec).occShape, TopAbs_VERTEX); ex.More(); ex.Next()) {
                consider(BRep_Tool::Pnt(TopoDS::Vertex(ex.Current())), i);
            }
            int edgeBudget = 0;
            for (TopExp_Explorer exE(geometryStateFor(rec).occShape, TopAbs_EDGE); exE.More(); exE.Next()) {
                if (++edgeBudget > 400) break;
                const TopoDS_Edge edge = TopoDS::Edge(exE.Current());
                const QList<gp_Pnt> points = SketchGeometry::sampleEdgePoints(edge, 5);
                for (const gp_Pnt& p : points) {
                    consider(p, i);
                }
            }
        }
        if (bestIdx >= 0) return bestIdx;
    }

    return -1;
}

int Widget::pickHistoryModelStrict(int x, int y) const
{
    // 左键高亮：只认屏幕上真正画出来的像素（PropPicker）。
    // CellPicker 容差 / OCC 射线会在轮廓外形成“隐形偏大外壳”。
    if (!renderer) return -1;

    auto isSolidBody = [](ModelType t) {
        return t != DATUM_PLANE && t != DATUM_AXIS && t != WORK_CSYS
            && t != REFERENCE_CSYS && t != SKETCH;
    };

    QVector<vtkActor*> actors;
    QVector<int> pickableRestore;
    for (int i = 0; i < historyList.size(); ++i) {
        const ModelingHistory& rec = historyList[i];
        if (!renderStateFor(rec).actor || renderStateFor(rec).actor->GetVisibility() == 0) continue;
        if (!isSolidBody(rec.type)) continue;
        actors.append(renderStateFor(rec).actor);
        pickableRestore.append(renderStateFor(rec).actor->GetPickable());
        renderStateFor(rec).actor->SetPickable(1);
    }

    auto restorePickable = [&]() {
        for (int i = 0; i < actors.size(); ++i)
            actors[i]->SetPickable(pickableRestore[i]);
    };

    vtkSmartPointer<vtkPropPicker> propPicker = vtkSmartPointer<vtkPropPicker>::New();
    propPicker->PickFromListOn();
    for (vtkActor* a : actors)
        propPicker->AddPickList(a);

    int hit = -1;
    if (propPicker->Pick(static_cast<double>(x), static_cast<double>(y), 0.0, renderer))
        hit = resolveHistoryIndexByActor(propPicker->GetActor());

    restorePickable();
    return hit;
}

int Widget::pickModelAtPosition(int x, int y)
{
    // 优先用稳健回退：彻底解决“能拾点不能高亮”
    const int robust = pickHistoryModelFallback(x, y);
    if (robust >= 0) return robust;

    if (!shapePicker) return -1;

    refreshShapePickerBindingsForCurrentContext(0.1);

    auto pickFirstHistoryIndex = [&]() -> int {
        vtkSmartPointer<vtkActorCollection> actors = shapePicker->GetPickedActors(true);
        if (!actors || actors->GetNumberOfItems() == 0) return -1;
        actors->InitTraversal();
        while (vtkActor* actor = actors->GetNextActor()) {
            if (!actor || actor->GetVisibility() == 0) continue;
            const int idx = resolveHistoryIndexByActor(actor);
            if (idx >= 0) return idx;
        }
        return -1;
    };

    shapePicker->SetSelectionMode(SM_Face);
    shapePicker->Pick(x, y, 0);
    int idx = pickFirstHistoryIndex();
    if (idx >= 0) return idx;

    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);
    idx = pickFirstHistoryIndex();
    if (idx >= 0) return idx;

    shapePicker->SetSelectionMode(SM_Vertex);
    shapePicker->Pick(x, y, 0);
    return pickFirstHistoryIndex();
}
