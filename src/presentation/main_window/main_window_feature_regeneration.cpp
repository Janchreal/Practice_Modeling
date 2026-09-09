// 全特征级联更新：依赖传播、特征重算、拓扑引用解析
#include "main_window.h"
#include "application/history/modeling_history_primitives.h"
#include "ui_main_window.h"
#include "geometry/boolean/boolean_ops.h"
#include "geometry/topology/feature_topology.h"
#include "geometry/primitives/primitive_geometry.h"
#include "geometry/pattern/pattern_geometry.h"
#include "geometry/revolution/revolve_geometry.h"
#include "geometry/modification/feature_modification_geometry.h"
#include "application/commands/extrusioncommand.h"
#include "rendering/model/shape_presentation_factory.h"

#include <QMessageBox>
#include <QSet>
#include <QStatusBar>
#include <QString>
#include <QVTKOpenGLNativeWidget.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <Standard_Failure.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>

#include <vtkRenderWindow.h>

bool Widget::featureDependsOnModel(int featureIndex, int modelIndex) const
{
    if (featureIndex < 0 || featureIndex >= historyList.size() || modelIndex < 0) {
        return false;
    }

    const ModelingHistory& record = historyList[featureIndex];
    if (record.recipe.hasRecipe && record.recipe.parentIndices.contains(modelIndex)) {
        return true;
    }

    if (record.type == BOOLEAN_RESULT) {
        if (record.booleanTargetIndex == modelIndex) {
            return true;
        }
        return record.booleanToolIndices.contains(modelIndex);
    }

    return false;
}

QList<int> Widget::collectDependentFeatureIndices(int parentIndex) const
{
    QSet<int> dependents;
    QList<int> queue;
    queue.append(parentIndex);
    QSet<int> visitedSources;
    visitedSources.insert(parentIndex);

    while (!queue.isEmpty()) {
        const int sourceIndex = queue.takeFirst();
        for (int i = 0; i < historyList.size(); ++i) {
            if (i == sourceIndex) {
                continue;
            }
            if (!featureDependsOnModel(i, sourceIndex)) {
                continue;
            }
            if (!dependents.contains(i)) {
                dependents.insert(i);
                if (!visitedSources.contains(i)) {
                    visitedSources.insert(i);
                    queue.append(i);
                }
            }
        }
    }

    QList<int> ordered = dependents.values();
    std::sort(ordered.begin(), ordered.end());
    return ordered;
}

int Widget::primaryParentIndex(const ModelingHistory& record) const
{
    if (record.recipe.hasRecipe && !record.recipe.parentIndices.isEmpty()) {
        return record.recipe.parentIndices.first();
    }
    if (record.type == SKETCH && record.recipe.sketch.datumPlaneIndex >= 0) {
        return record.recipe.sketch.datumPlaneIndex;
    }
    if (record.type == HOLLOW && record.recipe.hollow.targetIndex >= 0) {
        return record.recipe.hollow.targetIndex;
    }
    if (record.type == PATTERN && !record.recipe.pattern.sourceIndices.isEmpty()) {
        return record.recipe.pattern.sourceIndices.first();
    }
    if (record.type == BOOLEAN_RESULT && record.booleanTargetIndex >= 0) {
        return record.booleanTargetIndex;
    }
    return -1;
}

TopoDS_Shape Widget::rebuildBaseShapeFromExtrusionRecipe(const ExtrusionRecipeData& recipe)
{
    if (PrimitiveGeometry::isPrimitiveType(recipe.baseType)) {
        return PrimitiveGeometry::buildPrimitiveShape(
            ModelingHistoryPrimitives::primitiveRequestFromExtrusionBase(recipe));
    }
    return getShapeFromHistory(recipe.mergeTargetIndex);
}

void Widget::assignFeatureRecipe(int index, const FeatureRecipe& recipe)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }

    ModelingHistory& record = historyList[index];
    record.recipe = recipe;
    record.recipe.hasRecipe = true;
    record.featureRegenerateFailed = false;

    if (record.type == BOOLEAN_RESULT) {
        record.booleanTargetIndex = recipe.boolean.targetIndex;
        record.booleanToolIndices = recipe.boolean.toolIndices;
        record.booleanOperationType = recipe.boolean.operationType;
        record.booleanKeepTarget = recipe.boolean.keepTarget;
        record.booleanKeepTool = recipe.boolean.keepTool;
    }
}

