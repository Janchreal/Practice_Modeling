#include "occvtkconverter.h"

// 在包含任何头文件之前添加必要的宏定义
#define _USE_MATH_DEFINES
#include <cmath>

// 先包含 VTK 头文件
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>

// 然后包含 OpenCASCADE 头文件
#include <Standard_Version.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
class OccShapeToVtkConverter::Private {
public:
    double linearDeflection = 0.003;
    double angularDeflection = 0.05;
};

OccShapeToVtkConverter::OccShapeToVtkConverter()
    : d(new Private)
{
    // Keep the fallback converter consistent with the main VIS pipeline.
    d->linearDeflection = 0.003;
    d->angularDeflection = 0.05;
}

OccShapeToVtkConverter::~OccShapeToVtkConverter() {
    delete d;
}

vtkSmartPointer<vtkPolyData> OccShapeToVtkConverter::convert(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return nullptr;
    }

    try {
        // 创建网格
        BRepMesh_IncrementalMesh(
            shape, d->linearDeflection, false, d->angularDeflection);

        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();

        vtkIdType pointOffset = 0;

        // 遍历所有面
        TopExp_Explorer faceExplorer;
        for (faceExplorer.Init(shape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next()) {
            TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
            TopLoc_Location location;
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

            if (triangulation.IsNull()) {
                continue;
            }

            // 添加点 - 使用 OCCT 7.9.1 的 API
            int nbNodes = triangulation->NbNodes();
            for (int i = 1; i <= nbNodes; i++) {
                gp_Pnt pnt = triangulation->Node(i).Transformed(location);
                points->InsertNextPoint(pnt.X(), pnt.Y(), pnt.Z());
            }

            // 添加三角形 - 使用 OCCT 7.9.1 的 API
            int nbTriangles = triangulation->NbTriangles();
            for (int i = 1; i <= nbTriangles; i++) {
                Poly_Triangle triangle = triangulation->Triangle(i);
                int node1, node2, node3;
                triangle.Get(node1, node2, node3);

                // OpenCASCADE索引从1开始，VTK从0开始
                node1--; node2--; node3--;

                vtkSmartPointer<vtkTriangle> vtkTri = vtkSmartPointer<vtkTriangle>::New();
                vtkTri->GetPointIds()->SetId(0, pointOffset + node1);
                vtkTri->GetPointIds()->SetId(1, pointOffset + node2);
                vtkTri->GetPointIds()->SetId(2, pointOffset + node3);

                triangles->InsertNextCell(vtkTri);
            }

            pointOffset += nbNodes;
        }

        polyData->SetPoints(points);
        polyData->SetPolys(triangles);

        return polyData;

    } catch (const Standard_Failure&) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

void OccShapeToVtkConverter::setLinearDeflection(double deflection) {
    d->linearDeflection = deflection;
}

void OccShapeToVtkConverter::setAngularDeflection(double deflection) {
    d->angularDeflection = deflection;
}
