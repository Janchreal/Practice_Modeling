#ifndef PRIMITIVE_BUILD_REQUEST_H
#define PRIMITIVE_BUILD_REQUEST_H

#include "geometry/placement/axis_placement.h"
#include "common/modeltype.h"

namespace PrimitiveGeometry {
    //PrimitiveBuildRequest封装了构建图元所需的全部参数
struct PrimitiveBuildRequest {
    PrimitiveBuildRequest() = default;
    //模型类型、第一个参数、第二个参数、第三个个参数
    PrimitiveBuildRequest(ModelType shapeType,
                          double firstParameter,
                          double secondParameter = 0.0,
                          double thirdParameter = 0.0,
                          const GeometryPlacement::AxisPlacement& axisPlacement =
                              GeometryPlacement::AxisPlacement())
        : type(shapeType)
        , param1(firstParameter)
        , param2(secondParameter)
        , param3(thirdParameter)
        , placement(axisPlacement)
    {
    }

    ModelType type = CUBOID;
    double param1 = 0.0;
    double param2 = 0.0;
    double param3 = 0.0;
    GeometryPlacement::AxisPlacement placement;
};

} // namespace PrimitiveGeometry

#endif // PRIMITIVE_BUILD_REQUEST_H
