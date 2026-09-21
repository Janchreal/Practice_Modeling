/*
#include 区
#ifndef 兜底宏（GL_DEPTH_BUFFER_BIT / GL_DEPTH_TEST）
namespace {                                    ← 内部工具函数
    configureOverlayDefaults
}

RenderPipeline::initialize                     ← 初始化
RenderPipeline::appearanceOverlay              ← 获取外观叠加层
RenderPipeline::referenceOverlay               ← 获取参考叠加层
RenderPipeline::ensureAppearanceOverlay        ← 懒创建外观层
RenderPipeline::ensureReferenceOverlay         ← 懒创建参考层
RenderPipeline::configureOverlay               ← 通用叠加层配置 + 观察者
RenderPipeline::configureAppearanceAlwaysOnTop ← 外观层"置顶"策略
RenderPipeline::configureReferenceAlwaysOnTop  ← 参考层"置顶"策略
RenderPipeline::addAppearanceProp              ← 添加 Prop 到外观层
RenderPipeline::addReferenceProp               ← 添加 Prop 到参考层
RenderPipeline::removeProp                     ← 从所有层移除 Prop
RenderPipeline::syncCameras                    ← 相机同步
*/
#include "render_pipeline.h"

#include <algorithm>
#include <cstdint>

#include <vtkCamera.h>//相机：位置、朝向、投影、裁剪面
#include <vtkCommand.h>//事件常量
#include <vtkOpenGLRenderWindow.h>//OpenGL 后端渲染窗口，能拿到 vtkOpenGLState
#include <vtkOpenGLState.h>//直接操作 GL 状态的接口（清缓冲、开关深度测试）
/*
* 为什么需要vtkOpenGLState
* VTK 默认把 OpenGL 状态封装得很好，但要让叠加层"永远置顶"必须手动清深度缓冲、临时关闭深度测试，
* 这些操作必须走底层 GL 接口。vtkOpenGLState 是 VTK 提供的、安全的 GL 状态操作封装
*/
#include <vtkProp.h>//所有可渲染对象的基类（Actor/Volume/Assembly 等的父类）
#include <vtkRenderer.h>//渲染器
#include <vtkRenderWindow.h>//渲染窗口
/*
* 为什么需要这两个宏
* 这两个常量本是OpenGL头文件里的宏
* 但本项目不直接依赖GL头文件通过vtkOpenGLState间接使用他们
* 为了让代码能在没有包含 GL 头文件的环境下编译通过，这里手动定义
* 值都是 OpenGL 官方规范里固定不变的
*/
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif

namespace {
    //这是一个纯粹的"默认值函数"，把叠加层共同点集中在这里，减少重复。
void configureOverlayDefaults(vtkRenderer* overlay, int layer)
{
    if (!overlay) {
        return;
    }

    overlay->SetLayer(layer);//指定该渲染器在窗口中的层号。VTK 按层号从小到大依次绘制，层号越大越靠上层
    overlay->SetViewport(0.0, 0.0, 1.0, 1.0);//视口铺满整个窗口（归一化坐标 [0,1]²）。主模型层通常也是全窗口，所以叠加层与它对齐
    overlay->InteractiveOff();//关闭交互。叠加层不处理鼠标事件，避免抢走主渲染器的拾取/相机操作
    overlay->SetUseFXAA(false);//关闭快速近似抗锯齿。叠加层通常是细线或图标，FXAA反而会糊掉。主模型层打开FXAA即可
}

} // namespace

bool RenderPipeline::initialize(vtkRenderWindow* renderWindow,
                                vtkRenderer* modelRenderer,
                                const SceneLightConfigurer& configureLights)
{
    if (!renderWindow || !modelRenderer) {
        return false;
    }

    renderWindow_ = renderWindow;
    modelRenderer_ = modelRenderer;
    configureLights_ = configureLights;//一个可调用对象（回调），用来给新叠加层配置光照
    //层数保证，vtk默认只分配一层，4的原因是预留给后面的扩展
    if (renderWindow_->GetNumberOfLayers() < 4) {
        renderWindow_->SetNumberOfLayers(4);
    }

    ensureAppearanceOverlay();
    ensureReferenceOverlay();
    return appearanceOverlay_ && referenceOverlay_;
}

//appearanceOverlay/referenceOverlay——访问器，直接转发到ensureXxx
//即使用户没调initialize，只要内部保存了renderWindow_和modelRenderer_，也能按需创建。
vtkRenderer* RenderPipeline::appearanceOverlay()
{
    return ensureAppearanceOverlay();
}

vtkRenderer* RenderPipeline::referenceOverlay()
{
    return ensureReferenceOverlay();
}


