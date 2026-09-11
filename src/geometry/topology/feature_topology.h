#ifndef FEATURE_TOPOLOGY_H
#define FEATURE_TOPOLOGY_H

#include "sub_shape_reference.h"

#include <cstdint>
#include <TopoDS_Shape.hxx>

SubShapeRef makeSubShapeRef(int parentIndex,
                            const TopoDS_Shape& parentShape,
                            const TopoDS_Shape& subShape,
                            TopAbs_ShapeEnum shapeType,
                            std::int64_t subShapeId = -1);

SubShapeRef makeSketchContourRef(int parentIndex,
                                 int contourIndex,
                                 const TopoDS_Shape& contourProfile,
                                 std::int64_t subShapeId = -1);

TopoDS_Shape resolveSubShapeRef(const TopoDS_Shape& parentShape, const SubShapeRef& ref);

#endif // FEATURE_TOPOLOGY_H
