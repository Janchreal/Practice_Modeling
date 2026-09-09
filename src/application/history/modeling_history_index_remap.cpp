#include "modeling_history_index_remap.h"

namespace {

void remapSingleIndex(int& index, int removedIndex)
{
    if (index > removedIndex) {
        --index;
    } else if (index == removedIndex) {
        index = -1;
    }
}

void remapIndexList(QList<int>& indices, int removedIndex)
{
    for (int i = indices.size() - 1; i >= 0; --i) {
        if (indices[i] > removedIndex) {
            indices[i] -= 1;
        } else if (indices[i] == removedIndex) {
            indices.removeAt(i);
        }
    }
}

void remapSingleIndexForInsert(int& index, int insertedIndex)
{
    if (index >= insertedIndex) {
        ++index;
    }
}

void remapIndexListForInsert(QList<int>& indices, int insertedIndex)
{
    for (int& index : indices) {
        remapSingleIndexForInsert(index, insertedIndex);
    }
}

void remapSubShapeRefs(QList<SubShapeRef>& refs, int removedIndex)
{
    for (SubShapeRef& ref : refs) {
        remapSingleIndex(ref.parentIndex, removedIndex);
    }
}

void remapSubShapeRefsForInsert(QList<SubShapeRef>& refs, int insertedIndex)
{
    for (SubShapeRef& ref : refs) {
        remapSingleIndexForInsert(ref.parentIndex, insertedIndex);
    }
}

void remapRecipeIndices(FeatureRecipe& recipe, int removedIndex)
{
    remapIndexList(recipe.parentIndices, removedIndex);
    remapSingleIndex(recipe.boolean.targetIndex, removedIndex);
    remapIndexList(recipe.boolean.toolIndices, removedIndex);
    remapSingleIndex(recipe.extrusion.mergeTargetIndex, removedIndex);
    remapIndexList(recipe.extrusion.profileModelIndices, removedIndex);
    remapSubShapeRefs(recipe.extrusion.profiles, removedIndex);
    remapSubShapeRefs(recipe.revolve.profiles, removedIndex);
    remapSingleIndex(recipe.fillet.targetIndex, removedIndex);
    remapSubShapeRefs(recipe.fillet.edges, removedIndex);
    remapIndexList(recipe.pattern.sourceIndices, removedIndex);
    remapSingleIndex(recipe.hollow.targetIndex, removedIndex);
    remapSubShapeRefs(recipe.hollow.faces, removedIndex);
    remapSingleIndex(recipe.sketch.datumPlaneIndex, removedIndex);
}

void remapRecipeIndicesForInsert(FeatureRecipe& recipe, int insertedIndex)
{
    remapIndexListForInsert(recipe.parentIndices, insertedIndex);
    remapSingleIndexForInsert(recipe.boolean.targetIndex, insertedIndex);
    remapIndexListForInsert(recipe.boolean.toolIndices, insertedIndex);
    remapSingleIndexForInsert(recipe.extrusion.mergeTargetIndex, insertedIndex);
    remapIndexListForInsert(recipe.extrusion.profileModelIndices, insertedIndex);
    remapSubShapeRefsForInsert(recipe.extrusion.profiles, insertedIndex);
    remapSubShapeRefsForInsert(recipe.revolve.profiles, insertedIndex);
    remapSingleIndexForInsert(recipe.fillet.targetIndex, insertedIndex);
    remapSubShapeRefsForInsert(recipe.fillet.edges, insertedIndex);
    remapIndexListForInsert(recipe.pattern.sourceIndices, insertedIndex);
    remapSingleIndexForInsert(recipe.hollow.targetIndex, insertedIndex);
    remapSubShapeRefsForInsert(recipe.hollow.faces, insertedIndex);
    remapSingleIndexForInsert(recipe.sketch.datumPlaneIndex, insertedIndex);
}

} // namespace

namespace ModelingHistoryIndexRemap {

void afterRemoval(QList<ModelingHistory>& records, int removedIndex)
{
    if (removedIndex < 0) {
        return;
    }

    for (ModelingHistory& record : records) {
        remapSingleIndex(record.booleanTargetIndex, removedIndex);
        remapIndexList(record.booleanToolIndices, removedIndex);
        if (record.recipe.hasRecipe) {
            remapRecipeIndices(record.recipe, removedIndex);
        }
    }
}

void afterInsertion(QList<ModelingHistory>& records, int insertedIndex)
{
    if (insertedIndex < 0) {
        return;
    }

    for (ModelingHistory& record : records) {
        remapSingleIndexForInsert(record.booleanTargetIndex, insertedIndex);
        remapIndexListForInsert(record.booleanToolIndices, insertedIndex);
        if (record.recipe.hasRecipe) {
            remapRecipeIndicesForInsert(record.recipe, insertedIndex);
        }
    }
}

} // namespace ModelingHistoryIndexRemap