bool Widget::applyShapeToHistory(int index, const TopoDS_Shape& newShape, const QString& newName)
{
    if (index < 0 || index >= historyList.size() || newShape.IsNull()) {
        return false;
    }

    ModelingHistory& record = historyList[index];
    if (!newName.isEmpty()) {
        record.name = newName;
    }

    try {
        geometryStateFor(record).occShape = newShape;

        ++shapeIDCounter;
        ModelRenderState& renderState = renderStateFor(record);
        const bool hadActor = renderState.actor != nullptr;
        const bool hadHighlightActor = renderState.highlightActor != nullptr;
        ShapePresentationOptions renderOptions;
        renderOptions.color = record.color;
        renderOptions.shapeId = shapeIDCounter;
        renderOptions.meshDeflection = ShapePresentationOptions::kDefaultMeshDeflection;
        renderOptions.meshAngle = ShapePresentationOptions::kDefaultMeshAngle;
        ShapePresentationFactory::refreshSolidModelState(renderState, newShape, renderOptions);
        if (!renderState.actor || !renderState.shapeDataSource) {
            record.featureRegenerateFailed = true;
            return false;
        }
        if (!hadActor && renderer) {
            renderer->AddActor(renderState.actor);
        }
        if (!hadHighlightActor) {
            addAppearanceActor(renderState.highlightActor);
        }

        // refreshSolidModelState replaces the shaded pipeline. Rebuild the
        // outline too; otherwise it keeps the previous mesh and can protrude
        // beyond the newly regenerated cone/sphere boundary.
        if (renderState.outlineActor && renderer) {
            renderer->RemoveActor(renderState.outlineActor);
            renderState.outlineActor = nullptr;
        }
        ensureModelBoundaryOutline(index);
        record.featureRegenerateFailed = false;

        refreshShapePickerBindingsForCurrentContext();
        updateIntersectionsForRecord(index);
        return true;
    } catch (Standard_Failure&) {
        record.featureRegenerateFailed = true;
        return false;
    }
}

void Widget::updateDependentFeatures(int modelIndex)
{
    if (cascadeUpdateGuard_ > 0) {
        return;
    }

    if (modelIndex < 0 || modelIndex >= historyList.size()) {
        return;
    }

    QSet<int> affectedIndices;
    QList<int> queue;
    queue.append(modelIndex);
    QSet<int> visitedSources;
    visitedSources.insert(modelIndex);

    while (!queue.isEmpty()) {
        const int sourceIndex = queue.takeFirst();
        for (int i = 0; i < historyList.size(); ++i) {
            if (i == sourceIndex) {
                continue;
            }
            if (!featureDependsOnModel(i, sourceIndex)) {
                continue;
            }
            if (!affectedIndices.contains(i)) {
                affectedIndices.insert(i);
                if (!visitedSources.contains(i)) {
                    visitedSources.insert(i);
                    queue.append(i);
                }
            }
        }
    }

    QList<int> orderedIndices = affectedIndices.values();
    std::sort(orderedIndices.begin(), orderedIndices.end());
    QStringList failedNames;
    for (int featureIndex : orderedIndices) {
        regenerateFeature(featureIndex);
        if (historyList[featureIndex].featureRegenerateFailed) {
            failedNames.append(historyList[featureIndex].name);
        }
    }

    if (!orderedIndices.isEmpty()) {
        updateHistoryList();
        updateFeatureTree();
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        if (!failedNames.isEmpty() && statusBar()) {
            statusBar()->showMessage(
                tr("以下特征更新失败：%1").arg(failedNames.join(QStringLiteral("、"))),
                8000);
        }
    }
}

void Widget::updateDependentBooleanResults(int modelIndex)
{
    updateDependentFeatures(modelIndex);
}

