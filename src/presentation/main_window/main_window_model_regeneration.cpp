// 表达式、参数更新、模型形状更新与布尔结果再生（从 main_window.cpp 拆出）
#include "main_window.h"
#include "application/history/modeling_history_primitives.h"
#include "rendering/model/model_display_style.h"
#include "primitive_geometry.h"
#include "ui_main_window.h"
#include "expression_dialog.h"
#include "regeneratemodelcommand.h"
#include "shape_presentation_factory.h"

#include <QMessageBox>
#include <QSet>
#include <QString>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <algorithm>

#include <Standard_Failure.hxx>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

// 表达式按钮点击事件
void Widget::on_expressionBtn_clicked()
{
    ExpressionDialog *dialog = new ExpressionDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}
// 更新模型参数
void Widget::updateModelParameter(const QString& paramName, double newValue)
{
    // 根据参数名称找到对应的模型索引
    int modelIndex = findModelIndexByParamName(paramName);
    if (modelIndex == -1) {
        QMessageBox::warning(this, "错误", QString("未找到参数 %1 对应的模型").arg(paramName));
        return;
    }

    // 更新模型参数
    ModelingHistory& history = historyList[modelIndex];

    double newParam1 = history.param1;
    double newParam2 = history.param2;
    double newParam3 = history.param3;

    // 解析参数名称，确定要更新哪个参数
    if (paramName.endsWith("_x") && history.type == CUBOID) {
        newParam1 = newValue; // 长方体长度
    } else if (paramName.endsWith("_y") && history.type == CUBOID) {
        newParam2 = newValue; // 长方体宽度
    } else if (paramName.endsWith("_z") && history.type == CUBOID) {
        newParam3 = newValue; // 长方体高度
    } else if (paramName.endsWith("_r") && (history.type == CYLINDER || history.type == CONE || history.type == SPHERE)) {
        newParam1 = newValue; // 半径
    } else if (paramName.endsWith("_h") && (history.type == CYLINDER || history.type == CONE)) {
        newParam2 = newValue; // 高度
    } else {
        QMessageBox::warning(this, "错误", QString("参数 %1 不匹配模型类型").arg(paramName));
        return;
    }

    executeCommand(new RegenerateModelCommand(
        this, modelIndex, newParam1, newParam2, newParam3,
        QString("修改%1").arg(history.name)));
}

// 根据参数名称查找模型索引
int Widget::findModelIndexByParamName(const QString& paramName)
{
    // 从参数名称中提取模型名称
    QString modelName;
    if (paramName.endsWith("_x") || paramName.endsWith("_y") || paramName.endsWith("_z") ||
        paramName.endsWith("_r") || paramName.endsWith("_h")) {
        modelName = paramName.left(paramName.length() - 2);
    } else {
        return -1;
    }

    // 在历史记录中查找模型
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].name == modelName) {
            return i;
        }
    }

    return -1;
}

// 重新生成模型
void Widget::regenerateModel(int index, bool triggerCascade)
{
    if (index < 0 || index >= historyList.size()) return;

    ModelingHistory& history = historyList[index];

    try {
        // 根据模型类型重新创建形状
        if (!PrimitiveGeometry::isPrimitiveType(history.type)) {
            switch (history.type) {
            case BOOLEAN_RESULT:
            case EXTRUSION:
            case REVOLUTION:
            case FILLET:
            case PATTERN:
            case HOLLOW:
                if (history.recipe.hasRecipe) {
                    regenerateFeature(index);
                }
                return;
            default:
                return;
            }
        }

        const TopoDS_Shape newShape = PrimitiveGeometry::buildPrimitiveShape(
            ModelingHistoryPrimitives::primitiveRequestFromHistory(history));

        if (newShape.IsNull()) {
            QMessageBox::warning(this, "错误", "形状创建失败！");
            return;
        }

        // 更新形状
        geometryStateFor(history).occShape = newShape;

        ++shapeIDCounter;
        ModelRenderState& renderState = renderStateFor(history);
        const bool hadActor = renderState.actor != nullptr;
        const bool hadHighlightActor = renderState.highlightActor != nullptr;
        ShapePresentationOptions renderOptions;
        renderOptions.color = history.color;
        renderOptions.shapeId = shapeIDCounter;
        ShapePresentationFactory::refreshSolidModelState(renderState, newShape, renderOptions);
        if (!renderState.actor || !renderState.shapeDataSource) {
            QMessageBox::warning(this, "错误", "模型显示数据更新失败！");
            return;
        }
        if (!hadActor && renderer) {
            renderer->AddActor(renderState.actor);
        }
        if (!hadHighlightActor) {
            addAppearanceActor(renderState.highlightActor);
        }

        // 几何变更后重建黑色轮廓
        if (renderStateFor(history).outlineActor) {
            renderer->RemoveActor(renderStateFor(history).outlineActor);
            renderStateFor(history).outlineActor = nullptr;
        }
        ensureModelBoundaryOutline(index);

        // 重新渲染
        vtkWidget->renderWindow()->Render();

        // 关键：更新历史列表显示，确保显示新的参数值
        updateHistoryList();

        refreshShapePickerBindingsForCurrentContext();
        updateIntersectionsForRecord(index);

        // 更新所有依赖于该模型的下游特征
        if (triggerCascade) {
            updateDependentFeatures(index);
        }
    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("模型重新生成失败: %1").arg(e.GetMessageString()));
    }
}

