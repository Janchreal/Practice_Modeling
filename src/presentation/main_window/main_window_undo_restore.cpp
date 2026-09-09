// 撤销/重做、删除与恢复模型（从 main_window.cpp 拆出）
#include "main_window.h"
#include "geometry/placement/axis_placement.h"
#include "application/history/modeling_history_placement.h"
#include "ui_main_window.h"
#include "primitive_geometry.h"
#include "shape_presentation_factory.h"

#include <QDateTime>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QStatusBar>

#include <Standard_Failure.hxx>

#include <IVtkTools_ShapeObject.hxx>

#include <TopoDS_Shape.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

// Undo/Redo 实现
void Widget::executeCommand(Command* command)
{
    if (!command) return;

    commandManager_.execute(command);

    emit undoStateChanged(canUndo(), canRedo());
}

void Widget::undo()
{
    if (!canUndo()) return;

    commandManager_.undo();

    emit undoStateChanged(canUndo(), canRedo());
}

void Widget::redo()
{
    if (!canRedo()) return;

    commandManager_.redo();

    emit undoStateChanged(canUndo(), canRedo());
}

bool Widget::canUndo() const
{
    return commandManager_.canUndo();
}

bool Widget::canRedo() const
{
    return commandManager_.canRedo();
}

void Widget::removeModel(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    const ModelingHistory record = historyList[index];

    // 从渲染器中移除
    if (renderStateFor(record).actor) {
        removeSceneActor(renderStateFor(record).actor);
    }
    if (renderStateFor(record).outlineActor) {
        removeSceneActor(renderStateFor(record).outlineActor);
    }
    if (renderStateFor(record).highlightActor) {
        removeSceneActor(renderStateFor(record).highlightActor);
    }
    if (renderStateFor(record).profilePickActor) {
        removeSceneActor(renderStateFor(record).profilePickActor);
    }
    removeIntersectionsForRecord(record.id);

    // 只删除一个模型
    removeRuntimeStateFor(record);
    modelDocument_.removeAt(index);
    updateHistoryList();
    updateFeatureTree();  // 更新特征树
    vtkWidget->renderWindow()->Render();
    markDocumentModified(true);
}

