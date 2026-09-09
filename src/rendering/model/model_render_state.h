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
    vtkSmartPointer<IVtkTools_DisplayModeFilter> solidDisplayFilter;
    vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter;
    vtkSmartPointer<vtkActor> highlightActor;
};

#endif // MODEL_RENDER_STATE_H
