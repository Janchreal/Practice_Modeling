#include "axis_placement.h"

namespace GeometryPlacement {

gp_Pnt resolveOrigin(bool hasOrigin,
                     double originX,
                     double originY,
                     double originZ)
{
    return hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
}

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

AxisPlacement makeAxisPlacement(bool hasOrigin,
                                double originX,
                                double originY,
                                double originZ,
                                AxisDirection axisDirection,
                                bool axisReversed,
                                bool hasCustomVectorDir,
                                const gp_Dir& customVectorDir)
{
    AxisPlacement placement;
    placement.hasOrigin = hasOrigin;
    placement.origin = resolveOrigin(hasOrigin, originX, originY, originZ);
    placement.axisDirection = axisDirection;
    placement.axisReversed = axisReversed;
    placement.hasCustomVectorDir = hasCustomVectorDir;
    placement.customVectorDir = customVectorDir;
    return placement;
}

gp_Ax2 makeAxisSystem(const AxisPlacement& placement)
{
    return gp_Ax2(placement.origin,
                  resolveAxis(placement.axisDirection,
                              placement.axisReversed,
                              placement.hasCustomVectorDir,
                              placement.customVectorDir));
}

} // namespace GeometryPlacement
