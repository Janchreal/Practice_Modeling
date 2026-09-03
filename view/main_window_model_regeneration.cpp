// 表达式、参数更新、模型形状更新与布尔结果再生（从 main_window.cpp 拆出）
#include "main_window.h"
#include "primitive_geometry.h"
#include "ui_main_window.h"
#include "expression_dialog.h"
#include "regeneratemodelcommand.h"

#include <QMessageBox>
#include <QSet>
#include <QString>
#include <QVTKOpenGLNativeWidget.h>

#include <algorithm>

#include <Standard_Failure.hxx>

#include <BRepMesh_IncrementalMesh.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <vtkPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

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
        TopoDS_Shape newShape;

        // 根据模型类型重新创建形状
        switch (history.type) {
        case CUBOID:
            newShape = PrimitiveGeometry::buildCuboidShape(history.param1, history.param2, history.param3,
                                                           history.hasOrigin, history.originX, history.originY, history.originZ,
                                                           history.axisDirection,
                                                           history.axisReversed,
                                                           history.hasCustomVectorDir,
                                                           history.customVectorDir);
            break;
        case CYLINDER:
            newShape = PrimitiveGeometry::buildCylinderShape(history.param1, history.param2,
                                                             history.hasOrigin, history.originX, history.originY, history.originZ,
                                                             history.axisDirection,
                                                             history.axisReversed,
                                                             history.hasCustomVectorDir,
                                                             history.customVectorDir);
            break;
        case CONE:
            newShape = PrimitiveGeometry::buildConeShape(history.param1, history.param2, history.param3,
                                                         history.hasOrigin, history.originX, history.originY, history.originZ,
                                                         history.axisDirection,
                                                         history.axisReversed,
                                                         history.hasCustomVectorDir,
                                                         history.customVectorDir);
            break;
        case SPHERE:
            newShape = PrimitiveGeometry::buildSphereShape(history.param1,
                                                           history.hasOrigin, history.originX, history.originY, history.originZ);
            break;
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

        if (newShape.IsNull()) {
            QMessageBox::warning(this, "错误", "形状创建失败！");
            return;
        }

        // 更新形状
        history.occShape = newShape;

        // 先对OCC形状进行网格离散化（提高质量）
        BRepMesh_IncrementalMesh mesh(newShape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        // 使用 VIS 重新生成数据源
        ++shapeIDCounter;
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(newShape);
        shapeWrapper->SetId(shapeIDCounter);

        // 手动构建 PolyData（OCCT 7.7.0 需要）
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        
        // 清理线条和顶点数据，只保留多边形（面）
        if (meshPolyData) {
            meshPolyData->SetLines(nullptr);  // 移除线条
            meshPolyData->SetVerts(nullptr);  // 移除顶点
        }

        // 创建 ShapeDataSource（仅用于拾取）
        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDataSource->SetShape(shapeWrapper);
        // OCCT 7.7.0 需要显式更新才能让拾取器识别
        shapeDataSource->Modified();
        shapeDataSource->Update();

        // 更新actor的映射器
        if (history.actor) {
            vtkSmartPointer<vtkDataSetMapper> newMapper = vtkSmartPointer<vtkDataSetMapper>::New();
            newMapper->SetInputData(meshPolyData);  // 使用手动构建的数据
            // IVtkTools::InitShapeMapper(newMapper);  // 注释掉，避免边缘显示
            configureSolidMapperForBoundaryOutline(newMapper);
            history.actor->SetMapper(newMapper);
            history.solidDisplayFilter = configureSolidShapePipeline(shapeDataSource, newMapper, history.actor);
            history.edgeDisplayFilter = nullptr;

            // 确保关闭边缘显示
            applySolidActorMaterial(history.actor->GetProperty());

            // 重新绑定数据源到 actor
            IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, history.actor);
        }
        
        // 更新高亮过滤器（接 ShapeDataSource 输出，与 vis_picker_example 一致）
        if (history.highlightFilter) {
            history.highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
            history.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
        }

        // 保存 PolyData
        vtkSmartPointer<vtkPolyData> newPolyData = vtkSmartPointer<vtkPolyData>::New();
        newPolyData->ShallowCopy(meshPolyData);
        history.polyData = newPolyData;

        // 保存引用 - 关键：同时保存Handle和数据源
        history.shapeWrapper = shapeWrapper;
        history.shapeDataSource = shapeDataSource;

        // 几何变更后重建黑色轮廓
        if (history.outlineActor) {
            renderer->RemoveActor(history.outlineActor);
            history.outlineActor = nullptr;
        }
        history.outlinePolyData = nullptr;
        history.outlineSourcePolyData = nullptr;
        ensureModelBoundaryOutline(index);

        // 重新渲染
        vtkWidget->renderWindow()->Render();

        // 关键：更新历史列表显示，确保显示新的参数值
        updateHistoryList();

        // 重新生成模型后，更新所有 ShapeDataSource
        for (int i = 0; i < historyList.size(); ++i) {
            if (historyList[i].shapeDataSource) {
                historyList[i].shapeDataSource->Modified();
                historyList[i].shapeDataSource->Update();
            }
        }

        // 更新 shapePicker 的 renderer
        if (shapePicker) {
            shapePicker->SetRenderer(renderer);
        }

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
        history.occShape = newShape;
        history.type = EXTRUSION;  // 标记为拉伸操作的结果

        // 先对OCC形状进行网格离散化（提高质量）
        BRepMesh_IncrementalMesh mesh(newShape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        // 使用 VIS 重新生成数据源
        ++shapeIDCounter;
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(newShape);
        shapeWrapper->SetId(shapeIDCounter);

        // 手动构建 PolyData（OCCT 7.7.0 需要）
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        
        // 清理线条和顶点数据，只保留多边形（面）
        if (meshPolyData) {
            meshPolyData->SetLines(nullptr);  // 移除线条
            meshPolyData->SetVerts(nullptr);  // 移除顶点
        }

        // 创建 ShapeDataSource（仅用于拾取）
        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDataSource->SetShape(shapeWrapper);
        // OCCT 7.7.0 需要显式更新才能让拾取器识别
        shapeDataSource->Modified();
        shapeDataSource->Update();

        // 更新actor的映射器
        if (history.actor) {
            vtkSmartPointer<vtkDataSetMapper> newMapper = vtkSmartPointer<vtkDataSetMapper>::New();
            newMapper->SetInputData(meshPolyData);  // 使用手动构建的数据
            configureSolidMapperForBoundaryOutline(newMapper);
            history.actor->SetMapper(newMapper);
            history.solidDisplayFilter = configureSolidShapePipeline(shapeDataSource, newMapper, history.actor);
            history.edgeDisplayFilter = nullptr;

            // 确保关闭边缘显示
            applySolidActorMaterial(history.actor->GetProperty());

            // 重新绑定数据源到 actor
            IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, history.actor);
        }
        
        // 更新高亮过滤器（接 ShapeDataSource 输出）
        if (history.highlightFilter) {
            history.highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
            history.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
        }

        // 保存 PolyData（使用深拷贝，确保数据独立）
        vtkSmartPointer<vtkPolyData> newPolyData = vtkSmartPointer<vtkPolyData>::New();
        newPolyData->DeepCopy(meshPolyData);  // 使用深拷贝而不是浅拷贝
        history.polyData = newPolyData;

        // 保存引用 - 关键：同时保存Handle和数据源
        history.shapeWrapper = shapeWrapper;
        history.shapeDataSource = shapeDataSource;

        // 清除旧的轮廓线，确保使用新的 PolyData 重新生成
        if (history.outlineActor) {
            renderer->RemoveActor(history.outlineActor);
            history.outlineActor = nullptr;
        }
        history.outlinePolyData = nullptr;
        history.outlineSourcePolyData = nullptr;
        ensureModelBoundaryOutline(index);
        
        // 如果当前模型被选中，重新生成轮廓线以反映合并后的形状
        if (currentSelectedIndex == index) {
            highlightModel(index);
        }

        // 重新渲染
        vtkWidget->renderWindow()->Render();

        // 更新历史列表显示
        updateHistoryList();

        // 更新所有 ShapeDataSource
        for (int i = 0; i < historyList.size(); ++i) {
            if (historyList[i].shapeDataSource) {
                historyList[i].shapeDataSource->Modified();
                historyList[i].shapeDataSource->Update();
            }
        }

        // 更新 shapePicker 的 renderer
        if (shapePicker) {
            shapePicker->SetRenderer(renderer);
        }

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

    if (historyList[index].actor) {
        historyList[index].actor->SetVisibility(visible ? 1 : 0);
        historyList[index].actor->SetPickable((historyList[index].type == DATUM_PLANE
                                               || historyList[index].type == DATUM_AXIS
                                               || historyList[index].type == WORK_CSYS
                                               || historyList[index].type == REFERENCE_CSYS)
                                                  ? false
                                                  : visible);

        if (shapePicker && historyList[index].type != DATUM_PLANE
            && historyList[index].type != DATUM_AXIS
            && historyList[index].type != WORK_CSYS && historyList[index].type != REFERENCE_CSYS) {
            shapePicker->SetSelectionMode(historyList[index].actor, SM_Face, visible);
            shapePicker->SetSelectionMode(historyList[index].actor, SM_Edge, visible);
            shapePicker->SetSelectionMode(historyList[index].actor, SM_Vertex, visible);
        }

        if (historyList[index].outlineActor) {
            historyList[index].outlineActor->SetVisibility(visible ? 1 : 0);
        }
        if (!visible && historyList[index].highlightActor) {
            historyList[index].highlightActor->SetVisibility(0);
        }

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
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
    if (!historyList[index].actor) return true;
    return historyList[index].actor->GetVisibility() != 0;
}

void Widget::updateModelShapeWithTypeInternal(int index, const TopoDS_Shape& newShape, ModelType newType, bool triggerCascade)
{
    if (index < 0 || index >= historyList.size()) return;
    if (newShape.IsNull()) return;

    ModelingHistory& history = historyList[index];

    history.occShape = newShape;
    history.type = newType;

    BRepMesh_IncrementalMesh mesh(newShape, 0.05, Standard_False, 0.3, Standard_True);
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

    if (history.actor) {
        vtkSmartPointer<vtkDataSetMapper> newMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        newMapper->SetInputData(meshPolyData);
        configureSolidMapperForBoundaryOutline(newMapper);
        history.actor->SetMapper(newMapper);
        history.solidDisplayFilter = configureSolidShapePipeline(shapeDataSource, newMapper, history.actor);
        history.edgeDisplayFilter = nullptr;

        applySolidActorMaterial(history.actor->GetProperty());

        IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, history.actor);
    }

    if (history.highlightFilter) {
        history.highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
        history.highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
    }

    vtkSmartPointer<vtkPolyData> newPolyData = vtkSmartPointer<vtkPolyData>::New();
    newPolyData->DeepCopy(meshPolyData);
    history.polyData = newPolyData;

    history.shapeWrapper = shapeWrapper;
    history.shapeDataSource = shapeDataSource;

    if (history.outlineActor && renderer) {
        renderer->RemoveActor(history.outlineActor);
        history.outlineActor = nullptr;
    }
    history.outlinePolyData = nullptr;
    history.outlineSourcePolyData = nullptr;
    ensureModelBoundaryOutline(index);

    if (currentSelectedIndex == index) {
        highlightModel(index);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }

    updateHistoryList();

    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    if (shapePicker) {
        shapePicker->SetRenderer(renderer);
    }

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