// 更新模型形状（用于拉伸等操作，将结果合并到现有模型）
void Widget::updateModelShape(int index, const TopoDS_Shape& newShape, bool triggerCascade)
{
    if (index < 0 || index >= historyList.size()) return;
    if (newShape.IsNull()) {
        QMessageBox::warning(this, "错误", "新形状无效！");
        return;
    }

    ModelingHistory& history = historyList[index];

    try {
        // 更新形状和类型
        geometryStateFor(history).occShape = newShape;
        history.type = EXTRUSION;  // 标记为拉伸操作的结果

        ++shapeIDCounter;
        ModelRenderState& renderState = renderStateFor(history);
        const bool hadActor = renderState.actor != nullptr;
        const bool hadHighlightActor = renderState.highlightActor != nullptr;
        ShapePresentationOptions renderOptions;
        renderOptions.color = history.color;
        renderOptions.shapeId = shapeIDCounter;
        renderOptions.deepCopyPolyData = true;
        ShapePresentationFactory::refreshSolidModelState(renderState, newShape, renderOptions);
        if (!renderState.actor || !renderState.shapeDataSource) {
            QMessageBox::warning(this, "错误", "模型显示数据更新失败！");
            return;
        }
        if (!hadActor && renderer) {
            renderer->AddActor(renderState.actor);
        }
        if (!hadHighlightActor) {
            addAppearanceActor(renderState.highlightActor);
        }

        // 清除旧的轮廓线，确保使用新的 PolyData 重新生成
        if (renderStateFor(history).outlineActor) {
            renderer->RemoveActor(renderStateFor(history).outlineActor);
            renderStateFor(history).outlineActor = nullptr;
        }
        ensureModelBoundaryOutline(index);

        // 如果当前模型被选中，重新生成轮廓线以反映合并后的形状
        if (currentSelectedIndex == index) {
            highlightModel(index);
        }

        // 重新渲染
        vtkWidget->renderWindow()->Render();

        // 更新历史列表显示
        updateHistoryList();

        refreshShapePickerBindingsForCurrentContext();
        updateIntersectionsForRecord(index);

        // 更新所有依赖于该模型的下游特征
        if (triggerCascade) {
            updateDependentFeatures(index);
        }
    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("模型更新失败: %1").arg(e.GetMessageString()));
    }
}

