#include "shape_presentation_factory.h"
#include "model_display_style.h"
#include "model_shape_pipeline.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkProperty.h>

namespace ShapePresentationFactory {

ModelRenderState createSolidModelState(const TopoDS_Shape& shape,
                                       const ShapePresentationOptions& options)
{
    ModelRenderState state;
    if (shape.IsNull()) {
        return state;
    }

    BRepMesh_IncrementalMesh mesh(shape, options.meshDeflection, Standard_False,
                                  options.meshAngle, Standard_True);
    mesh.Perform();

    state.shapeWrapper = new IVtkOCC_Shape(shape);
    state.shapeWrapper->SetId(options.shapeId);
    state.shapeDataSource = ModelShapePipeline::createShapeDataSource(state.shapeWrapper);

    auto mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    ModelDisplayStyle::configureSolidMapperForBoundaryOutline(mapper);

    state.actor = vtkSmartPointer<vtkActor>::New();
    state.actor->SetMapper(mapper);
    state.actor->SetPickable(true);
    ModelDisplayStyle::applySolidActorMaterial(state.actor->GetProperty());
    state.actor->GetProperty()->SetColor(options.color.redF(),
                                         options.color.greenF(),
                                         options.color.blueF());

    state.solidDisplayFilter =
        ModelShapePipeline::configureSolidShapePipeline(state.shapeDataSource, mapper, state.actor);
    IVtkTools_ShapeObject::SetShapeSource(state.shapeDataSource, state.actor);

    state.highlightFilter = ModelShapePipeline::createHighlightFilter(state.shapeDataSource);
    state.highlightActor = ModelShapePipeline::createHighlightActor(state.highlightFilter);
    state.polyData = ModelShapePipeline::copyShapeDataSourceOutput(state.shapeDataSource,
                                                                   options.deepCopyPolyData);
    return state;
}

void refreshSolidModelState(ModelRenderState& state,
                            const TopoDS_Shape& shape,
                            const ShapePresentationOptions& options)
{
    if (shape.IsNull()) {
        return;
    }

    BRepMesh_IncrementalMesh mesh(shape, options.meshDeflection, Standard_False,
                                  options.meshAngle, Standard_True);
    mesh.Perform();

    state.shapeWrapper = new IVtkOCC_Shape(shape);
    state.shapeWrapper->SetId(options.shapeId);
    state.shapeDataSource = ModelShapePipeline::createShapeDataSource(state.shapeWrapper);

    if (!state.actor) {
        state.actor = vtkSmartPointer<vtkActor>::New();
    }

    auto mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    ModelDisplayStyle::configureSolidMapperForBoundaryOutline(mapper);
    state.actor->SetMapper(mapper);
    state.actor->SetPickable(true);
    ModelDisplayStyle::applySolidActorMaterial(state.actor->GetProperty());
    state.actor->GetProperty()->SetColor(options.color.redF(),
                                         options.color.greenF(),
                                         options.color.blueF());
    state.solidDisplayFilter =
        ModelShapePipeline::configureSolidShapePipeline(state.shapeDataSource, mapper, state.actor);
    IVtkTools_ShapeObject::SetShapeSource(state.shapeDataSource, state.actor);

    if (!state.highlightFilter) {
        state.highlightFilter = ModelShapePipeline::createHighlightFilter(state.shapeDataSource);
    } else {
        state.highlightFilter->SetInputConnection(state.shapeDataSource->GetOutputPort());
        state.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
    }

    if (!state.highlightActor) {
        state.highlightActor = ModelShapePipeline::createHighlightActor(state.highlightFilter);
    } else {
        ModelShapePipeline::rewireHighlightActor(state.highlightActor, state.highlightFilter);
    }

    state.polyData = ModelShapePipeline::copyShapeDataSourceOutput(state.shapeDataSource,
                                                                   options.deepCopyPolyData);
}

} // namespace ShapePresentationFactory
