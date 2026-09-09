#include "primitive_geometry.h"
#include "primitive_shapes.h"

#include <memory>

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

PrimitivePtr createPrimitive(ModelType type)
{
    switch (type) {
    case CUBOID:
        return std::make_unique<CuboidPrimitive>();
    case CYLINDER:
        return std::make_unique<CylinderPrimitive>();
    case CONE:
        return std::make_unique<ConePrimitive>();
    case SPHERE:
        return std::make_unique<SpherePrimitive>();
    default:
        return nullptr;
    }
}

TopoDS_Shape buildPrimitiveShape(const PrimitiveBuildRequest& request)
{
    const PrimitivePtr primitive = createPrimitive(request.type);
    return primitive ? primitive->build(request) : TopoDS_Shape();
}

TopoDS_Shape buildCuboidShape(double length, double width, double height,
                              const GeometryPlacement::AxisPlacement& placement)
{
    return CuboidPrimitive().build(
        PrimitiveBuildRequest(CUBOID, length, width, height, placement));
}

TopoDS_Shape buildCylinderShape(double radius, double height,
                                const GeometryPlacement::AxisPlacement& placement)
{
    return CylinderPrimitive().build(
        PrimitiveBuildRequest(CYLINDER, radius, height, 0.0, placement));
}

TopoDS_Shape buildConeShape(double radius1, double radius2, double height,
                            const GeometryPlacement::AxisPlacement& placement)
{
    return ConePrimitive().build(
        PrimitiveBuildRequest(CONE, radius1, radius2, height, placement));
}

TopoDS_Shape buildSphereShape(double radius,
                              const GeometryPlacement::AxisPlacement& placement)
{
    return SpherePrimitive().build(
        PrimitiveBuildRequest(SPHERE, radius, 0.0, 0.0, placement));
}

} // namespace PrimitiveGeometry
