// 撤销/重做、删除与恢复模型（从 widget.cpp 拆出）
#include "widget.h"
#include "ui_widget.h"
#include "command.h"

#include <QDateTime>
#include <QMessageBox>
#include <QString>
#include <QtAlgorithms>

#include <Standard_Failure.hxx>

#include <BRepMesh_IncrementalMesh.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <TopoDS_Shape.hxx>

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

// Undo/Redo 实现
void Widget::executeCommand(Command* command)
{
    if (!command) return;

    command->execute();
    pushToUndoStack(command);
    clearRedoStack();

    emit undoStateChanged(canUndo(), canRedo());
}

void Widget::pushToUndoStack(Command* command)
{
    undoStack.append(command);

    // 限制栈大小
    if (undoStack.size() > MAX_UNDO_STACK_SIZE) {
        delete undoStack.takeFirst();
    }
}

void Widget::clearRedoStack()
{
    qDeleteAll(redoStack);
    redoStack.clear();
}

void Widget::undo()
{
    if (!canUndo()) return;

    Command* command = undoStack.takeLast();
    command->undo();
    redoStack.append(command);

    emit undoStateChanged(canUndo(), canRedo());
}

void Widget::redo()
{
    if (!canRedo()) return;

    Command* command = redoStack.takeLast();
    command->execute();
    undoStack.append(command);

    emit undoStateChanged(canUndo(), canRedo());
}

bool Widget::canUndo() const
{
    return !undoStack.isEmpty();
}

bool Widget::canRedo() const
{
    return !redoStack.isEmpty();
}

void Widget::removeModel(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    // 从渲染器中移除
    if (historyList[index].actor) {
        removeSceneActor(historyList[index].actor);
    }
    if (historyList[index].outlineActor) {
        removeSceneActor(historyList[index].outlineActor);
    }
    if (historyList[index].highlightActor) {
        removeSceneActor(historyList[index].highlightActor);
    }

    // 只删除一个模型
    historyList.removeAt(index);
    updateHistoryList();
    updateFeatureTree();  // 更新特征树
    vtkWidget->renderWindow()->Render();
    markDocumentModified(true);
}

void Widget::showModel(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    if (historyList[index].actor) {
        historyList[index].actor->SetVisibility(true);
    }
    if (historyList[index].outlineActor) {
        historyList[index].outlineActor->SetVisibility(true);
    }

    updateHistoryListVisibility();
    vtkWidget->renderWindow()->Render();
}
// Undo按钮点击槽函数
void Widget::on_undoButton_clicked()
{
    undo();
}

// Redo按钮点击槽函数
void Widget::on_redoButton_clicked()
{
    redo();
}

// 更新按钮状态
void Widget::updateUndoRedoButtons()
{
    bool undoEnabled = canUndo();
    bool redoEnabled = canRedo();

    // 设置按钮启用状态
    ui->undoButton->setEnabled(undoEnabled);
    ui->redoButton->setEnabled(redoEnabled);

    // 可选：添加工具提示显示下一步操作
    if (undoEnabled && !undoStack.isEmpty()) {
        QString desc = undoStack.last()->getDescription();
        ui->undoButton->setToolTip(QString("撤销: %1").arg(desc));
    } else {
        ui->undoButton->setToolTip("撤销");
    }

    if (redoEnabled && !redoStack.isEmpty()) {
        QString desc = redoStack.last()->getDescription();
        ui->redoButton->setToolTip(QString("重做: %1").arg(desc));
    } else {
        ui->redoButton->setToolTip("重做");
    }

    // 可选：根据状态改变按钮样式
    QString normalStyle = "QPushButton { background-color: #f0f0f0; border: 1px solid #ccc; }";
    QString enabledStyle = "QPushButton { background-color: #e6f3ff; border: 1px solid #0078d4; }";

    ui->undoButton->setStyleSheet(undoEnabled ? enabledStyle : normalStyle);
    ui->redoButton->setStyleSheet(redoEnabled ? enabledStyle : normalStyle);

}
int Widget::createGeometryDirectly(ModelType type, const QString& name, const QColor& color,
                                   double param1, double param2, double param3,
                                   bool hasOrigin,
                                   double originX, double originY, double originZ,
                                   AxisDirection axisDirection,
                                   bool axisReversed,
                                   bool hasCustomVectorDir,
                                   const gp_Dir& customVectorDir)
{
    try {
        TopoDS_Shape shape;

        switch (type) {
        case CUBOID:
            shape = createCuboidShape(param1, param2, param3, hasOrigin, originX, originY, originZ,
                                       axisDirection, axisReversed,
                                       hasCustomVectorDir, customVectorDir);
            break;
        case CYLINDER:
            shape = createCylinderShape(param1, param2, hasOrigin, originX, originY, originZ,
                                         axisDirection, axisReversed,
                                         hasCustomVectorDir, customVectorDir);
            break;
        case CONE:
            shape = createConeShape(param1, param2, param3, hasOrigin, originX, originY, originZ,
                                   axisDirection, axisReversed,
                                   hasCustomVectorDir, customVectorDir);
            break;
        case SPHERE:
            shape = createSphereShape(param1, hasOrigin, originX, originY, originZ);
            break;
        default:
            return -1;
        }

        if (shape.IsNull()) {
            return -1;
        }

        displayOccShape(shape, name, type, color, param1, param2, param3,
                        hasOrigin, originX, originY, originZ,
                        axisDirection, axisReversed,
                        hasCustomVectorDir, customVectorDir);
        
        return historyList.size() - 1; // 返回新模型的索引

    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("创建几何体失败: %1").arg(e.GetMessageString()));
        return -1;
    }
}