void Widget::setModelVisibility(int index, bool visible)
{
    if (index < 0 || index >= historyList.size()) return;

    // 依赖源已删除且没有可重建几何时，禁止通过特征树重新显示陈旧的旧 actor。
    if (visible && historyList[index].featureRegenerateFailed
        && geometryStateFor(historyList[index]).occShape.IsNull()) {
        visible = false;
        if (statusBar()) {
            statusBar()->showMessage(
                tr("该特征的依赖已失效，无法显示旧几何。"), 3000);
        }
    }

    if (historyList[index].type == WORK_CSYS) {
        setWorkCsysVisible(visible);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        syncMirrorWindows();
        return;
    }
    if (historyList[index].type == REFERENCE_CSYS) {
        setReferenceCsysVisible(visible);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        syncMirrorWindows();
        return;
    }

    if (renderStateFor(historyList[index]).actor) {
        renderStateFor(historyList[index]).actor->SetVisibility(visible ? 1 : 0);
        renderStateFor(historyList[index]).actor->SetPickable(
            ModelDisplayStyle::isInteractiveModelType(historyList[index].type) ? visible : false);

        if (shapePicker && ModelDisplayStyle::isInteractiveModelType(historyList[index].type)) {
            shapePicker->SetSelectionMode(renderStateFor(historyList[index]).actor, SM_Face, visible);
            shapePicker->SetSelectionMode(renderStateFor(historyList[index]).actor, SM_Edge, visible);
            shapePicker->SetSelectionMode(renderStateFor(historyList[index]).actor, SM_Vertex, visible);
        }

        if (renderStateFor(historyList[index]).outlineActor) {
            renderStateFor(historyList[index]).outlineActor->SetVisibility(visible ? 1 : 0);
        }
        if (!visible && renderStateFor(historyList[index]).highlightActor) {
            renderStateFor(historyList[index]).highlightActor->SetVisibility(0);
        }

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
    if (renderStateFor(historyList[index]).profilePickActor) {
        const bool profilePickable =
            extrusionDialog
            && (currentSelectionMode == ExtrusionSelection
                || currentSelectionMode == EdgeSelection
                || currentSelectionMode == FaceSelection);
        const bool profileVisible = visible && historyList[index].type == SKETCH;
        renderStateFor(historyList[index]).profilePickActor->SetVisibility(profileVisible ? 1 : 0);
        renderStateFor(historyList[index]).profilePickActor->SetPickable(
            profileVisible && profilePickable);
        if (profileVisible && profilePickable) {
            IVtkTools_ShapeObject::SetShapeSource(
                renderStateFor(historyList[index]).profilePickShapeDataSource,
                renderStateFor(historyList[index]).profilePickActor);
        } else {
            IVtkTools_ShapeObject::SetShapeSource(
                nullptr, renderStateFor(historyList[index]).profilePickActor);
        }
    }
    updateIntersectionsForRecord(index);
    syncMirrorWindows();
}

void Widget::setModelVisibleForCommand(int index, bool visible)
{
    setModelVisibility(index, visible);
    updateFeatureTree();
}

bool Widget::isModelVisibleForCommand(int index) const
{
    if (index < 0 || index >= historyList.size()) return true;
    if (!renderStateFor(historyList[index]).actor) return true;
    return renderStateFor(historyList[index]).actor->GetVisibility() != 0;
}

void Widget::updateModelShapeWithTypeInternal(int index, const TopoDS_Shape& newShape, ModelType newType, bool triggerCascade)
{
    if (index < 0 || index >= historyList.size()) return;
    if (newShape.IsNull()) return;

    ModelingHistory& history = historyList[index];

    if (renderStateFor(history).profilePickActor) {
        removeSceneActor(renderStateFor(history).profilePickActor);
        renderStateFor(history).profilePickActor = nullptr;
        renderStateFor(history).profilePickShapeWrapper = nullptr;
        renderStateFor(history).profilePickShapeDataSource = nullptr;
    }

    geometryStateFor(history).occShape = newShape;
    history.type = newType;

    ++shapeIDCounter;
    ModelRenderState& renderState = renderStateFor(history);
    const bool hadActor = renderState.actor != nullptr;
    const bool hadHighlightActor = renderState.highlightActor != nullptr;
    ShapePresentationOptions renderOptions;
    renderOptions.color = history.color;
    renderOptions.shapeId = shapeIDCounter;
    renderOptions.deepCopyPolyData = true;
    ShapePresentationFactory::refreshSolidModelState(renderState, newShape, renderOptions);
    if (!renderState.actor || !renderState.shapeDataSource) {
        return;
    }
    if (!hadActor && renderer) {
        renderer->AddActor(renderState.actor);
    }
    if (!hadHighlightActor) {
        addAppearanceActor(renderState.highlightActor);
    }

    if (renderStateFor(history).outlineActor && renderer) {
        renderer->RemoveActor(renderStateFor(history).outlineActor);
        renderStateFor(history).outlineActor = nullptr;
    }
    ensureModelBoundaryOutline(index);

    if (currentSelectedIndex == index) {
        highlightModel(index);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }

    updateHistoryList();

    refreshShapePickerBindingsForCurrentContext();
    updateIntersectionsForRecord(index);

    if (triggerCascade) {
        updateDependentFeatures(index);
    }
}

void Widget::setModelParametersForCommand(int index, double param1, double param2, double param3)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }

    historyList[index].param1 = param1;
    historyList[index].param2 = param2;
    historyList[index].param3 = param3;
    updateModelName(index);
}

void Widget::regenerateModelForCommand(int index, bool triggerCascade)
{
    regenerateModel(index, triggerCascade);
}

void Widget::applyModelStateForCommand(int index, const TopoDS_Shape& shape, ModelType type, const QString& name, bool triggerCascade)
{
    if (index < 0 || index >= historyList.size()) return;
    if (shape.IsNull()) return;

    historyList[index].name = name;
    updateModelShapeWithTypeInternal(index, shape, type, triggerCascade);
    updateFeatureTree();
    markDocumentModified(true);
}

void Widget::updateModelName(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    ModelingHistory& history = historyList[index];

    // 根据模型类型和参数更新名称
    switch (history.type) {
    case CUBOID:
        history.name = QString("长方体(l=%1,w=%2,h=%3)").arg(history.param1).arg(history.param2).arg(history.param3);
        break;
    case CYLINDER:
        history.name = QString("圆柱体(r=%1,h=%2)").arg(history.param1).arg(history.param2);
        break;
    case CONE:
        history.name = QString("圆锥体(r=%1,h=%2)").arg(history.param1).arg(history.param2);
        break;
    case SPHERE:
        history.name = QString("球体(r=%1)").arg(history.param1);
        break;
    case EXTRUSION:
        history.name = QString("拉伸(距离=%1)").arg(history.param1);
        break;
    case REVOLUTION:
        history.name = QString("旋转(角度=%1)").arg(history.param1);
        break;
    case FILLET:
        history.name = QString("倒角(半径=%1)").arg(history.param1);
        break;
    case HOLLOW:
        history.name = QString("挖空(厚度=%1)").arg(history.param1);
        break;
    case DATUM_PLANE:
    case DATUM_AXIS:
        // 基准平面/轴名称由创建时决定，这里不根据参数改写
        break;
    // 对于布尔运算和其他复杂操作，保持原有名称
    default:
        break;
    }
}
