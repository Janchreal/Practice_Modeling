#ifndef INTERACTION_TOOLS_POINT_SNAP_MANAGER_H
#define INTERACTION_TOOLS_POINT_SNAP_MANAGER_H

#include "point_snap_types.h"

class PointSnapManager {
public:
    static PointSnapMode modeFromLegacySnapKind(int snapKind);
    static PointSnapType typeFromLegacyCandidateKind(int kind);
    static int legacyCandidateKind(PointSnapType type);
    static QString labelForType(PointSnapType type);
    static int priority(PointSnapType type);
};

#endif // INTERACTION_TOOLS_POINT_SNAP_MANAGER_H