void Widget::removeModelByIndex(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    const bool isWorkCsys = (historyList[index].type == WORK_CSYS);
    const bool isWorkCsysActor = (isWorkCsys && workCsysActor.GetPointer() != nullptr
                                  && historyList[index].actor == workCsysActor);
    const bool isRefCsys = (historyList[index].type == REFERENCE_CSYS);
    const bool isRefCsysActor = (isRefCsys && refCsysActor_.GetPointer() != nullptr
                                 && historyList[index].actor == refCsysActor_);

    // 从渲染器中移除所有相关的actor
    if (historyList[index].actor) {
        removeSceneActor(historyList[index].actor);
    }
    if (historyList[index].outlineActor) {
        removeSceneActor(historyList[index].outlineActor);
    }
    if (historyList[index].highlightActor) {
        removeSceneActor(historyList[index].highlightActor);
    }

    if (isWorkCsys || isWorkCsysActor) {
        if (workCsysAxisXActor_) removeSceneActor(workCsysAxisXActor_);
        if (workCsysAxisYActor_) removeSceneActor(workCsysAxisYActor_);
        if (workCsysAxisZActor_) removeSceneActor(workCsysAxisZActor_);
        if (workCsysLabelXActor_) removeSceneActor(workCsysLabelXActor_);
        if (workCsysLabelYActor_) removeSceneActor(workCsysLabelYActor_);
        if (workCsysLabelZActor_) removeSceneActor(workCsysLabelZActor_);
    }
    if (isRefCsys || isRefCsysActor) {
        clearReferenceCsysState();
    }

    historyList.removeAt(index);
    remapHistoryIndicesAfterRemoval(index);

    // 若删除的是工作坐标系，清空对应状态
    if (isWorkCsys || isWorkCsysActor) {
        workCsysActor = nullptr;
        workCsysTransform = nullptr;
        hasWorkCsys = false;
        workCsysDragActive = false;
        workCsysHistoryIndex_ = -1;
        workCsysAxisXActor_ = nullptr;
        workCsysAxisYActor_ = nullptr;
        workCsysAxisZActor_ = nullptr;
        workCsysLabelXActor_ = nullptr;
        workCsysLabelYActor_ = nullptr;
        workCsysLabelZActor_ = nullptr;
    } else {
        // 其它项被删除时，修正工作/参考坐标系在 historyList 中的索引
        if (workCsysHistoryIndex_ > index) {
            workCsysHistoryIndex_ -= 1;
        }
        if (referenceCsysHistoryIndex_ > index) {
            referenceCsysHistoryIndex_ -= 1;
        }
    }

    updateHistoryList();
    updateFeatureTree();  // 更新特征树
    vtkWidget->renderWindow()->Render();
    markDocumentModified(true);

    // 删除后：不要重建 shapePicker（在本工程中重建会导致剩余模型不可拾取/只剩最后一个可拾取）
    // 仅刷新剩余模型的拾取绑定与数据源即可
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }
    if (shapePicker) {
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
    }

}
void Widget::restoreModel(int index, const QString& name, ModelType type, const QColor& color,
                          double param1, double param2, double param3, const TopoDS_Shape& occShape,
                          bool hasOrigin,
                          double originX, double originY, double originZ,
                          AxisDirection axisDirection,
                          bool axisReversed,
                          bool hasCustomVectorDir,
                          const gp_Dir& customVectorDir)
{
    try {
        if (type == WORK_CSYS) {
            restoreWorkCsys(index, name, color, param1, param2, param3);
            return;
        }
        if (type == REFERENCE_CSYS) {
            restoreReferenceCsys(index, name, color);
            return;
        }
        // 先对OCC形状进行网格离散化（提高质量）
        BRepMesh_IncrementalMesh mesh(occShape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        // 使用 VIS 恢复形状
        ++shapeIDCounter;
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(occShape);
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

        // 创建映射器
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData(meshPolyData);  // 使用手动构建的数据
        // IVtkTools::InitShapeMapper(mapper);  // 注释掉，避免边缘显示
        configureSolidMapperForBoundaryOutline(mapper);

        // 创建演员并设置渲染属性
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
        actor->SetPickable(true);
        
        // 设置更好的渲染属性（配合场景三点光）
        applySolidActorMaterial(actor->GetProperty());
        vtkSmartPointer<IVtkTools_DisplayModeFilter> solidDisplayFilter =
            configureSolidShapePipeline(shapeDataSource, mapper, actor);

        // 绑定数据源到 actor
        IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, actor);

        // 添加到渲染器
        vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter =
            vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
        highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
        highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");

        vtkSmartPointer<vtkPolyDataMapper> highlightMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        highlightMapper->SetInputConnection(highlightFilter->GetOutputPort());
        highlightMapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> highlightActor = vtkSmartPointer<vtkActor>::New();
        highlightActor->SetMapper(highlightMapper);
        highlightActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
        highlightActor->GetProperty()->SetOpacity(0.6);
        highlightActor->GetProperty()->SetRepresentationToSurface();
        highlightActor->GetProperty()->EdgeVisibilityOff();
        highlightActor->GetProperty()->SetLighting(true);
        highlightActor->SetPickable(false);
        highlightActor->SetVisibility(false);
        renderer->AddActor(actor);
        addAppearanceActor(highlightActor);

        // 保存 PolyData
        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->ShallowCopy(meshPolyData);

        // 创建历史记录
        ModelingHistory record;
        record.type = type;
        record.name = name;
        record.timestamp = QDateTime::currentDateTime();
        record.actor = actor;
        record.color = color;
        record.param1 = param1;
        record.param2 = param2;
        record.param3 = param3;
        record.polyData = polyData;
        record.occShape = occShape;
        record.shapeWrapper = shapeWrapper;  // 保存Handle引用
        record.shapeDataSource = shapeDataSource;
        record.solidDisplayFilter = solidDisplayFilter;
        record.highlightFilter = highlightFilter;
        record.highlightActor = highlightActor;
        record.outlineActor = nullptr;
        record.outlinePolyData = nullptr;
        record.outlineSourcePolyData = nullptr;
        record.hasOrigin = hasOrigin;
        record.originX = originX;
        record.originY = originY;
        record.originZ = originZ;
        record.axisDirection = axisDirection;
        record.axisReversed = axisReversed;

        // 插入到指定位置
        if (index >= 0 && index <= historyList.size()) {
            remapHistoryIndicesAfterInsertion(index);
            historyList.insert(index, record);
            ensureModelBoundaryOutline(index);
        } else {
            historyList.append(record);
            ensureModelBoundaryOutline(historyList.size() - 1);
        }

        updateHistoryList();
        updateFeatureTree();  // 更新特征树
        vtkWidget->renderWindow()->Render();

        // 恢复模型后，更新所有 ShapeDataSource
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


    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("模型恢复失败: %1").arg(e.GetMessageString()));
    }
}

