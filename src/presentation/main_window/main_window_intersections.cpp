#include "main_window.h"

#include "geometry/intersection/intersection_ops.h"
#include "rendering/model/intersection_edge_pipeline.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#include <QSet>

#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

namespace {

bool isIntersectionCandidate(ModelType type)
{
    return type != SKETCH
        && type != DATUM_PLANE
        && type != DATUM_AXIS
        && type != WORK_CSYS
        && type != REFERENCE_CSYS;
}

bool boxesOverlap(const TopoDS_Shape& first, const TopoDS_Shape& second)
{
    if (first.IsNull() || second.IsNull()) {
        return false;
    }

    try {
        Bnd_Box firstBox;
        Bnd_Box secondBox;
        BRepBndLib::Add(first, firstBox);
        BRepBndLib::Add(second, secondBox);
        if (firstBox.IsVoid() || secondBox.IsVoid()) {
            return false;
        }
        return !firstBox.IsOut(secondBox);
    } catch (...) {
        return false;
    }
}

} // namespace

void Widget::updateIntersectionForPair(int firstIndex, int secondIndex)
{
    if (firstIndex < 0 || firstIndex >= historyList.size()
        || secondIndex < 0 || secondIndex >= historyList.size()
        || firstIndex == secondIndex) {
        return;
    }

    const ModelingHistory& firstRecord = historyList[firstIndex];
    const ModelingHistory& secondRecord = historyList[secondIndex];
    const quint64 firstId = firstRecord.id;
    const quint64 secondId = secondRecord.id;
    if (firstId == 0 || secondId == 0) {
        return;
    }

    auto removePair = [this, firstId, secondId]() {
        IntersectionRenderState* state =
            intersectionRenderStore_.find(firstId, secondId);
        if (state && state->actor) {
            removeSceneActor(state->actor);
        }
        intersectionRenderStore_.remove(firstId, secondId);
    };

    if (!isIntersectionCandidate(firstRecord.type)
        || !isIntersectionCandidate(secondRecord.type)
        || !renderStateFor(firstRecord).actor
        || !renderStateFor(secondRecord).actor
        || renderStateFor(firstRecord).actor->GetVisibility() == 0
        || renderStateFor(secondRecord).actor->GetVisibility() == 0) {
        removePair();
        return;
    }

    const TopoDS_Shape firstShape = getShapeFromHistory(firstIndex);
    const TopoDS_Shape secondShape = getShapeFromHistory(secondIndex);
    if (firstShape.IsNull() || secondShape.IsNull() || !boxesOverlap(firstShape, secondShape)) {
        removePair();
        return;
    }

    const TopoDS_Shape section =
        IntersectionOps::computeSection(firstShape, secondShape);
    vtkSmartPointer<vtkPolyData> polyData =
        IntersectionEdgePipeline::createPolyData(section, 0.003);
    vtkSmartPointer<vtkActor> actor =
        IntersectionEdgePipeline::createActor(polyData);
    if (!actor || !renderer) {
        removePair();
        return;
    }

    removePair();
    renderer->AddActor(actor);
    IntersectionRenderState& state =
        intersectionRenderStore_.ensure(firstId, secondId);
    state.polyData = polyData;
    state.actor = actor;
}

void Widget::updateIntersectionsForRecord(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }

    const quint64 recordId = historyList[index].id;
    if (recordId == 0) {
        return;
    }

    const QList<IntersectionRenderStore::Key> existing =
        intersectionRenderStore_.keysForRecord(recordId);
    for (const IntersectionRenderStore::Key& key : existing) {
        bool firstPresent = false;
        bool secondPresent = false;
        for (const ModelingHistory& record : historyList) {
            if (record.id == key.first) {
                firstPresent = true;
            }
            if (record.id == key.second) {
                secondPresent = true;
            }
        }
        if (!firstPresent || !secondPresent) {
            IntersectionRenderState* state =
                intersectionRenderStore_.find(key.first, key.second);
            if (state && state->actor) {
                removeSceneActor(state->actor);
            }
            intersectionRenderStore_.remove(key.first, key.second);
        }
    }

    for (int otherIndex = 0; otherIndex < historyList.size(); ++otherIndex) {
        if (otherIndex != index) {
            updateIntersectionForPair(index, otherIndex);
        }
    }
}

void Widget::removeIntersectionsForRecord(quint64 recordId)
{
    if (recordId == 0) {
        return;
    }

    const QList<IntersectionRenderStore::Key> keys =
        intersectionRenderStore_.keysForRecord(recordId);
    for (const IntersectionRenderStore::Key& key : keys) {
        IntersectionRenderState* state =
            intersectionRenderStore_.find(key.first, key.second);
        if (state && state->actor) {
            removeSceneActor(state->actor);
        }
        intersectionRenderStore_.remove(key.first, key.second);
    }
}

void Widget::clearIntersectionRenderStates()
{
    for (int i = 0; i < historyList.size(); ++i) {
        removeIntersectionsForRecord(historyList[i].id);
    }
    intersectionRenderStore_.clear();
}
