#include "shape_presentation_factory.h"
#include "model_display_style.h"
#include "model_shape_pipeline.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <algorithm>

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkProperty.h>

namespace {

void prepareDisplayTriangulation(const TopoDS_Shape& shape,
                                 const ShapePresentationOptions& options)
{
    if (shape.IsNull()) {
        return;
    }

    // A document may contain a coarse cached triangulation. Rebuild the
    // disposable display mesh so it follows the current quality policy.
    BRepTools::Clean(shape);

    // Clamp legacy callers as well: 0.3 radians is visibly faceted on
    // cones/spheres and creates false feature edges at intersections.
    const double deflection = std::clamp(options.meshDeflection, 0.0005, 0.01);
    const double angle = std::clamp(options.meshAngle, 0.02, 0.08);

    // The constructor performs the meshing immediately.
    BRepMesh_IncrementalMesh(
        shape, deflection, Standard_False, angle, Standard_True);
}

} // namespace

namespace ShapePresentationFactory {

ModelRenderState createSolidModelState(const TopoDS_Shape& shape,
                                       const ShapePresentationOptions& options)
{
    ModelRenderState state;
    if (shape.IsNull()) {
        return state;
    }

    prepareDisplayTriangulation(shape, options);

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

    prepareDisplayTriangulation(shape, options);

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
