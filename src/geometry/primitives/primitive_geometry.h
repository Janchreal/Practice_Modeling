#ifndef PRIMITIVE_GEOMETRY_H
#define PRIMITIVE_GEOMETRY_H

#include "geometry/placement/axis_placement.h"
#include "common/modeltype.h"
#include "primitive_build_request.h"

#include <TopoDS_Shape.hxx>

namespace PrimitiveGeometry {

bool isPrimitiveType(ModelType type);

TopoDS_Shape buildPrimitiveShape(const PrimitiveBuildRequest& request);

TopoDS_Shape buildCuboidShape(double length, double width, double height,
                              const GeometryPlacement::AxisPlacement& placement);

TopoDS_Shape buildCylinderShape(double radius, double height,
                                const GeometryPlacement::AxisPlacement& placement);

TopoDS_Shape buildConeShape(double radius1, double radius2, double height,
                            const GeometryPlacement::AxisPlacement& placement);

TopoDS_Shape buildSphereShape(double radius,
                              const GeometryPlacement::AxisPlacement& placement);

} // namespace PrimitiveGeometry

#endif // PRIMITIVE_GEOMETRY_H
