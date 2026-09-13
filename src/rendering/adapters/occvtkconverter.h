#ifndef OCCVTKCONVERTER_H
#define OCCVTKCONVERTER_H

#include <vtkSmartPointer.h>

// 前置声明
class vtkPolyData;
class TopoDS_Shape;

class OccShapeToVtkConverter {
public:
    OccShapeToVtkConverter() = default;
    ~OccShapeToVtkConverter() = default;

    vtkSmartPointer<vtkPolyData> convert(const TopoDS_Shape& shape);
};

#endif // OCCVTKCONVERTER_H
