#ifndef INTERSECTION_EDGE_PIPELINE_H
#define INTERSECTION_EDGE_PIPELINE_H

#include <TopoDS_Shape.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

namespace IntersectionEdgePipeline {

vtkSmartPointer<vtkPolyData> createPolyData(const TopoDS_Shape& sectionShape,
                                            double deflection = 0.01);

vtkSmartPointer<vtkActor> createActor(vtkPolyData* polyData);

} // namespace IntersectionEdgePipeline

#endif // INTERSECTION_EDGE_PIPELINE_H
