#ifndef MODELING_HISTORY_PRIMITIVES_H
#define MODELING_HISTORY_PRIMITIVES_H

#include "modeling_history_placement.h"
#include "application/history/model_history_snapshot.h"
#include "modeling_history_record.h"
#include "geometry/primitives/primitive_build_request.h"

namespace ModelingHistoryPrimitives {

inline PrimitiveGeometry::PrimitiveBuildRequest primitiveRequestFromHistory(const ModelingHistory& record)
{
    return PrimitiveGeometry::PrimitiveBuildRequest(record.type,
                                                    record.param1,
                                                    record.param2,
                                                    record.param3,
                                                    ModelingHistoryPlacement::placementFromHistory(record));
}

inline PrimitiveGeometry::PrimitiveBuildRequest primitiveRequestFromSnapshot(const ModelHistorySnapshot& snapshot)
{
    return PrimitiveGeometry::PrimitiveBuildRequest(snapshot.type,
                                                    snapshot.param1,
                                                    snapshot.param2,
                                                    snapshot.param3,
                                                    ModelingHistoryPlacement::placementFromSnapshot(snapshot));
}

inline PrimitiveGeometry::PrimitiveBuildRequest primitiveRequestFromExtrusionBase(const ExtrusionRecipeData& recipe)
{
    return PrimitiveGeometry::PrimitiveBuildRequest(recipe.baseType,
                                                    recipe.baseParam1,
                                                    recipe.baseParam2,
                                                    recipe.baseParam3,
                                                    ModelingHistoryPlacement::placementFromExtrusionBase(recipe));
}

} // namespace ModelingHistoryPrimitives

#endif // MODELING_HISTORY_PRIMITIVES_H
