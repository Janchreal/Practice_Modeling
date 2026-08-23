#ifndef OCCVTKCONVERTER_H
#define OCCVTKCONVERTER_H

#include <vtkSmartPointer.h>

// 前置声明
class vtkPolyData;
class TopoDS_Shape;

class OccShapeToVtkConverter {
public:
    OccShapeToVtkConverter();
    ~OccShapeToVtkConverter();

    vtkSmartPointer<vtkPolyData> convert(const TopoDS_Shape& shape);
    void setLinearDeflection(double deflection);
    void setAngularDeflection(double deflection);

private:
    class Private;
    Private* d;
};

#endif // OCCVTKCONVERTER_H
