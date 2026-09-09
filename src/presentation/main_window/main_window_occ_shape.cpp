// OCC 基本体创建、旋转复合体、网格化显示与形状校验
#include "main_window.h"
#include "application/history/modeling_history_placement.h"
#include "application/history/modeling_history_primitives.h"
#include "geometry/primitives/primitive_geometry.h"
#include "rendering/model/model_display_style.h"
#include "mirror_view_state.h"
#include "shape_presentation_factory.h"

#include <QDateTime>
#include <QMessageBox>
#include <QVTKOpenGLNativeWidget.h>

#include <IVtkTools_ShapeDataSource.hxx>

#include <gp_Dir.hxx>

#include <vtkDataSetMapper.h>
#include <vtkFeatureEdges.h>
#include <vtkMapper.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

void Widget::styleModelBoundaryOutline(int index, bool selectedHighlight)
{
    if (index < 0 || index >= historyList.size()) return;
    ModelingHistory& rec = historyList[index];
    if (!renderStateFor(rec).outlineActor || !ModelDisplayStyle::needsBoundaryOutline(rec.type)) return;

    ModelDisplayStyle::applyBoundaryOutlineStyle(
        renderStateFor(rec).outlineActor->GetProperty(),
        selectedHighlight,
        featureOperationGhostMode_);
}

void Widget::ensureModelBoundaryOutline(int index)
{
    if (!renderer || index < 0 || index >= historyList.size()) return;
    ModelingHistory& rec = historyList[index];
    if (!ModelDisplayStyle::needsBoundaryOutline(rec.type) || !renderStateFor(rec).shapeDataSource) return;
    if (!renderStateFor(rec).outlineActor) {
        vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
        if (renderStateFor(rec).solidDisplayFilter) {
            featureEdges->SetInputConnection(renderStateFor(rec).solidDisplayFilter->GetOutputPort());
        } else {
            featureEdges->SetInputConnection(renderStateFor(rec).shapeDataSource->GetOutputPort());
        }
        featureEdges->BoundaryEdgesOn();
        featureEdges->FeatureEdgesOn();
        featureEdges->ManifoldEdgesOff();
        featureEdges->NonManifoldEdgesOff();
        featureEdges->PassLinesOff();
        // A finer display mesh is used below; keep only genuine sharp edges,
        // rather than exposing tessellation facets as outline teeth.
        featureEdges->SetFeatureAngle(45.0);

        vtkSmartPointer<vtkDataSetMapper> edgeMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        edgeMapper->SetInputConnection(featureEdges->GetOutputPort());
        edgeMapper->ScalarVisibilityOff();
        edgeMapper->SetResolveCoincidentTopologyToPolygonOffset();
        edgeMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1.0, -1.0);

        renderStateFor(rec).outlineActor = vtkSmartPointer<vtkActor>::New();
        renderStateFor(rec).outlineActor->SetMapper(edgeMapper);
        renderStateFor(rec).outlineActor->SetPickable(false);
        renderStateFor(rec).outlineActor->SetVisibility(renderStateFor(rec).actor ? renderStateFor(rec).actor->GetVisibility() : 1);
        renderer->AddActor(renderStateFor(rec).outlineActor);
    }
    styleModelBoundaryOutline(index, index == currentSelectedIndex);
}

void Widget::refreshAllModelBoundaryOutlines()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).outlineActor && renderer) {
            renderer->RemoveActor(renderStateFor(historyList[i]).outlineActor);
            renderStateFor(historyList[i]).outlineActor = nullptr;
        }
        ensureModelBoundaryOutline(i);
    }
}

QString Widget::getTypeName(ModelType type)
{
    switch (type) {
    case CYLINDER: return "圆柱体";
    case CONE: return "圆锥体";
    case SPHERE: return "球体";
    case CUBOID: return "长方体";
    case BOOLEAN_RESULT: return "布尔运算";
    case EXTRUSION: return "拉伸";
    case REVOLUTION: return "旋转";
    case FILLET: return "倒角";
    case HOLLOW: return "挖空";
    case DATUM_PLANE: return "基准平面";
    case DATUM_AXIS: return "基准轴";
    case WORK_CSYS: return "工作坐标系";
    case REFERENCE_CSYS: return "基准坐标系";
    case SKETCH: return "草图";
    case PATTERN: return "阵列特征";
    default: return "未知";
    }
}

TopoDS_Shape Widget::getSelectedOccShape()
{
    if (currentSelectedIndex < 0 || currentSelectedIndex >= historyList.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个模型！");
        return TopoDS_Shape();
    }

    TopoDS_Shape shape = getShapeFromHistory(currentSelectedIndex);
    if (shape.IsNull()) {
        QMessageBox::warning(this, "警告", "该类型的模型不支持此操作！");
    }
    return shape;
}

TopoDS_Shape Widget::getShapeFromHistory(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return TopoDS_Shape();
    }

    const ModelingHistory& record = historyList[index];
    if (!geometryStateFor(record).occShape.IsNull()) {
        return geometryStateFor(record).occShape;
    }

    if (PrimitiveGeometry::isPrimitiveType(record.type)) {
        return PrimitiveGeometry::buildPrimitiveShape(
            ModelingHistoryPrimitives::primitiveRequestFromHistory(record));
    }

    switch (record.type) {
    case BOOLEAN_RESULT:
        return geometryStateFor(record).occShape;
    default:
        return TopoDS_Shape();
    }
}

