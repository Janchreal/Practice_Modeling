#include "render_pipeline.h"

#include <algorithm>
#include <cstdint>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLState.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif

namespace {

void configureOverlayDefaults(vtkRenderer* overlay, int layer)
{
    if (!overlay) {
        return;
    }

    overlay->SetLayer(layer);
    overlay->SetViewport(0.0, 0.0, 1.0, 1.0);
    overlay->InteractiveOff();
    overlay->SetUseFXAA(false);
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
    configureLights_ = configureLights;

    if (renderWindow_->GetNumberOfLayers() < 4) {
        renderWindow_->SetNumberOfLayers(4);
    }

    ensureAppearanceOverlay();
    ensureReferenceOverlay();
    return appearanceOverlay_ && referenceOverlay_;
}

vtkRenderer* RenderPipeline::appearanceOverlay()
{
    return ensureAppearanceOverlay();
}

vtkRenderer* RenderPipeline::referenceOverlay()
{
    return ensureReferenceOverlay();
}

vtkRenderer* RenderPipeline::ensureAppearanceOverlay()
{
    if (appearanceOverlay_ || !renderWindow_ || !modelRenderer_) {
        return appearanceOverlay_;
    }

    appearanceOverlay_ = vtkSmartPointer<vtkRenderer>::New();
    configureOverlayDefaults(appearanceOverlay_, 1);
    configureAppearanceAlwaysOnTop();

    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    if (modelRenderer_->GetActiveCamera()) {
        camera->DeepCopy(modelRenderer_->GetActiveCamera());
    }
    appearanceOverlay_->SetActiveCamera(camera);
    if (configureLights_) {
        configureLights_(appearanceOverlay_);
    }
    renderWindow_->AddRenderer(appearanceOverlay_);
    return appearanceOverlay_;
}

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

vtkRenderer* RenderPipeline::configureOverlay(
    vtkRenderer* overlay,
    vtkSmartPointer<vtkCallbackCommand>& observer,
    bool disableDepthTest)
{
    if (!overlay) {
        return nullptr;
    }

    overlay->SetErase(0);
    overlay->SetPreserveColorBuffer(1);
    overlay->SetPreserveDepthBuffer(0);

    if (!observer) {
        observer = vtkSmartPointer<vtkCallbackCommand>::New();
        observer->SetClientData(reinterpret_cast<void*>(static_cast<intptr_t>(disableDepthTest ? 1 : 0)));
        observer->SetCallback(
            [](vtkObject* caller, unsigned long eventId, void* clientData, void*) {
                auto* renderer = vtkRenderer::SafeDownCast(caller);
                if (!renderer) {
                    return;
                }

                auto* renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
                if (!renderWindow || !renderWindow->GetState()) {
                    return;
                }

                vtkOpenGLState* state = renderWindow->GetState();
                const bool disableDepth = reinterpret_cast<intptr_t>(clientData) != 0;
                if (eventId == vtkCommand::StartEvent) {
                    state->vtkglClear(GL_DEPTH_BUFFER_BIT);
                    if (disableDepth) {
                        state->vtkglDisable(GL_DEPTH_TEST);
                    }
                } else if (eventId == vtkCommand::EndEvent && disableDepth) {
                    state->vtkglEnable(GL_DEPTH_TEST);
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

void RenderPipeline::syncCameras()
{
    if (!modelRenderer_ || !modelRenderer_->GetActiveCamera()) {
        return;
    }

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

        camera->DeepCopy(modelCamera);
        overlay->ResetCameraClippingRange();
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