bool Widget::regenerateFeature(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return false;
    }

    ModelingHistory& record = historyList[index];
    if (!record.recipe.hasRecipe) {
        switch (record.type) {
        case CUBOID:
        case CYLINDER:
        case CONE:
        case SPHERE:
            regenerateModel(index);
            return true;
        case BOOLEAN_RESULT:
            regenerateBooleanResult(index);
            return true;
        default:
            return false;
        }
    }

    switch (record.type) {
    case BOOLEAN_RESULT:
        regenerateBooleanResult(index);
        return !record.featureRegenerateFailed;
    case EXTRUSION: {
        TopoDS_Shape resultShape;
        if (!ExtrusionCommand::extrudeFromRecipe(this, record.recipe.extrusion, resultShape)) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, resultShape);
    }
    case REVOLUTION: {
        const RevolveRecipeData& recipe = record.recipe.revolve;
        QList<TopoDS_Shape> profiles;
        for (const SubShapeRef& ref : recipe.profiles) {
            const TopoDS_Shape resolved = resolveSubShapeRef(getShapeFromHistory(ref.parentIndex), ref);
            if (resolved.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            profiles.append(resolved);
        }

        const gp_Ax1 axis(recipe.axisOrigin, recipe.axisDir);
        double sweepDeg = recipe.endAngleDeg - recipe.startAngleDeg;
        if (std::abs(sweepDeg) < 1e-9 && std::abs(recipe.angleDeg) > 1e-9) {
            sweepDeg = recipe.angleDeg;
        }
        while (sweepDeg > 360.0) sweepDeg -= 360.0;
        while (sweepDeg < -360.0) sweepDeg += 360.0;
        const double startRad = recipe.startAngleDeg * M_PI / 180.0;
        const double sweepRad = sweepDeg * M_PI / 180.0;
        TopoDS_Compound compound;
        if (!buildRevolutionCompound(profiles, axis, sweepRad, compound, startRad)) {
            record.featureRegenerateFailed = true;
            return false;
        }

        TopoDS_Shape resultShape = compound;
        if (recipe.boolOpType >= 0) {
            if (!applyDialogBooleanToShape(recipe.boolOpType, recipe.boolTargetIndex,
                                          compound, resultShape)
                || resultShape.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
        }
        return applyShapeToHistory(index, resultShape);
    }
    case FILLET: {
        const FilletRecipeData& recipe = record.recipe.fillet;
        if (recipe.targetIndex < 0 || recipe.targetIndex >= historyList.size()) {
            record.featureRegenerateFailed = true;
            return false;
        }

        const TopoDS_Shape targetShape = getShapeFromHistory(recipe.targetIndex);
        if (targetShape.IsNull()) {
            record.featureRegenerateFailed = true;
            return false;
        }

        if (recipe.isChamfer) {
            QList<TopoDS_Edge> edgeShapes;
            for (const SubShapeRef& edgeRef : recipe.edges) {
                const TopoDS_Shape edgeShape = resolveSubShapeRef(
                    getShapeFromHistory(edgeRef.parentIndex), edgeRef);
                if (edgeShape.IsNull()) {
                    record.featureRegenerateFailed = true;
                    return false;
                }
                edgeShapes.append(TopoDS::Edge(edgeShape));
            }
            TopoDS_Shape chamferShape;
            if (!FeatureModificationGeometry::buildChamferShape(
                    targetShape, edgeShapes, recipe.chamferDistance, recipe.chamferDistance2,
                    recipe.isChamferTwoDistances, chamferShape)) {
                record.featureRegenerateFailed = true;
                return false;
            }
            return applyShapeToHistory(index, chamferShape);
        }

        const double radius = recipe.usedRadius > 0.0 ? recipe.usedRadius : recipe.radius;
        QList<TopoDS_Edge> edgeShapes;
        for (const SubShapeRef& edgeRef : recipe.edges) {
            const TopoDS_Shape edgeShape = resolveSubShapeRef(
                getShapeFromHistory(edgeRef.parentIndex), edgeRef);
            if (edgeShape.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            edgeShapes.append(TopoDS::Edge(edgeShape));
        }
        TopoDS_Shape filletShape;
        if (!FeatureModificationGeometry::buildFilletShape(
                targetShape, edgeShapes, radius, recipe.isG2, recipe.rho, filletShape, nullptr)) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, filletShape);
    }
    case PATTERN: {
        const PatternRecipeData& recipe = record.recipe.pattern;
        QList<TopoDS_Shape> sourceShapes;
        for (int sourceIndex : recipe.sourceIndices) {
            const TopoDS_Shape src = getShapeFromHistory(sourceIndex);
            if (!src.IsNull()) {
                sourceShapes.append(src);
            }
        }
        const TopoDS_Shape resultShape = PatternGeometry::buildPatternShape(
            sourceShapes, recipe.layoutType, recipe.origin, recipe.direction1, recipe.pitch1, recipe.count1,
            recipe.useRadialReplication, recipe.direction2, recipe.pitch2, recipe.count2,
            recipe.polygonSpanDegrees, recipe.polygonSpacing, recipe.polygonAlongEdgeCount,
            recipe.polygonAlongEdgePitch);
        if (resultShape.IsNull()) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, resultShape);
    }
    case HOLLOW: {
        const HollowRecipeData& recipe = record.recipe.hollow;
        if (recipe.targetIndex < 0 || recipe.targetIndex >= historyList.size() || recipe.faces.isEmpty()) {
            record.featureRegenerateFailed = true;
            return false;
        }

        const TopoDS_Shape targetShape = getShapeFromHistory(recipe.targetIndex);
        if (targetShape.IsNull()) {
            record.featureRegenerateFailed = true;
            return false;
        }

        TopTools_ListOfShape facesToRemove;
        for (const SubShapeRef& faceRef : recipe.faces) {
            const TopoDS_Shape faceShape = resolveSubShapeRef(
                getShapeFromHistory(faceRef.parentIndex), faceRef);
            if (faceShape.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            facesToRemove.Append(faceShape);
        }

        TopoDS_Shape hollowShape;
        if (!FeatureModificationGeometry::buildHollowShape(
                targetShape, facesToRemove, recipe.thickness, hollowShape)) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, hollowShape);
    }
    case CUBOID:
    case CYLINDER:
    case CONE:
    case SPHERE:
        regenerateModel(index);
        return true;
    default:
        return false;
    }
}

