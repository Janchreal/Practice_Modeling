#ifndef INTERSECTION_OPS_H
#define INTERSECTION_OPS_H

#include <TopoDS_Shape.hxx>

namespace IntersectionOps {

TopoDS_Shape computeSection(const TopoDS_Shape& first,
                            const TopoDS_Shape& second,
                            double fuzzyValue = 0.0);

bool hasIntersection(const TopoDS_Shape& first,
                     const TopoDS_Shape& second,
                     double fuzzyValue = 0.0);

} // namespace IntersectionOps

#endif // INTERSECTION_OPS_H
