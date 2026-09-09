#include "primitive_shapes.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <Standard_Failure.hxx>

namespace {

bool hasPositiveParameters(double first, double second = 0.0, double third = 0.0,
                           bool checkSecond = true, bool checkThird = false)
{
    if (first <= 0.0)
        return false;
    if (checkSecond && second <= 0.0)
        return false;
    if (checkThird && third <= 0.0)
        return false;
    return true;
}

TopoDS_Shape emptyShape()
{
    return TopoDS_Shape();
}

} // namespace

namespace PrimitiveGeometry {

ModelType CuboidPrimitive::modelType() const
{
    return CUBOID;
}

TopoDS_Shape CuboidPrimitive::build(const PrimitiveBuildRequest& request) const
{
    if (request.type != modelType()
        || !hasPositiveParameters(request.param1, request.param2, request.param3, true, true)) {
        return emptyShape();
    }

    try {
        return BRepPrimAPI_MakeBox(
            GeometryPlacement::makeAxisSystem(request.placement),
            request.param1,
            request.param2,
            request.param3).Shape();
    } catch (const Standard_Failure&) {
        return emptyShape();
    }
}

ModelType CylinderPrimitive::modelType() const
{
    return CYLINDER;
}

TopoDS_Shape CylinderPrimitive::build(const PrimitiveBuildRequest& request) const
{
    if (request.type != modelType()
        || !hasPositiveParameters(request.param1, request.param2)) {
        return emptyShape();
    }

    try {
        return BRepPrimAPI_MakeCylinder(
            GeometryPlacement::makeAxisSystem(request.placement),
            request.param1,
            request.param2).Shape();
    } catch (const Standard_Failure&) {
        return emptyShape();
    }
}

ModelType ConePrimitive::modelType() const
{
    return CONE;
}

TopoDS_Shape ConePrimitive::build(const PrimitiveBuildRequest& request) const
{
    if (request.type != modelType()
        || !hasPositiveParameters(request.param1, request.param3, 0.0, true, false)) {
        return emptyShape();
    }

    if (request.param2 < 0.0)
        return emptyShape();

    try {
        return BRepPrimAPI_MakeCone(
            GeometryPlacement::makeAxisSystem(request.placement),
            request.param1,
            request.param2,
            request.param3).Shape();
    } catch (const Standard_Failure&) {
        return emptyShape();
    }
}

ModelType SpherePrimitive::modelType() const
{
    return SPHERE;
}

TopoDS_Shape SpherePrimitive::build(const PrimitiveBuildRequest& request) const
{
    if (request.type != modelType() || request.param1 <= 0.0)
        return emptyShape();

    try {
        if (request.placement.hasOrigin) {
            return BRepPrimAPI_MakeSphere(request.placement.origin, request.param1).Shape();
        }
        return BRepPrimAPI_MakeSphere(request.param1).Shape();
    } catch (const Standard_Failure&) {
        return emptyShape();
    }
}

} // namespace PrimitiveGeometry
