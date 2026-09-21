#include "primitive_geometry.h"
#include "primitive_shapes.h"

#include <memory>//用于std::make_unique、std::unique_ptr智能指针

namespace PrimitiveGeometry {
    //判断传入的ModelType是否为基本图元类型
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
//创建具体图元对象
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
/*
对外的统一构建函数
1.从request中取出type
2.调用createPrimitive(request.type)创建对应图元对象
3.如果创建成功，调用多态接口：primitive->build(request)返回TopoDS_Shape
4.如果创建失败，返回默认构造的 TopoDS_Shape()，也就是一个空形状null shape
*/
TopoDS_Shape buildPrimitiveShape(const PrimitiveBuildRequest& request)
{
    const PrimitivePtr primitive = createPrimitive(request.type);
    return primitive ? primitive->build(request) : TopoDS_Shape();
}
//四个便捷构建函数这些函数直接创建具体图元对象并调用build()，相当于针对常用图元的语法糖
//构建长方体placement轴放置/位置姿态信息
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