void Widget::rebindHistoryShapeSource(ModelingHistory& history)
{
    if (!renderStateFor(history).actor) return;
    IVtkTools_ShapeObject::SetShapeSource(renderStateFor(history).shapeDataSource, renderStateFor(history).actor);

    if (history.type == SKETCH && renderStateFor(history).polyData) {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(renderStateFor(history).polyData);
        mapper->ScalarVisibilityOff();
        renderStateFor(history).actor->SetMapper(mapper);
        ModelDisplayStyle::applySketchModelAppearance(
            renderStateFor(history).actor->GetProperty(),
            history.color);
        renderStateFor(history).actor->SetVisibility(1);
    }

    if (renderStateFor(history).profilePickActor
        && renderStateFor(history).profilePickShapeDataSource
        && history.type == SKETCH) {
        IVtkTools_ShapeObject::SetShapeSource(
            renderStateFor(history).profilePickShapeDataSource,
            renderStateFor(history).profilePickActor);
        const bool profilePickable =
            extrusionDialog
            && (currentSelectionMode == ExtrusionSelection
                || currentSelectionMode == EdgeSelection
                || currentSelectionMode == FaceSelection);
        renderStateFor(history).profilePickActor->SetPickable(profilePickable);
        renderStateFor(history).profilePickActor->SetVisibility(
            renderStateFor(history).actor->GetVisibility());
        if (shapePicker) {
            const bool visible = renderStateFor(history).actor->GetVisibility() != 0;
            shapePicker->SetSelectionMode(
                renderStateFor(history).profilePickActor, SM_Face,
                visible && profilePickable);
            shapePicker->SetSelectionMode(
                renderStateFor(history).profilePickActor, SM_Edge,
                visible && profilePickable);
        }
    }
}

// 通用的显示函数
void Widget::displayOccShape(const TopoDS_Shape& shape, const QString& name,
                             ModelType type, const QColor& color,
                             double param1, double param2, double param3,
                             const GeometryPlacement::AxisPlacement& placement)
{
    // 统一在主上下文写入建模结果，避免“只在子窗口出现”的分叉状态。
    QObject* previousMirrorKey = nullptr;
    if (vtkWidget && g_mirrorRenderContextMap.contains(this)) {
        auto& map = g_mirrorRenderContextMap[this];
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it.value().vtkWidget == vtkWidget) {
                previousMirrorKey = it.key();
                break;
            }
        }
    }
    if (previousMirrorKey && renderer) {
        syncMainCameraFromWindowRenderer(renderer);
    }
    activateMainRenderContext();

    if (shape.IsNull()) {
        QMessageBox::warning(dialogParentWidget(), "错误", "创建的形状无效！");
        if (previousMirrorKey) {
            activateMirrorRenderContext(previousMirrorKey);
            if (vtkWidget) vtkWidget->setFocus();
        }
        return;
    }

    ++shapeIDCounter;

    ShapePresentationOptions presentationOptions;
    presentationOptions.color = color;
    presentationOptions.shapeId = shapeIDCounter;
    presentationOptions.meshDeflection = ShapePresentationOptions::kDefaultMeshDeflection;
    presentationOptions.meshAngle = ShapePresentationOptions::kDefaultMeshAngle;
    ModelRenderState renderState =
        ShapePresentationFactory::createSolidModelState(shape, presentationOptions);
    if (!renderState.actor || !renderState.shapeDataSource) {
        QMessageBox::warning(dialogParentWidget(), "错误", "创建模型显示数据失败！");
        if (previousMirrorKey) {
            activateMirrorRenderContext(previousMirrorKey);
            if (vtkWidget) vtkWidget->setFocus();
        }
        return;
    }

    renderer->AddActor(renderState.actor);
    addAppearanceActor(renderState.highlightActor);

    ModelingHistory record;
    record.type = type;
    record.name = name;
    record.timestamp = QDateTime::currentDateTime();
    record.color = color;
    record.param1 = param1;
    record.param2 = param2;
    record.param3 = param3;
    ModelingHistoryPlacement::applyPlacementToHistory(record, placement);

    const int index = modelDocument_.append(record);
    ModelingHistory& storedRecord = historyList[index];
    geometryStateFor(storedRecord).occShape = shape;
    renderStateFor(storedRecord) = renderState;
    ensureModelBoundaryOutline(index);
    updateIntersectionsForRecord(index);
    markDocumentModified(true);

    renderer->ResetCamera();
    refreshCameraClippingRange();
    refreshOverlayScreenScale();
    vtkWidget->renderWindow()->Render();
    updateHistoryList();
    updateFeatureTree();  // 更新特征树

    // 刷新拾取绑定：不重建 shapePicker，按当前主/镜像上下文重绑 actor。
    refreshShapePickerBindingsForCurrentContext();

    // 将主上下文的最新结果同步到所有子窗口，再恢复原交互上下文。
    syncMirrorWindows();
    if (previousMirrorKey) {
        activateMirrorRenderContext(previousMirrorKey);
        if (vtkWidget) vtkWidget->setFocus();
    }
}

// 检查形状是否适合操作
bool Widget::isValidShapeForOperation(int index)
{
    if (index < 0 || index >= historyList.size()) return false;

    // 检查多边形数据是否有效
    vtkPolyData* polyData = renderStateFor(historyList[index]).polyData;
    if (!polyData || polyData->GetNumberOfPoints() == 0) return false;

    return true;
}