void Widget::showModel(int index)
{
    if (index < 0 || index >= historyList.size()) return;
    if (historyList[index].featureRegenerateFailed
        && geometryStateFor(historyList[index]).occShape.IsNull()) {
        if (statusBar()) {
            statusBar()->showMessage(
                tr("该特征的依赖已失效，无法恢复显示。"), 3000);
        }
        return;
    }

    if (renderStateFor(historyList[index]).actor) {
        renderStateFor(historyList[index]).actor->SetVisibility(true);
    }
    if (renderStateFor(historyList[index]).outlineActor) {
        renderStateFor(historyList[index]).outlineActor->SetVisibility(true);
    }
    if (renderStateFor(historyList[index]).profilePickActor
        && historyList[index].type == SKETCH) {
        const bool profilePickable =
            extrusionDialog
            && (currentSelectionMode == ExtrusionSelection
                || currentSelectionMode == EdgeSelection
                || currentSelectionMode == FaceSelection);
        renderStateFor(historyList[index]).profilePickActor->SetVisibility(true);
        renderStateFor(historyList[index]).profilePickActor->SetPickable(profilePickable);
        IVtkTools_ShapeObject::SetShapeSource(
            profilePickable
                ? renderStateFor(historyList[index]).profilePickShapeDataSource
                : nullptr,
            renderStateFor(historyList[index]).profilePickActor);
        if (shapePicker) {
            shapePicker->SetSelectionMode(
                renderStateFor(historyList[index]).profilePickActor,
                SM_Face, profilePickable);
            shapePicker->SetSelectionMode(
                renderStateFor(historyList[index]).profilePickActor,
                SM_Edge, profilePickable);
        }
    }
    updateIntersectionsForRecord(index);

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
    const Command* undoCommand = commandManager_.undoCommand();
    const Command* redoCommand = commandManager_.redoCommand();

    if (undoEnabled && undoCommand) {
        QString desc = undoCommand->getDescription();
        ui->undoButton->setToolTip(QString("撤销: %1").arg(desc));
    } else {
        ui->undoButton->setToolTip("撤销");
    }

    if (redoEnabled && redoCommand) {
        QString desc = redoCommand->getDescription();
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
int Widget::createGeometryDirectly(const QString& name, const QColor& color,
                                   const PrimitiveGeometry::PrimitiveBuildRequest& request)
{
    try {
        if (!PrimitiveGeometry::isPrimitiveType(request.type)) {
            return -1;
        }

        const TopoDS_Shape shape = PrimitiveGeometry::buildPrimitiveShape(request);

        if (shape.IsNull()) {
            return -1;
        }

        displayOccShape(shape, name, request.type, color,
                        request.param1, request.param2, request.param3,
                        request.placement);

        return historyList.size() - 1; // 返回新模型的索引

    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("创建几何体失败: %1").arg(e.GetMessageString()));
        return -1;
    }
}

void Widget::removeModelByIndex(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    const ModelingHistory record = historyList[index];
    const QList<int> dependentIndices = collectDependentFeatureIndices(index);
    QList<quint64> invalidatedRecordIds;
    for (const int dependentIndex : dependentIndices) {
        if (dependentIndex < 0 || dependentIndex >= historyList.size()
            || dependentIndex == index) {
            continue;
        }
        if (historyList[dependentIndex].id != 0) {
            invalidatedRecordIds.append(historyList[dependentIndex].id);
        }
    }

    const bool isWorkCsys = (record.type == WORK_CSYS);
    const bool isWorkCsysActor = (isWorkCsys && workCsysActor.GetPointer() != nullptr
                                  && renderStateFor(record).actor == workCsysActor);
    const bool isRefCsys = (record.type == REFERENCE_CSYS);
    const bool isRefCsysActor = (isRefCsys && refCsysActor_.GetPointer() != nullptr
                                 && renderStateFor(record).actor == refCsysActor_);

    // 从渲染器中移除所有相关的actor
    if (renderStateFor(record).actor) {
        removeSceneActor(renderStateFor(record).actor);
    }
    if (renderStateFor(record).outlineActor) {
        removeSceneActor(renderStateFor(record).outlineActor);
    }
    if (renderStateFor(record).highlightActor) {
        removeSceneActor(renderStateFor(record).highlightActor);
    }
    if (renderStateFor(record).profilePickActor) {
        removeSceneActor(renderStateFor(record).profilePickActor);
    }
    removeIntersectionsForRecord(record.id);

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

    removeRuntimeStateFor(record);
    modelDocument_.removeAt(index);
    remapHistoryIndicesAfterRemoval(index);

    QStringList invalidatedNames;
    for (int i = 0; i < historyList.size(); ++i) {
        ModelingHistory& dependent = historyList[i];
        if (!invalidatedRecordIds.contains(dependent.id)) {
            continue;
        }

        dependent.featureRegenerateFailed = true;
        geometryStateFor(dependent).occShape = TopoDS_Shape();
        if (renderStateFor(dependent).actor) {
            renderStateFor(dependent).actor->SetVisibility(false);
            renderStateFor(dependent).actor->SetPickable(false);
        }
        if (renderStateFor(dependent).outlineActor) {
            renderStateFor(dependent).outlineActor->SetVisibility(false);
        }
        if (renderStateFor(dependent).highlightActor) {
            renderStateFor(dependent).highlightActor->SetVisibility(false);
        }
        if (renderStateFor(dependent).profilePickActor) {
            removeSceneActor(renderStateFor(dependent).profilePickActor);
            renderStateFor(dependent).profilePickActor = nullptr;
            renderStateFor(dependent).profilePickShapeWrapper = nullptr;
            renderStateFor(dependent).profilePickShapeDataSource = nullptr;
        }
        invalidatedNames.append(dependent.name);
        updateIntersectionsForRecord(i);
    }

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
    if (!invalidatedNames.isEmpty() && statusBar()) {
        statusBar()->showMessage(
            tr("删除源模型后，下游特征已失效：%1")
                .arg(invalidatedNames.join(QStringLiteral("、"))),
            5000);
    }
    vtkWidget->renderWindow()->Render();
    markDocumentModified(true);

    // 删除后：不要重建 shapePicker（在本工程中重建会导致剩余模型不可拾取/只剩最后一个可拾取）
    // 仅刷新剩余模型的拾取绑定与数据源即可
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor && renderStateFor(historyList[i]).shapeDataSource) {
            const bool visible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            renderStateFor(historyList[i]).actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, renderStateFor(historyList[i]).actor);
            }
        }
    }
    refreshShapePickerBindingsForCurrentContext();

}
void Widget::restoreModel(int index, const QString& name, ModelType type, const QColor& color,
                          double param1, double param2, double param3, const TopoDS_Shape& occShape,
                          const GeometryPlacement::AxisPlacement& placement)
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
        if (occShape.IsNull()) {
            QMessageBox::warning(this, "错误", "要恢复的模型形状无效！");
            return;
        }

        ++shapeIDCounter;
        ShapePresentationOptions presentationOptions;
        presentationOptions.color = color;
        presentationOptions.shapeId = shapeIDCounter;
        presentationOptions.meshDeflection = ShapePresentationOptions::kDefaultMeshDeflection;
        presentationOptions.meshAngle = ShapePresentationOptions::kDefaultMeshAngle;
        ModelRenderState renderState =
            ShapePresentationFactory::createSolidModelState(occShape, presentationOptions);
        if (!renderState.actor || !renderState.shapeDataSource) {
            QMessageBox::warning(this, "错误", "恢复模型显示数据失败！");
            return;
        }

        renderer->AddActor(renderState.actor);
        addAppearanceActor(renderState.highlightActor);

        // 创建历史记录
        ModelingHistory record;
        record.type = type;
        record.name = name;
        record.timestamp = QDateTime::currentDateTime();
        record.color = color;
        record.param1 = param1;
        record.param2 = param2;
        record.param3 = param3;
        ModelingHistoryPlacement::applyPlacementToHistory(record, placement);

        // 插入到指定位置
        int insertedIndex = -1;
        if (index >= 0 && index <= historyList.size()) {
            remapHistoryIndicesAfterInsertion(index);
            modelDocument_.insert(index, record);
            insertedIndex = index;
        } else {
            insertedIndex = modelDocument_.append(record);
        }

        ModelingHistory& storedRecord = historyList[insertedIndex];
        geometryStateFor(storedRecord).occShape = occShape;
        renderStateFor(storedRecord) = renderState;
        ensureModelBoundaryOutline(insertedIndex);
        updateIntersectionsForRecord(insertedIndex);

        updateHistoryList();
        updateFeatureTree();  // 更新特征树
        vtkWidget->renderWindow()->Render();

        refreshShapePickerBindingsForCurrentContext();

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
    record.color = color;
    record.param1 = x;
    record.param2 = y;
    record.param3 = z;

    if (index < 0) index = 0;
    if (index > historyList.size()) index = historyList.size();
    remapHistoryIndicesAfterInsertion(index);
    modelDocument_.insert(index, record);
    renderStateFor(historyList[index]).actor = workCsysActor;
    renderStateFor(historyList[index]).polyData = emptyPoly;
    workCsysHistoryIndex_ = index;

    updateFeatureTree();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}
