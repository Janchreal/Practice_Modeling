#ifndef PRIMITIVE_GEOMETRY_H
#define PRIMITIVE_GEOMETRY_H

#include "axisdirection.h"

#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>

namespace PrimitiveGeometry {

gp_Dir resolveAxis(AxisDirection axisDirection,
                   bool axisReversed,
                   bool hasCustomVectorDir,
                   const gp_Dir& customVectorDir);

TopoDS_Shape buildCuboidShape(double length, double width, double height,
                              bool hasOrigin = false,
                              double originX = 0.0,
                              double originY = 0.0,
                              double originZ = 0.0,
                              AxisDirection axisDirection = AxisDirection::Z,
                              bool axisReversed = false,
                              bool hasCustomVectorDir = false,
                              const gp_Dir& customVectorDir = gp_Dir(0, 0, 1));

TopoDS_Shape buildCylinderShape(double radius, double height,
                                bool hasOrigin = false,
                                double originX = 0.0,
                                double originY = 0.0,
                                double originZ = 0.0,
                                AxisDirection axisDirection = AxisDirection::Z,
                                bool axisReversed = false,
                                bool hasCustomVectorDir = false,
                                const gp_Dir& customVectorDir = gp_Dir(0, 0, 1));

TopoDS_Shape buildConeShape(double radius1, double radius2, double height,
                            bool hasOrigin = false,
                            double originX = 0.0,
                            double originY = 0.0,
                            double originZ = 0.0,
                            AxisDirection axisDirection = AxisDirection::Z,
                            bool axisReversed = false,
                            bool hasCustomVectorDir = false,
                            const gp_Dir& customVectorDir = gp_Dir(0, 0, 1));

TopoDS_Shape buildSphereShape(double radius,
                              bool hasOrigin = false,
                              double originX = 0.0,
                              double originY = 0.0,
                              double originZ = 0.0);

} // namespace PrimitiveGeometry

#endif // PRIMITIVE_GEOMETRY_H
