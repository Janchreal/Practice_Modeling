// 全特征级联更新：依赖传播、特征重算、拓扑引用解析
#include "widget.h"
#include "ui_widget.h"
#include "feature_topology.h"
#include "extrusioncommand.h"

#include <QMessageBox>
#include <QSet>
#include <QStatusBar>
#include <QString>
#include <QVTKOpenGLNativeWidget.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <ChFi3d_FilletShape.hxx>
#include <GeomAbs_Shape.hxx>
#include <Precision.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <vtkPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

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
    switch (recipe.baseType) {
    case CUBOID:
        return createCuboidShape(recipe.baseParam1, recipe.baseParam2, recipe.baseParam3,
                                 recipe.baseHasOrigin, recipe.baseOriginX, recipe.baseOriginY, recipe.baseOriginZ,
                                 AxisDirection::Z, false, false, gp_Dir(0, 0, 1));
    case CYLINDER:
        return createCylinderShape(recipe.baseParam1, recipe.baseParam2,
                                   recipe.baseHasOrigin, recipe.baseOriginX, recipe.baseOriginY, recipe.baseOriginZ,
                                   AxisDirection::Z, false, false, gp_Dir(0, 0, 1));
    case CONE:
        return createConeShape(recipe.baseParam1, recipe.baseParam2, recipe.baseParam3,
                               recipe.baseHasOrigin, recipe.baseOriginX, recipe.baseOriginY, recipe.baseOriginZ,
                               AxisDirection::Z, false, false, gp_Dir(0, 0, 1));
    case SPHERE:
        return createSphereShape(recipe.baseParam1,
                                 recipe.baseHasOrigin, recipe.baseOriginX, recipe.baseOriginY, recipe.baseOriginZ);
    default:
        return getShapeFromHistory(recipe.mergeTargetIndex);
    }
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
        record.occShape = newShape;

        const double meshDeflection = (record.type == BOOLEAN_RESULT) ? 0.03 : 0.05;
        BRepMesh_IncrementalMesh mesh(newShape, meshDeflection, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        ++shapeIDCounter;
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(newShape);
        shapeWrapper->SetId(shapeIDCounter);

        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        if (meshPolyData) {
            meshPolyData->SetLines(nullptr);
            meshPolyData->SetVerts(nullptr);
        }

        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDataSource->SetShape(shapeWrapper);
        shapeDataSource->Modified();
        shapeDataSource->Update();

        if (record.actor) {
            vtkSmartPointer<vtkDataSetMapper> newMapper = vtkSmartPointer<vtkDataSetMapper>::New();
            newMapper->SetInputData(meshPolyData);
            configureSolidMapperForBoundaryOutline(newMapper);
            record.actor->SetMapper(newMapper);
            record.solidDisplayFilter = configureSolidShapePipeline(shapeDataSource, newMapper, record.actor);
            record.edgeDisplayFilter = nullptr;
            applySolidActorMaterial(record.actor->GetProperty());
            IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, record.actor);
        }

        if (record.highlightFilter) {
            record.highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
            record.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
        }

        vtkSmartPointer<vtkPolyData> newPolyData = vtkSmartPointer<vtkPolyData>::New();
        newPolyData->ShallowCopy(meshPolyData);
        record.polyData = newPolyData;
        record.shapeWrapper = shapeWrapper;
        record.shapeDataSource = shapeDataSource;
        record.featureRegenerateFailed = false;

        if (shapePicker && renderer) {
            shapePicker->SetRenderer(renderer);
            shapePicker->SetTolerance(0.05);
            prepareShapePickerBindingsForCurrentContext();
            shapeDataSource->Modified();
            shapeDataSource->Update();
        }
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
        QList<ExtrusionFaceSelection> selections;
        for (const SubShapeRef& ref : recipe.profiles) {
            const TopoDS_Shape resolved = resolveSubShapeRef(this, ref);
            if (resolved.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            ExtrusionFaceSelection selection;
            selection.modelIndex = ref.parentIndex;
            selection.subShapeId = ref.subShapeId;
            selection.shape = resolved;
            selection.shapeType = ref.shapeType;
            selections.append(selection);
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
        if (!buildRevolutionCompound(selections, axis, sweepRad, compound, startRad)) {
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
            auto collectEdgeFaces = [](const TopoDS_Shape& shape,
                                         const TopoDS_Edge& edge,
                                         TopoDS_Face& outF1,
                                         TopoDS_Face& outF2) -> bool {
                int found = 0;
                outF1 = TopoDS_Face();
                outF2 = TopoDS_Face();
                for (TopExp_Explorer exF(shape, TopAbs_FACE); exF.More(); exF.Next()) {
                    const TopoDS_Face f = TopoDS::Face(exF.Current());
                    for (TopExp_Explorer exE(f, TopAbs_EDGE); exE.More(); exE.Next()) {
                        const TopoDS_Edge e = TopoDS::Edge(exE.Current());
                        if (e.IsSame(edge)) {
                            if (found == 0) outF1 = f;
                            else if (found == 1) {
                                outF2 = f;
                                return true;
                            }
                            ++found;
                            break;
                        }
                    }
                }
                return false;
            };

            BRepFilletAPI_MakeChamfer chamfer(targetShape);
            for (const SubShapeRef& edgeRef : recipe.edges) {
                const TopoDS_Shape edgeShape = resolveSubShapeRef(this, edgeRef);
                if (edgeShape.IsNull()) {
                    record.featureRegenerateFailed = true;
                    return false;
                }
                const TopoDS_Edge e = TopoDS::Edge(edgeShape);
                if (recipe.isChamferTwoDistances) {
                    TopoDS_Face f1, f2;
                    if (!collectEdgeFaces(targetShape, e, f1, f2) || f1.IsNull()) {
                        record.featureRegenerateFailed = true;
                        return false;
                    }
                    chamfer.Add(recipe.chamferDistance, recipe.chamferDistance2, e, f1);
                } else {
                    chamfer.Add(recipe.chamferDistance, e);
                }
            }
            chamfer.Build();
            if (!chamfer.IsDone()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            return applyShapeToHistory(index, chamfer.Shape());
        }

        BRepFilletAPI_MakeFillet fillet(targetShape);
        const double radius = recipe.usedRadius > 0.0 ? recipe.usedRadius : recipe.radius;
        for (const SubShapeRef& edgeRef : recipe.edges) {
            const TopoDS_Shape edgeShape = resolveSubShapeRef(this, edgeRef);
            if (edgeShape.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            fillet.Add(radius, TopoDS::Edge(edgeShape));
        }
        fillet.Build();
        if (!fillet.IsDone()) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, fillet.Shape());
    }
    case PATTERN: {
        const PatternRecipeData& recipe = record.recipe.pattern;
        TopoDS_Shape resultShape;
        if (recipe.layoutType == PatternLayoutType::Circular) {
            resultShape = buildCircularPatternShape(
                recipe.sourceIndices, recipe.origin, recipe.direction1, recipe.pitch1, recipe.count1,
                recipe.useRadialReplication, recipe.direction2, recipe.pitch2, recipe.count2);
        } else if (recipe.layoutType == PatternLayoutType::Polygonal) {
            resultShape = buildPolygonalPatternShape(
                recipe.sourceIndices, recipe.origin, recipe.direction1, recipe.polygonSpanDegrees, recipe.count1,
                recipe.polygonSpacing, recipe.polygonAlongEdgeCount, recipe.polygonAlongEdgePitch,
                recipe.useRadialReplication, recipe.direction2, recipe.pitch2, recipe.count2);
        } else {
            resultShape = buildLinearPatternShape(
                recipe.sourceIndices, recipe.direction1, recipe.pitch1, recipe.count1,
                recipe.useRadialReplication, recipe.direction2, recipe.pitch2, recipe.count2);
        }
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
            const TopoDS_Shape faceShape = resolveSubShapeRef(this, faceRef);
            if (faceShape.IsNull()) {
                record.featureRegenerateFailed = true;
                return false;
            }
            facesToRemove.Append(faceShape);
        }

        BRepOffsetAPI_MakeThickSolid hollowMaker;
        hollowMaker.MakeThickSolidByJoin(targetShape, facesToRemove, recipe.thickness, 1e-3);
        if (!hollowMaker.IsDone()) {
            record.featureRegenerateFailed = true;
            return false;
        }
        return applyShapeToHistory(index, hollowMaker.Shape());
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

        if (!shapesSatisfyBooleanOverlap(targetShape, toolShapes, operationType)) {
            booleanResult.featureRegenerateFailed = true;
            return;
        }

        TopoDS_Shape resultShape;
        if (!executeOccBooleanMulti(targetShape, toolShapes, operationType, resultShape) || resultShape.IsNull()) {
            booleanResult.featureRegenerateFailed = true;
            return;
        }

        const QString operationName = booleanOperationName(operationType);
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
