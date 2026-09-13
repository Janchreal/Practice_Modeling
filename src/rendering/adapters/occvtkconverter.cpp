#include "occvtkconverter.h"

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>

#include <Standard_Failure.hxx>

#include <vtkPolyData.h>
#include <TopoDS_Shape.hxx>

vtkSmartPointer<vtkPolyData> OccShapeToVtkConverter::convert(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return nullptr;
    }

    try {
        // IVtk owns display tessellation. Keep this fallback on the same
        // ShapeDataSource path as the main model presentation.
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDataSource->SetShape(shapeWrapper);
        shapeDataSource->Modified();
        shapeDataSource->Update();

        vtkPolyData* output = shapeDataSource->GetOutput();
        if (!output) {
            return nullptr;
        }

        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->DeepCopy(output);
        return polyData;

    } catch (const Standard_Failure&) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}
