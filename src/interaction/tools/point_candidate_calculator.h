#ifndef INTERACTION_TOOLS_POINT_CANDIDATE_CALCULATOR_H
#define INTERACTION_TOOLS_POINT_CANDIDATE_CALCULATOR_H

#include "point_snap_types.h"

#include <TopoDS_Edge.hxx>

class PointCandidateCalculator {
public:
    static PointCandidateList edgeCandidates(const TopoDS_Edge& edge,
                                             const PointSnapMode& mode);
};

#endif // INTERACTION_TOOLS_POINT_CANDIDATE_CALCULATOR_H
