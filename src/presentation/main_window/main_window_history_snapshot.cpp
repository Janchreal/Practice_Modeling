// 建模历史快照：捕获/恢复/索引重映射，支撑级联更新的撤销重做
#include "main_window.h"
#include "ui_main_window.h"
#include "application/history/modeling_history_index_remap.h"
#include "application/history/modeling_history_placement.h"
#include "application/history/model_history_snapshot.h"

#include <QSet>
#include <algorithm>

#include <vtkRenderWindow.h>

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
    snapshot.occShape = geometryStateFor(record).occShape;
    snapshot.recipe = record.recipe;
    snapshot.featureRegenerateFailed = record.featureRegenerateFailed;
    snapshot.booleanTargetIndex = record.booleanTargetIndex;
    snapshot.booleanToolIndices = record.booleanToolIndices;
    snapshot.booleanOperationType = record.booleanOperationType;
    snapshot.booleanKeepTarget = record.booleanKeepTarget;
    snapshot.booleanKeepTool = record.booleanKeepTool;
    ModelingHistoryPlacement::copyPlacementFromHistoryToSnapshot(record, snapshot);
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
    ModelingHistoryPlacement::copyPlacementFromSnapshotToHistory(snapshot, record);

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

    refreshShapePickerBindingsForCurrentContext();

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }

    markDocumentModified(true);
}

void Widget::remapHistoryIndicesAfterRemoval(int removedIndex)
{
    ModelingHistoryIndexRemap::afterRemoval(historyList, removedIndex);
}

void Widget::remapHistoryIndicesAfterInsertion(int insertedIndex)
{
    ModelingHistoryIndexRemap::afterInsertion(historyList, insertedIndex);
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
                 ModelingHistoryPlacement::placementFromSnapshot(snapshot));

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
