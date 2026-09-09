#ifndef MODEL_SHAPE_PIPELINE_H
#define MODEL_SHAPE_PIPELINE_H

#include <IVtkOCC_Shape.hxx>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkDataSetMapper;
class vtkPolyData;

class IVtkTools_DisplayModeFilter;
class IVtkTools_ShapeDataSource;
class IVtkTools_SubPolyDataFilter;

namespace ModelShapePipeline {

vtkSmartPointer<IVtkTools_ShapeDataSource>
createShapeDataSource(const Handle(IVtkOCC_Shape)& shapeWrapper);

vtkSmartPointer<vtkPolyData>
copyShapeDataSourceOutput(IVtkTools_ShapeDataSource* source, bool deepCopy = false);

vtkSmartPointer<IVtkTools_DisplayModeFilter>
configureSolidShapePipeline(IVtkTools_ShapeDataSource* source,
                            vtkDataSetMapper* mapper,
                            vtkActor* actor);

vtkSmartPointer<IVtkTools_SubPolyDataFilter>
createHighlightFilter(IVtkTools_ShapeDataSource* source);

vtkSmartPointer<vtkActor>
createHighlightActor(IVtkTools_SubPolyDataFilter* filter);

void rewireHighlightActor(vtkActor* actor, IVtkTools_SubPolyDataFilter* filter);

} // namespace ModelShapePipeline

#endif // MODEL_SHAPE_PIPELINE_H