void Widget::regenerateBooleanResult(int booleanResultIndex)
{
    if (booleanResultIndex < 0 || booleanResultIndex >= historyList.size()) {
        return;
    }

    ModelingHistory& booleanResult = historyList[booleanResultIndex];
    if (booleanResult.type != BOOLEAN_RESULT) {
        return;
    }

    const int targetIndex = booleanResult.recipe.hasRecipe
        ? booleanResult.recipe.boolean.targetIndex
        : booleanResult.booleanTargetIndex;
    const QList<int> toolIndices = booleanResult.recipe.hasRecipe
        ? booleanResult.recipe.boolean.toolIndices
        : booleanResult.booleanToolIndices;
    const int operationType = booleanResult.recipe.hasRecipe
        ? booleanResult.recipe.boolean.operationType
        : booleanResult.booleanOperationType;

    if (targetIndex < 0 || targetIndex >= historyList.size()
        || toolIndices.isEmpty() || operationType < 0 || operationType > 2) {
        booleanResult.featureRegenerateFailed = true;
        return;
    }

    try {
        const TopoDS_Shape targetShape = getShapeFromHistory(targetIndex);
        if (targetShape.IsNull()) {
            booleanResult.featureRegenerateFailed = true;
            return;
        }

        QList<TopoDS_Shape> toolShapes;
        QStringList toolNames;
        for (int toolIndex : toolIndices) {
            if (toolIndex < 0 || toolIndex >= historyList.size()) {
                booleanResult.featureRegenerateFailed = true;
                return;
            }
            const TopoDS_Shape toolShape = getShapeFromHistory(toolIndex);
            if (toolShape.IsNull()) {
                booleanResult.featureRegenerateFailed = true;
                return;
            }
            toolShapes.append(toolShape);
            toolNames << historyList[toolIndex].name;
        }

        if (!BooleanOps::shapesSatisfyBooleanOverlap(targetShape, toolShapes, operationType)) {
            booleanResult.featureRegenerateFailed = true;
            return;
        }

        TopoDS_Shape resultShape;
        if (!BooleanOps::executeOccBooleanMulti(targetShape, toolShapes, operationType, resultShape) || resultShape.IsNull()) {
            booleanResult.featureRegenerateFailed = true;
            return;
        }

        const QString operationName = BooleanOps::booleanOperationName(operationType);
        const QString targetName = historyList[targetIndex].name;
        const QString resultName = QStringLiteral("%1(%2,%3)")
                                       .arg(operationName, targetName, toolNames.join(QStringLiteral("+")));

        if (!applyShapeToHistory(booleanResultIndex, resultShape, resultName)) {
            booleanResult.featureRegenerateFailed = true;
        }
    } catch (Standard_Failure&) {
        booleanResult.featureRegenerateFailed = true;
    }
}
