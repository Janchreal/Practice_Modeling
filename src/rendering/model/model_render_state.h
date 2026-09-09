#ifndef MODEL_RENDER_STATE_H
#define MODEL_RENDER_STATE_H

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_DisplayModeFilter.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// VTK state associated with one document record. Never serialize this type.
struct ModelRenderState {
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkActor> outlineActor;
    vtkSmartPointer<vtkPolyData> polyData;
    Handle(IVtkOCC_Shape) shapeWrapper;
    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource;
    // Invisible surface proxy used only for selecting a closed sketch profile.
    // The visible sketch remains a line presentation and keeps its original
    // edge ShapeSource for sub-shape picking.
    vtkSmartPointer<vtkActor> profilePickActor;
    Handle(IVtkOCC_Shape) profilePickShapeWrapper;
    vtkSmartPointer<IVtkTools_ShapeDataSource> profilePickShapeDataSource;
    vtkSmartPointer<IVtkTools_DisplayModeFilter> solidDisplayFilter;
    vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter;
    vtkSmartPointer<vtkActor> highlightActor;
};

#endif // MODEL_RENDER_STATE_H