void Widget::restoreWorkCsys(int index, const QString& name, const QColor& color,
                             double x, double y, double z)
{
    if (!renderer || !vtkWidget) return;
    if (!ensureWorkCsysActorsCreated()) return;
    // 恢复时允许颜色自定义（记录颜色），但三轴仍使用红绿蓝/橙色高亮，这里忽略 color。

    workCsysTransform->Identity();
    workCsysTransform->Translate(x, y, z);
    hasWorkCsys = true;
    workCsysDragActive = false;
    setWorkCsysVisible(true);
    applyAxisDirectionHighlight(currentAxisDirection);

    // 将工作坐标系插回 historyList 指定位置
    vtkSmartPointer<vtkPolyData> emptyPoly;
    ModelingHistory record;
    record.type = WORK_CSYS;
    record.name = name;
    record.timestamp = QDateTime::currentDateTime();
    record.actor = workCsysActor;
    record.color = color;
    record.param1 = x;
    record.param2 = y;
    record.param3 = z;
    record.polyData = emptyPoly;
    record.occShape = TopoDS_Shape();
    record.shapeWrapper = nullptr;
    record.shapeDataSource = nullptr;
    record.outlineActor = nullptr;
    record.outlinePolyData = nullptr;
    record.outlineSourcePolyData = nullptr;

    if (index < 0) index = 0;
    if (index > historyList.size()) index = historyList.size();
    remapHistoryIndicesAfterInsertion(index);
    historyList.insert(index, record);
    workCsysHistoryIndex_ = index;

    updateFeatureTree();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}
