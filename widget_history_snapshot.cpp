// 建模历史快照：捕获/恢复/索引重映射，支撑级联更新的撤销重做
#include "widget.h"
#include "ui_widget.h"
#include "modelhistorysnapshot.h"

#include <QSet>
#include <algorithm>

#include <vtkRenderWindow.h>

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

ModelHistorySnapshot Widget::captureModelSnapshot(int index) const
{
    ModelHistorySnapshot snapshot;
    if (index < 0 || index >= historyList.size()) {
        return snapshot;
    }

    const ModelingHistory& record = historyList[index];
    snapshot.historyIndex = index;
    snapshot.type = record.type;
    snapshot.name = record.name;
    snapshot.color = record.color;
    snapshot.param1 = record.param1;
    snapshot.param2 = record.param2;
    snapshot.param3 = record.param3;
    snapshot.occShape = record.occShape;
    snapshot.recipe = record.recipe;
    snapshot.featureRegenerateFailed = record.featureRegenerateFailed;
    snapshot.booleanTargetIndex = record.booleanTargetIndex;
    snapshot.booleanToolIndices = record.booleanToolIndices;
    snapshot.booleanOperationType = record.booleanOperationType;
    snapshot.booleanKeepTarget = record.booleanKeepTarget;
    snapshot.booleanKeepTool = record.booleanKeepTool;
    snapshot.hasOrigin = record.hasOrigin;
    snapshot.originX = record.originX;
    snapshot.originY = record.originY;
    snapshot.originZ = record.originZ;
    snapshot.axisDirection = record.axisDirection;
    snapshot.axisReversed = record.axisReversed;
    snapshot.hasCustomVectorDir = record.hasCustomVectorDir;
    snapshot.customVectorDir = record.customVectorDir;
    snapshot.visible = isModelVisibleForCommand(index);
    return snapshot;
}

QList<ModelHistorySnapshot> Widget::captureModelSnapshots(const QList<int>& indices) const
{
    QList<ModelHistorySnapshot> snapshots;
    snapshots.reserve(indices.size());
    for (int index : indices) {
        snapshots.append(captureModelSnapshot(index));
    }
    return snapshots;
}

QList<int> Widget::collectCascadeAffectedIndices(int rootIndex) const
{
    QSet<int> affected;
    if (rootIndex >= 0 && rootIndex < historyList.size()) {
        affected.insert(rootIndex);
        for (int dependentIndex : collectDependentFeatureIndices(rootIndex)) {
            affected.insert(dependentIndex);
        }
    }

    QList<int> ordered = affected.values();
    std::sort(ordered.begin(), ordered.end());
    return ordered;
}

CascadeUndoRecord Widget::beginCascadeUndoCapture(int rootIndex) const
{
    CascadeUndoRecord record;
    record.affectedIndices = collectCascadeAffectedIndices(rootIndex);
    record.beforeStates = captureModelSnapshots(record.affectedIndices);
    return record;
}

void Widget::finishCascadeUndoCapture(CascadeUndoRecord& record) const
{
    record.afterStates = captureModelSnapshots(record.affectedIndices);
    record.committed = true;
}

void Widget::restoreModelSnapshot(int index, const ModelHistorySnapshot& snapshot)
{
    if (index < 0 || index >= historyList.size() || snapshot.occShape.IsNull()) {
        return;
    }

    ModelingHistory& record = historyList[index];
    record.type = snapshot.type;
    record.name = snapshot.name;
    record.color = snapshot.color;
    record.param1 = snapshot.param1;
    record.param2 = snapshot.param2;
    record.param3 = snapshot.param3;
    record.recipe = snapshot.recipe;
    record.featureRegenerateFailed = snapshot.featureRegenerateFailed;
    record.booleanTargetIndex = snapshot.booleanTargetIndex;
    record.booleanToolIndices = snapshot.booleanToolIndices;
    record.booleanOperationType = snapshot.booleanOperationType;
    record.booleanKeepTarget = snapshot.booleanKeepTarget;
    record.booleanKeepTool = snapshot.booleanKeepTool;
    record.hasOrigin = snapshot.hasOrigin;
    record.originX = snapshot.originX;
    record.originY = snapshot.originY;
    record.originZ = snapshot.originZ;
    record.axisDirection = snapshot.axisDirection;
    record.axisReversed = snapshot.axisReversed;
    record.hasCustomVectorDir = snapshot.hasCustomVectorDir;
    record.customVectorDir = snapshot.customVectorDir;

    applyShapeToHistory(index, snapshot.occShape, snapshot.name);
    setModelVisibleForCommand(index, snapshot.visible);
}

