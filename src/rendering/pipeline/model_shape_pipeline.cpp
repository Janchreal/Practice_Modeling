#include "model_shape_pipeline.h"

#include <IVtk_Types.hxx>
#include <IVtkTools_DisplayModeFilter.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace {

vtkSmartPointer<IVtkTools_SubPolyDataFilter>
createHighlightFilterImpl(IVtkTools_ShapeDataSource* source)
{
    auto filter = vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
    if (source) {
        filter->SetInputConnection(source->GetOutputPort());
    }
    filter->SetIdsArrayName("SUBSHAPE_IDS");
    return filter;
}

vtkSmartPointer<vtkActor>
createHighlightActorImpl(IVtkTools_SubPolyDataFilter* filter)
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (filter) {
        mapper->SetInputConnection(filter->GetOutputPort());
    }
    mapper->ScalarVisibilityOff();

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 0.0);
    actor->GetProperty()->SetOpacity(0.6);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOff();
    actor->GetProperty()->SetLighting(true);
    actor->SetPickable(false);
    actor->SetVisibility(false);
    return actor;
}

void rewireHighlightActorImpl(vtkActor* actor, IVtkTools_SubPolyDataFilter* filter)
{
    if (!actor) {
        return;
    }

    const int oldVisibility = actor->GetVisibility();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (filter) {
        mapper->SetInputConnection(filter->GetOutputPort());
    }
    mapper->ScalarVisibilityOff();

    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 0.0);
    actor->GetProperty()->SetOpacity(0.6);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOff();
    actor->GetProperty()->SetLighting(true);
    actor->SetPickable(false);
    actor->SetVisibility(oldVisibility);
}

} // namespace

namespace ModelShapePipeline {

vtkSmartPointer<IVtkTools_DisplayModeFilter>
configureSolidShapePipeline(IVtkTools_ShapeDataSource* source,
                            vtkDataSetMapper* mapper,
                            vtkActor* actor)
{
    if (!source || !mapper || !actor) {
        return nullptr;
    }

    auto filter = vtkSmartPointer<IVtkTools_DisplayModeFilter>::New();
    filter->SetInputConnection(source->GetOutputPort());
    filter->SetDisplayMode(DM_Shading);
    filter->SetSmoothShading(true);
    mapper->SetInputConnection(filter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    actor->SetMapper(mapper);
    return filter;
}

vtkSmartPointer<IVtkTools_ShapeDataSource>
createShapeDataSource(const Handle(IVtkOCC_Shape)& shapeWrapper)
{
    if (shapeWrapper.IsNull()) {
        return nullptr;
    }

    auto source = vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    source->SetShape(shapeWrapper);
    source->Modified();
    source->Update();
    return source;
}

vtkSmartPointer<vtkPolyData>
copyShapeDataSourceOutput(IVtkTools_ShapeDataSource* source, bool deepCopy)
{
    auto copy = vtkSmartPointer<vtkPolyData>::New();
    if (!source) {
        return copy;
    }

    source->Update();
    vtkPolyData* output = source->GetOutput();
    if (!output) {
        return copy;
    }

    if (deepCopy) {
        copy->DeepCopy(output);
    } else {
        copy->ShallowCopy(output);
    }
    return copy;
}

vtkSmartPointer<IVtkTools_SubPolyDataFilter>
createHighlightFilter(IVtkTools_ShapeDataSource* source)
{
    return createHighlightFilterImpl(source);
}

vtkSmartPointer<vtkActor>
createHighlightActor(IVtkTools_SubPolyDataFilter* filter)
{
    return createHighlightActorImpl(filter);
}

void rewireHighlightActor(vtkActor* actor, IVtkTools_SubPolyDataFilter* filter)
{
    rewireHighlightActorImpl(actor, filter);
}

} // namespace ModelShapePipeline
