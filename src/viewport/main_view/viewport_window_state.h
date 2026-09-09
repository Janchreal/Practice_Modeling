#ifndef VIEWPORT_MAIN_VIEW_VIEWPORT_WINDOW_STATE_H
#define VIEWPORT_MAIN_VIEW_VIEWPORT_WINDOW_STATE_H

#include "rendering/pipeline/render_pipeline.h"
#include "viewport/main_view/mouse_interactor.h"

#include <IVtkTools_ShapePicker.hxx>

#include <vtkActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;

/** Runtime resources owned by the main modeling viewport. */
class ViewportWindowState {
protected:
    QVTKOpenGLNativeWidget* vtkWidget = nullptr;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkOrientationMarkerWidget> axesWidget;
    RenderPipeline renderPipeline_;
    vtkSmartPointer<vtkRenderer> centerAxesRenderer;
    double lastWorldPoint[4] = {0.0, 0.0, 0.0, 1.0};
    double overlayScaleRefDistance_ = 14.0;
    double overlayScaleRefParallel_ = 1.0;
    vtkSmartPointer<IVtkTools_ShapePicker> shapePicker;
    vtkSmartPointer<vtkActor> previewActor;
    vtkSmartPointer<MouseInteractorStyle> m_interactorStyle;
};

#endif // VIEWPORT_MAIN_VIEW_VIEWPORT_WINDOW_STATE_H
