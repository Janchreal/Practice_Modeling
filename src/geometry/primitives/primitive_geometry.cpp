#include "primitive_geometry.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

namespace {

TopoDS_Shape buildSphereAt(double radius, const gp_Pnt& center)
{
    return BRepPrimAPI_MakeSphere(center, radius).Shape();
}

} // namespace

namespace PrimitiveGeometry {

bool isPrimitiveType(ModelType type)
{
    switch (type) {
    case CUBOID:
    case CYLINDER:
    case CONE:
    case SPHERE:
        return true;
    default:
        return false;
    }
}

TopoDS_Shape buildPrimitiveShape(const PrimitiveBuildRequest& request)
{
    switch (request.type) {
    case CUBOID:
        return buildCuboidShape(request.param1, request.param2, request.param3, request.placement);
    case CYLINDER:
        return buildCylinderShape(request.param1, request.param2, request.placement);
    case CONE:
        return buildConeShape(request.param1, request.param2, request.param3, request.placement);
    case SPHERE:
        return buildSphereShape(request.param1, request.placement);
    default:
        return TopoDS_Shape();
    }
}

TopoDS_Shape buildCuboidShape(double length, double width, double height,
                              const GeometryPlacement::AxisPlacement& placement)
{
    const gp_Ax2 axisSystem = GeometryPlacement::makeAxisSystem(placement);
    return BRepPrimAPI_MakeBox(axisSystem, length, width, height).Shape();
}

TopoDS_Shape buildCylinderShape(double radius, double height,
                                const GeometryPlacement::AxisPlacement& placement)
{
    const gp_Ax2 axisSystem = GeometryPlacement::makeAxisSystem(placement);
    return BRepPrimAPI_MakeCylinder(axisSystem, radius, height).Shape();
}

TopoDS_Shape buildConeShape(double radius1, double radius2, double height,
                            const GeometryPlacement::AxisPlacement& placement)
{
    const gp_Ax2 axisSystem = GeometryPlacement::makeAxisSystem(placement);
    return BRepPrimAPI_MakeCone(axisSystem, radius1, radius2, height).Shape();
}

TopoDS_Shape buildSphereShape(double radius,
                              const GeometryPlacement::AxisPlacement& placement)
{
    if (placement.hasOrigin) {
        return buildSphereAt(radius, placement.origin);
    }
    return BRepPrimAPI_MakeSphere(radius).Shape();
}

} // namespace PrimitiveGeometry