//ensureAppearanceOverlay/ensureReferenceOverlay——懒创建
vtkRenderer* RenderPipeline::ensureAppearanceOverlay()
{//1.幂等保护：已创建 → 直接返回，未初始化 → 返回 nullptr，只有"未创建但已初始化"才继续走创建逻辑
    if (appearanceOverlay_ || !renderWindow_ || !modelRenderer_) {
        return appearanceOverlay_;
    }

    appearanceOverlay_ = vtkSmartPointer<vtkRenderer>::New();
    configureOverlayDefaults(appearanceOverlay_, 1);//应用默认配置，层号1。主模型渲染器通常在layer0
    configureAppearanceAlwaysOnTop();//置顶策略
//相机初始化，叠加层必须有独立的相机对象，不能跟主相机共享指针，初始创建时deep copy主相机参数，保证初始视角一致，如果主相机还没有，就先放一个默认相机，后续syncCameras会补齐
    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    if (modelRenderer_->GetActiveCamera()) {
        camera->DeepCopy(modelRenderer_->GetActiveCamera());
    }
    appearanceOverlay_->SetActiveCamera(camera);
    //配置光照，把外部注入的光照策略应用到新渲染器。为什么需要每个渲染器单独配光照？ 因为 VTK 里光源是绑定到渲染器上的，每个渲染器有自己的光源集合
    if (configureLights_) {
        configureLights_(appearanceOverlay_);
    }
    //加入窗口
    renderWindow_->AddRenderer(appearanceOverlay_);
    return appearanceOverlay_;
}

//几乎与ensureAppearanceOverlay对称
vtkRenderer* RenderPipeline::ensureReferenceOverlay()
{
    if (referenceOverlay_ || !renderWindow_ || !modelRenderer_) {
        return referenceOverlay_;
    }

    referenceOverlay_ = vtkSmartPointer<vtkRenderer>::New();
    configureOverlayDefaults(referenceOverlay_, 2);
    configureReferenceAlwaysOnTop();

    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    if (modelRenderer_->GetActiveCamera()) {
        camera->DeepCopy(modelRenderer_->GetActiveCamera());
    }
    referenceOverlay_->SetActiveCamera(camera);
    if (configureLights_) {
        configureLights_(referenceOverlay_);
    }
    renderWindow_->AddRenderer(referenceOverlay_);
    return referenceOverlay_;
}

//核心，通用叠加层配置
vtkRenderer* RenderPipeline::configureOverlay(
    vtkRenderer* overlay,
    vtkSmartPointer<vtkCallbackCommand>& observer,//引用传入，说明它由调用者（appearanceObserver_ / referenceObserver_）持有，防止被 GC
    bool disableDepthTest)
{
    if (!overlay) {
        return nullptr;
    }
    //顺序：主模型层先画，之后外观叠加层画，最后参考叠加层画。
    overlay->SetErase(0);//不清屏（不擦除背景色），保持下面层的渲染结果，叠加层只画自己的东西
    overlay->SetPreserveColorBuffer(1);//保留颜色缓冲，确保叠加层画完后不会擦掉已画内容
    overlay->SetPreserveDepthBuffer(0);//不保留深度缓冲，叠加层绘制时要清掉深度，让所有叠加层对象都画在最前面
    //观察者（Observer）机制，用vtkCallbackCommand挂接渲染的开始/结束事件，VTK 对象间的观察者连接是弱引用，如果只创建一个局部 vtkSmartPointer 就丢掉，观察者会被销毁，回调不再触发。
    if (!observer) {
        observer = vtkSmartPointer<vtkCallbackCommand>::New();
        observer->SetClientData(reinterpret_cast<void*>(static_cast<intptr_t>(disableDepthTest ? 1 : 0)));
        observer->SetCallback(
            [](vtkObject* caller, unsigned long eventId, void* clientData, void*) {
                auto* renderer = vtkRenderer::SafeDownCast(caller);//把vtkObject*安全转成 vtkRenderer*。这里caller就是触发事件的渲染器本身
                if (!renderer) {
                    return;
                }
                //拿渲染窗口，并 SafeDownCast 成 vtkOpenGLRenderWindow（因为只有 OpenGL 后端才有 GetState
                auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
                if (!renderWindow || !renderWindow->GetState()) {
                    return;
                }
                //拿vtkOpenGLState*
                vtkOpenGLState* state = renderWindow->GetState();
                //取出 clientData 里的 bool
                const bool disableDepth = reinterpret_cast<intptr_t>(clientData) != 0;
                //StartEvent（该渲染器即将开始绘制）
                if (eventId == vtkCommand::StartEvent) {
                    state->vtkglClear(GL_DEPTH_BUFFER_BIT);//深度缓冲
                    if (disableDepth) {
                        state->vtkglDisable(GL_DEPTH_TEST);//关闭深度测试，该层内部也不做前后遮挡，后面的对象会盖住前面的对象（按添加顺序）
                    }
                //EndEvent（该渲染器绘制结束）
                } else if (eventId == vtkCommand::EndEvent && disableDepth) {
                    state->vtkglEnable(GL_DEPTH_TEST);//如果刚才关过深度测试，必须重新打开，否则会污染后续渲染器
                }
            });
        overlay->AddObserver(vtkCommand::StartEvent, observer);
        overlay->AddObserver(vtkCommand::EndEvent, observer);
    }

    return overlay;
}

