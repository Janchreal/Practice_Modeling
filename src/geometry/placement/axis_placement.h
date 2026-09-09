#ifndef AXIS_PLACEMENT_H
#define AXIS_PLACEMENT_H

#include "common/axisdirection.h"

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace GeometryPlacement {

struct AxisPlacement {
    bool hasOrigin = false;
    gp_Pnt origin = gp_Pnt(0, 0, 0);
    AxisDirection axisDirection = AxisDirection::Z;
    bool axisReversed = false;
    bool hasCustomVectorDir = false;
    gp_Dir customVectorDir = gp_Dir(0, 0, 1);
};

gp_Pnt resolveOrigin(bool hasOrigin,
                     double originX,
                     double originY,
                     double originZ);

gp_Dir resolveAxis(AxisDirection axisDirection,
                   bool axisReversed,
                   bool hasCustomVectorDir,
                   const gp_Dir& customVectorDir);

AxisPlacement makeAxisPlacement(bool hasOrigin,
                                double originX,
                                double originY,
                                double originZ,
                                AxisDirection axisDirection,
                                bool axisReversed,
                                bool hasCustomVectorDir,
                                const gp_Dir& customVectorDir);

gp_Ax2 makeAxisSystem(const AxisPlacement& placement);

} // namespace GeometryPlacement

#endif // AXIS_PLACEMENT_H
