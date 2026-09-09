#ifndef MODELING_HISTORY_PLACEMENT_H
#define MODELING_HISTORY_PLACEMENT_H

#include "geometry/placement/axis_placement.h"
#include "domain/features/featurerecipe.h"
#include "application/history/model_history_snapshot.h"
#include "modeling_history_record.h"

namespace ModelingHistoryPlacement {

inline GeometryPlacement::AxisPlacement placementFromHistory(const ModelingHistory& record)
{
    return GeometryPlacement::makeAxisPlacement(record.hasOrigin,
                                                record.originX,
                                                record.originY,
                                                record.originZ,
                                                record.axisDirection,
                                                record.axisReversed,
                                                record.hasCustomVectorDir,
                                                record.customVectorDir);
}

inline void applyPlacementToHistory(ModelingHistory& record,
                                    const GeometryPlacement::AxisPlacement& placement)
{
    record.hasOrigin = placement.hasOrigin;
    record.originX = placement.origin.X();
    record.originY = placement.origin.Y();
    record.originZ = placement.origin.Z();
    record.axisDirection = placement.axisDirection;
    record.axisReversed = placement.axisReversed;
    record.hasCustomVectorDir = placement.hasCustomVectorDir;
    record.customVectorDir = placement.customVectorDir;
}

inline GeometryPlacement::AxisPlacement placementFromSnapshot(const ModelHistorySnapshot& snapshot)
{
    return GeometryPlacement::makeAxisPlacement(snapshot.hasOrigin,
                                                snapshot.originX,
                                                snapshot.originY,
                                                snapshot.originZ,
                                                snapshot.axisDirection,
                                                snapshot.axisReversed,
                                                snapshot.hasCustomVectorDir,
                                                snapshot.customVectorDir);
}

inline void applyPlacementToSnapshot(ModelHistorySnapshot& snapshot,
                                     const GeometryPlacement::AxisPlacement& placement)
{
    snapshot.hasOrigin = placement.hasOrigin;
    snapshot.originX = placement.origin.X();
    snapshot.originY = placement.origin.Y();
    snapshot.originZ = placement.origin.Z();
    snapshot.axisDirection = placement.axisDirection;
    snapshot.axisReversed = placement.axisReversed;
    snapshot.hasCustomVectorDir = placement.hasCustomVectorDir;
    snapshot.customVectorDir = placement.customVectorDir;
}

inline void copyPlacementFromHistoryToSnapshot(const ModelingHistory& record,
                                               ModelHistorySnapshot& snapshot)
{
    applyPlacementToSnapshot(snapshot, placementFromHistory(record));
}

inline void copyPlacementFromSnapshotToHistory(const ModelHistorySnapshot& snapshot,
                                               ModelingHistory& record)
{
    applyPlacementToHistory(record, placementFromSnapshot(snapshot));
}

inline GeometryPlacement::AxisPlacement placementFromExtrusionBase(const ExtrusionRecipeData& recipe)
{
    return GeometryPlacement::makeAxisPlacement(recipe.baseHasOrigin,
                                                recipe.baseOriginX,
                                                recipe.baseOriginY,
                                                recipe.baseOriginZ,
                                                AxisDirection::Z,
                                                false,
                                                false,
                                                gp_Dir(0, 0, 1));
}

} // namespace ModelingHistoryPlacement

#endif // MODELING_HISTORY_PLACEMENT_H