void RenderPipeline::configureAppearanceAlwaysOnTop()
{
    configureOverlay(appearanceOverlay_, appearanceObserver_, false);
}

void RenderPipeline::configureReferenceAlwaysOnTop()
{
    configureOverlay(referenceOverlay_, referenceObserver_, true);
}

void RenderPipeline::addAppearanceProp(vtkProp* prop)
{
    if (!prop || !appearanceOverlay()) {
        return;
    }
    removeProp(prop);
    appearanceOverlay_->AddViewProp(prop);
}

void RenderPipeline::addReferenceProp(vtkProp* prop)
{
    if (!prop || !referenceOverlay()) {
        return;
    }
    removeProp(prop);
    referenceOverlay_->AddViewProp(prop);
}

void RenderPipeline::removeProp(vtkProp* prop)
{
    if (!prop) {
        return;
    }
    if (modelRenderer_) {
        modelRenderer_->RemoveViewProp(prop);
    }
    if (appearanceOverlay_) {
        appearanceOverlay_->RemoveViewProp(prop);
    }
    if (referenceOverlay_) {
        referenceOverlay_->RemoveViewProp(prop);
    }
}
//相机同步
void RenderPipeline::syncCameras()
{
    if (!modelRenderer_ || !modelRenderer_->GetActiveCamera()) {
        return;
    }
    //相机指针保护
    vtkCamera* modelCamera = modelRenderer_->GetActiveCamera();
    auto syncOne = [modelCamera](vtkRenderer* overlay) {
        if (!overlay) {
            return;
        }

        vtkCamera* camera = overlay->GetActiveCamera();
        if (!camera || camera == modelCamera) {
            vtkSmartPointer<vtkCamera> ownCamera = vtkSmartPointer<vtkCamera>::New();
            ownCamera->DeepCopy(modelCamera);
            overlay->SetActiveCamera(ownCamera);
            camera = overlay->GetActiveCamera();
        }
        if (!camera) {
            return;
        }
        //每次同步时深拷贝，把主相机的所有参数（位置、焦点、上下方向、投影角度、投影方式、裁剪面、视口等）复制到叠加层相机。
        camera->DeepCopy(modelCamera);
        overlay->ResetCameraClippingRange();//让 VTK 根据叠加层里现有Prop的包围盒重新计算裁剪面
        /*裁剪面扩展
        * clippingRange[0]=Near,clippingRange[1]=Far
        * span=Far-Near,即当前裁剪范围跨度
        * 新 Near = Near - 0.10 * span（向前扩 10%），同时保证至少为 1e-4（防止接近零导致数值误差/除法问题）
        * 新 Far = Far + 0.50 * span（向后扩 50%）
        * ResetCameraClippingRange 是根据叠加层的几何体计算的，理论上刚好覆盖。但如果叠加层里有动态内容（如 gizmo 手柄在相机附近、字体 tag 在远处），物体在下一帧可能会落出裁剪面，提前扩展给了一个安全边界（safety margin），避免闪烁的物体被"切掉"
        * 为什么 Far 扩得比 Near 多（0.5 vs 0.1）？因为 Far 太近会导致远物体消失，而 Far 拉远通常不会带来明显精度损失（OpenGL 深度缓冲精度主要受 Near/Far 比例影响，Near 才是关键）
        */
        double clippingRange[2] = {0.0, 0.0};
        camera->GetClippingRange(clippingRange);
        const double span = std::max(1e-6, clippingRange[1] - clippingRange[0]);
        camera->SetClippingRange(std::max(1e-4, clippingRange[0] - 0.10 * span),
                                 clippingRange[1] + 0.50 * span);
    };

    syncOne(appearanceOverlay_);
    syncOne(referenceOverlay_);
    configureAppearanceAlwaysOnTop();
    configureReferenceAlwaysOnTop();
}
