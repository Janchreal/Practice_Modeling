#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <functional>

#include <vtkCallbackCommand.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class vtkProp;
class vtkRenderWindow;

/**
 * Owns the non-model render layers used by a modeling viewport.
 *
 * The model renderer remains owned by the viewport host because picking and
 * camera navigation are still transitional responsibilities there. This
 * class owns the appearance/highlight and reference overlay renderers and
 * keeps their cameras independent from the model camera.
 */
class RenderPipeline {
public:
    using SceneLightConfigurer = std::function<void(vtkRenderer*)>;

    bool initialize(vtkRenderWindow* renderWindow,
                    vtkRenderer* modelRenderer,
                    const SceneLightConfigurer& configureLights);

    vtkRenderer* appearanceOverlay();
    vtkRenderer* referenceOverlay();

    void configureAppearanceAlwaysOnTop();
    void configureReferenceAlwaysOnTop();

    void addAppearanceProp(vtkProp* prop);
    void addReferenceProp(vtkProp* prop);
    void removeProp(vtkProp* prop);

    void syncCameras();

private:
    vtkRenderer* ensureAppearanceOverlay();
    vtkRenderer* ensureReferenceOverlay();
    vtkRenderer* configureOverlay(vtkRenderer* overlay,
                                  vtkSmartPointer<vtkCallbackCommand>& observer,
                                  bool disableDepthTest);

    vtkRenderWindow* renderWindow_ = nullptr;
    vtkRenderer* modelRenderer_ = nullptr;
    SceneLightConfigurer configureLights_;
    vtkSmartPointer<vtkRenderer> appearanceOverlay_;
    vtkSmartPointer<vtkRenderer> referenceOverlay_;
    vtkSmartPointer<vtkCallbackCommand> appearanceObserver_;
    vtkSmartPointer<vtkCallbackCommand> referenceObserver_;
};

#endif // RENDER_PIPELINE_H
