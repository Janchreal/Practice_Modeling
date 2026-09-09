#include "intersection_edge_pipeline.h"

#include "model_display_style.h"

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GeomAbs_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <algorithm>
#include <cmath>

#include <vtkCellArray.h>
#include <vtkMapper.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace IntersectionEdgePipeline {

vtkSmartPointer<vtkPolyData> createPolyData(const TopoDS_Shape& sectionShape,
                                            double deflection)
{
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    if (sectionShape.IsNull()) {
        return polyData;
    }

    const double safeDeflection = std::max(1.0e-6, deflection);
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    for (TopExp_Explorer explorer(sectionShape, TopAbs_EDGE);
         explorer.More();
         explorer.Next()) {
        const TopoDS_Shape& shape = explorer.Current();
        if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
            continue;
        }

        try {
            const TopoDS_Edge edge = TopoDS::Edge(shape);
            BRepAdaptor_Curve curve(edge);
            const Standard_Real first = curve.FirstParameter();
            const Standard_Real last = curve.LastParameter();
            if (!std::isfinite(first) || !std::isfinite(last)
                || std::abs(last - first) <= 1.0e-12) {
                continue;
            }

            GCPnts_QuasiUniformDeflection sampler(
                curve, safeDeflection, first, last, GeomAbs_C1);
            if (!sampler.IsDone() || sampler.NbPoints() < 2) {
                continue;
            }

            const vtkIdType startId = points->GetNumberOfPoints();
            const vtkIdType count = sampler.NbPoints();
            for (vtkIdType i = 1; i <= count; ++i) {
                const gp_Pnt point = curve.Value(sampler.Parameter(i));
                points->InsertNextPoint(point.X(), point.Y(), point.Z());
            }

            vtkSmartPointer<vtkPolyLine> polyline =
                vtkSmartPointer<vtkPolyLine>::New();
            polyline->GetPointIds()->SetNumberOfIds(count);
            for (vtkIdType i = 0; i < count; ++i) {
                polyline->GetPointIds()->SetId(i, startId + i);
            }
            lines->InsertNextCell(polyline);
        } catch (...) {
            // A malformed section edge must not prevent the remaining edges
            // from being displayed.
        }
    }

    polyData->SetPoints(points);
    polyData->SetLines(lines);
    return polyData;
}

vtkSmartPointer<vtkActor> createActor(vtkPolyData* polyData)
{
    if (!polyData || polyData->GetNumberOfLines() == 0) {
        return nullptr;
    }

    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);
    mapper->ScalarVisibilityOff();
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    // Keep the exact section curve just in front of the two tessellated
    // surfaces without pushing it far enough to reveal it through the model.
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2.0, -2.0);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.82, 0.12);
    actor->GetProperty()->SetLineWidth(2.0);
    ModelDisplayStyle::applyFeaturePickHighlight(actor, true);
    // applyFeaturePickHighlight is shared with selection previews and uses a
    // much larger offset there. Intersection curves must stay close to the
    // actual section or they can appear detached from the two solids.
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2.0, -2.0);
    actor->SetPickable(false);
    return actor;
}

} // namespace IntersectionEdgePipeline