void Widget::restoreCascadeUndoStates(const CascadeUndoRecord& record, bool useBeforeStates)
{
    const QList<ModelHistorySnapshot>& states = useBeforeStates ? record.beforeStates : record.afterStates;

    ++cascadeUpdateGuard_;
    for (const ModelHistorySnapshot& snapshot : states) {
        if (snapshot.historyIndex >= 0 && snapshot.historyIndex < historyList.size()) {
            restoreModelSnapshot(snapshot.historyIndex, snapshot);
        }
    }
    --cascadeUpdateGuard_;

    updateHistoryList();
    updateFeatureTree();

    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    if (shapePicker) {
        shapePicker->SetRenderer(renderer);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }

    markDocumentModified(true);
}

void Widget::remapHistoryIndicesAfterRemoval(int removedIndex)
{
    if (removedIndex < 0) {
        return;
    }

    for (ModelingHistory& record : historyList) {
        remapSingleIndex(record.booleanTargetIndex, removedIndex);
        remapIndexList(record.booleanToolIndices, removedIndex);
        if (record.recipe.hasRecipe) {
            remapRecipeIndices(record.recipe, removedIndex);
        }
    }
}

void Widget::remapHistoryIndicesAfterInsertion(int insertedIndex)
{
    if (insertedIndex < 0) {
        return;
    }

    for (ModelingHistory& record : historyList) {
        remapSingleIndexForInsert(record.booleanTargetIndex, insertedIndex);
        remapIndexListForInsert(record.booleanToolIndices, insertedIndex);
        if (record.recipe.hasRecipe) {
            remapRecipeIndicesForInsert(record.recipe, insertedIndex);
        }
    }
}

void Widget::restoreModelFromSnapshot(int index, const ModelHistorySnapshot& snapshot)
{
    if (snapshot.type == WORK_CSYS) {
        restoreWorkCsys(index, snapshot.name, snapshot.color,
                        snapshot.param1, snapshot.param2, snapshot.param3);
        return;
    }
    if (snapshot.type == REFERENCE_CSYS) {
        restoreReferenceCsys(index, snapshot.name, snapshot.color);
        return;
    }

    restoreModel(index, snapshot.name, snapshot.type, snapshot.color,
                 snapshot.param1, snapshot.param2, snapshot.param3, snapshot.occShape,
                 snapshot.hasOrigin, snapshot.originX, snapshot.originY, snapshot.originZ,
                 snapshot.axisDirection, snapshot.axisReversed,
                 snapshot.hasCustomVectorDir, snapshot.customVectorDir);

    const int restoredIndex = (index >= 0 && index < historyList.size()) ? index : historyList.size() - 1;
    if (restoredIndex < 0 || restoredIndex >= historyList.size()) {
        return;
    }

    ModelingHistory& record = historyList[restoredIndex];
    record.recipe = snapshot.recipe;
    record.featureRegenerateFailed = snapshot.featureRegenerateFailed;
    record.booleanTargetIndex = snapshot.booleanTargetIndex;
    record.booleanToolIndices = snapshot.booleanToolIndices;
    record.booleanOperationType = snapshot.booleanOperationType;
    record.booleanKeepTarget = snapshot.booleanKeepTarget;
    record.booleanKeepTool = snapshot.booleanKeepTool;
    setModelVisibleForCommand(restoredIndex, snapshot.visible);
}
