#include "primitive_geometry.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

namespace PrimitiveGeometry {

gp_Dir resolveAxis(AxisDirection axisDirection,
                   bool axisReversed,
                   bool hasCustomVectorDir,
                   const gp_Dir& customVectorDir)
{
    gp_Dir axis(0, 0, 1);
    if (hasCustomVectorDir) {
        axis = customVectorDir;
    } else if (axisDirection == AxisDirection::X) {
        axis = gp_Dir(1, 0, 0);
    } else if (axisDirection == AxisDirection::Y) {
        axis = gp_Dir(0, 1, 0);
    }
    if (axisReversed) {
        axis.Reverse();
    }
    return axis;
}

TopoDS_Shape buildCuboidShape(double length, double width, double height,
                              bool hasOrigin,
                              double originX,
                              double originY,
                              double originZ,
                              AxisDirection axisDirection,
                              bool axisReversed,
                              bool hasCustomVectorDir,
                              const gp_Dir& customVectorDir)
{
    const gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    const gp_Dir axis = resolveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    const gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeBox(axisSystem, length, width, height).Shape();
}

TopoDS_Shape buildCylinderShape(double radius, double height,
                                bool hasOrigin,
                                double originX,
                                double originY,
                                double originZ,
                                AxisDirection axisDirection,
                                bool axisReversed,
                                bool hasCustomVectorDir,
                                const gp_Dir& customVectorDir)
{
    const gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    const gp_Dir axis = resolveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    const gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeCylinder(axisSystem, radius, height).Shape();
}

TopoDS_Shape buildConeShape(double radius1, double radius2, double height,
                            bool hasOrigin,
                            double originX,
                            double originY,
                            double originZ,
                            AxisDirection axisDirection,
                            bool axisReversed,
                            bool hasCustomVectorDir,
                            const gp_Dir& customVectorDir)
{
    const gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    const gp_Dir axis = resolveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    const gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeCone(axisSystem, radius1, radius2, height).Shape();
}

TopoDS_Shape buildSphereShape(double radius,
                              bool hasOrigin,
                              double originX,
                              double originY,
                              double originZ)
{
    if (hasOrigin) {
        const gp_Pnt center(originX, originY, originZ);
        return BRepPrimAPI_MakeSphere(center, radius).Shape();
    }
    return BRepPrimAPI_MakeSphere(radius).Shape();
}

} // namespace PrimitiveGeometry
