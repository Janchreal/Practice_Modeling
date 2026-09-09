// 历史列表、删除、子形状/模型高亮、清空历史（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "deletemodelcommand.h"
#include "rendering/model/model_display_style.h"

#include <QDateTime>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <TColStd_MapIteratorOfPackedMapOfInteger.hxx>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCellData.h>
#include <vtkFeatureEdges.h>
#include <vtkIdTypeArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

// 添加到历史记录
void Widget::addToHistory(ModelType type, const QString& name,
                              vtkSmartPointer<vtkActor> actor, const QColor& color,double param1,double param2,double param3,
                              vtkSmartPointer<vtkPolyData> polyData,const TopoDS_Shape& occShape,
                              Handle(IVtkOCC_Shape) shapeWrapper,
                              vtkSmartPointer<IVtkTools_ShapeDataSource> dataSource)
{
    ModelingHistory record;
    record.type = type;
    record.name = name;
    record.timestamp = QDateTime::currentDateTime();
    record.color = color;
    record.param1=param1;
    record.param2=param2;
    record.param3=param3;
    const int index = modelDocument_.append(record);
    ModelingHistory& storedRecord = historyList[index];
    renderStateFor(storedRecord).actor = actor;
    renderStateFor(storedRecord).polyData = polyData;
    geometryStateFor(storedRecord).occShape = occShape;
    renderStateFor(storedRecord).shapeWrapper = shapeWrapper;
    renderStateFor(storedRecord).shapeDataSource = dataSource;
    updateHistoryList();
    updateFeatureTree();  // 同时更新特征树
    markDocumentModified(true);
}

void Widget::updateHistoryList()
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    updateFeatureTree();
    syncMirrorWindows();
    return;
}
// 改进的点击事件处理
void Widget::handleHistoryItemClicked(QListWidgetItem *item)
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    Q_UNUSED(item);
    return;
}
// 删除单个历史记录项
void Widget::onDeleteHistoryItem(int index)
{
    if (index < 0 || index >= historyList.size()) return;

    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要删除\"%1\"吗？").arg(historyList[index].name),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 保存删除前的模型数据
        const ModelingHistory& record = historyList[index];

        // 创建删除命令
        Command* command = new DeleteModelCommand(this, index);

        // 执行删除命令
        executeCommand(command);

        QMessageBox::information(this, "完成", QString("已删除\"%1\"").arg(record.name));
    }
}

// 高亮显示选中的子形状（使用 VIS：与 vis_picker_example 一致，SubPolyDataFilter::SetData + Mapper 更新）
void Widget::highlightSubShapes(int modelIndex, const IVtk_ShapeIdList& subShapeIds)
{
    clearSubShapeHighlight();

    if (modelIndex < 0 || modelIndex >= historyList.size()) {
        return;
    }

    if (subShapeIds.Extent() == 0) {
        return;
    }

    ModelingHistory& record = historyList[modelIndex];

    if (renderStateFor(record).highlightFilter.GetPointer() == nullptr ||
        renderStateFor(record).highlightActor.GetPointer() == nullptr) {
        return;
    }

    vtkPolyData* inputData = vtkPolyData::SafeDownCast(
        renderStateFor(record).highlightFilter->GetInputDataObject(0, 0));
    if (!inputData) {
        return;
    }

    vtkIdTypeArray* subShapeIDArray = vtkIdTypeArray::SafeDownCast(
        inputData->GetCellData()->GetArray("SUBSHAPE_IDS"));
    if (!subShapeIDArray) {
        return;
    }

    TColStd_PackedMapOfInteger cellMask;
    for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
        cellMask.Add((int)sIt.Value());
    }

    IVtk_IdTypeMap dataToKeep;
    for (TColStd_MapIteratorOfPackedMapOfInteger it(cellMask); it.More(); it.Next()) {
        dataToKeep.Add(it.Key());
    }

    renderStateFor(record).highlightFilter->SetData(dataToKeep);
    renderStateFor(record).highlightFilter->Modified();

    if (vtkPolyDataMapper* hm = vtkPolyDataMapper::SafeDownCast(renderStateFor(record).highlightActor->GetMapper())) {
        hm->Update();
    }

    addAppearanceActor(renderStateFor(record).highlightActor);
    renderStateFor(record).highlightActor->SetVisibility(true);
    currentPickedModelIndex = modelIndex;

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 清除子形状高亮（仅隐藏 VIS 叠加层；整体选中样式由 highlightModel 负责）
void Widget::clearSubShapeHighlight()
{
    if (currentPickedModelIndex >= 0 && currentPickedModelIndex < historyList.size()) {
        ModelingHistory& record = historyList[currentPickedModelIndex];
        if (renderStateFor(record).highlightActor) {
            renderStateFor(record).highlightActor->SetVisibility(false);
        }
    }
    currentPickedModelIndex = -1;
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 高亮显示选中的模型（整体高亮）
void Widget::highlightModel(int index)
{
    hoveredModelIndex_ = -1;
    hoverBaseSelectedIndex_ = -2;

    // 拉伸/旋转幽灵模式下：不要用整模高亮冲掉半透明+轮廓
    if (featureOperationGhostMode_) {
        for (int i = 0; i < historyList.size(); ++i) {
            if (renderStateFor(historyList[i]).highlightActor) {
                renderStateFor(historyList[i]).highlightActor->SetVisibility(false);
            }
            applyFeatureGhostStyleToModel(i);
        }
        currentSelectedIndex = index;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        syncMirrorWindows();
        return;
    }

    // 首先恢复所有模型的常规外观与黑色轮廓
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor) {
            ModelDisplayStyle::restoreModelAppearance(
                renderStateFor(historyList[i]).actor->GetProperty(),
                historyList[i].type,
                historyList[i].color);
        }

        // 清除VIS高亮
        if (renderStateFor(historyList[i]).highlightActor) {
            renderStateFor(historyList[i]).highlightActor->SetVisibility(false);
        }

        // 保持/重建黑色轮廓（不再删除）
        ensureModelBoundaryOutline(i);
        styleModelBoundaryOutline(i, false);
    }

    // 高亮选中的模型
    if (index >= 0 && index < historyList.size() && renderStateFor(historyList[index]).actor) {
        QColor selectedColor;
        if (index == selectedTargetIndex) {
            selectedColor = QColor::fromRgbF(0.3, 1.0, 0.3);
        } else if (selectedToolIndices.contains(index)) {
            selectedColor = QColor::fromRgbF(1.0, 0.3, 0.3);
        } else {
            selectedColor = QColor::fromRgbF(1.0, 1.0, 0.0);
        }
        ModelDisplayStyle::applySelectedModelAppearance(
            renderStateFor(historyList[index]).actor->GetProperty(),
            selectedColor,
            historyList[index].type == SKETCH);

        // 选中轮廓改为青色加粗
        ensureModelBoundaryOutline(index);
        styleModelBoundaryOutline(index, true);

        vtkWidget->renderWindow()->Render();
        currentSelectedIndex = index;
    } else {
        currentSelectedIndex = -1;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
    syncMirrorWindows();
}

void Widget::clearModelHoverHighlight()
{
    if (hoveredModelIndex_ < 0 && hoverBaseSelectedIndex_ == currentSelectedIndex) {
        return;
    }

    const int selectedIndex = currentSelectedIndex;
    hoveredModelIndex_ = -1;
    hoverBaseSelectedIndex_ = -2;
    highlightModel(selectedIndex);
    hoveredModelIndex_ = -1;
    hoverBaseSelectedIndex_ = currentSelectedIndex;
}

void Widget::updateHistoryListSelection()
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    return;
}

void Widget::updateHistoryListVisibility()
{
    // 已弃用 historyList（QListWidget），仅使用左侧特征树
    return;
}

void Widget::showAllModels()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor) {
            renderStateFor(historyList[i]).actor->SetVisibility(true);
            renderStateFor(historyList[i]).actor->SetPickable(
                ModelDisplayStyle::isInteractiveModelType(historyList[i].type));
        }
        if (renderStateFor(historyList[i]).outlineActor) {
            renderStateFor(historyList[i]).outlineActor->SetVisibility(true);
        }
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    updateHistoryListVisibility();
}

void Widget::updateModelHoverHighlight(int x, int y)
{
    if (featureOperationGhostMode_ || currentSelectionMode != None || snap_.armed) {
        clearModelHoverHighlight();
        return;
    }

    auto pickByShapePicker = [&]() -> int {
        if (!shapePicker || !renderer) return -1;
        refreshShapePickerBindingsForCurrentContext(0.05, false);

        auto firstHistoryIndex = [&]() -> int {
            vtkSmartPointer<vtkActorCollection> actors = shapePicker->GetPickedActors(true);
            if (!actors || actors->GetNumberOfItems() <= 0) return -1;
            actors->InitTraversal();
            while (vtkActor* actor = actors->GetNextActor()) {
                if (!actor || actor->GetVisibility() == 0 || actor->GetPickable() == 0) continue;
                const int idx = resolveHistoryIndexByActor(actor);
                if (idx >= 0) return idx;
            }
            return -1;
        };

        shapePicker->SetSelectionMode(SM_Face);
        shapePicker->Pick(x, y, 0);
        int idx = firstHistoryIndex();
        if (idx >= 0) return idx;

        shapePicker->SetSelectionMode(SM_Edge);
        shapePicker->Pick(x, y, 0);
        idx = firstHistoryIndex();
        if (idx >= 0) return idx;

        shapePicker->SetSelectionMode(SM_Vertex);
        shapePicker->Pick(x, y, 0);
        return firstHistoryIndex();
    };

    const bool mirrorContext = (mirrorContextForVtkWidget(vtkWidget) != nullptr);
    int hoverIndex = mirrorContext ? pickByShapePicker() : pickHistoryModelStrict(x, y);

    if (hoverIndex < 0 || hoverIndex >= historyList.size() || hoverIndex == currentSelectedIndex) {
        clearModelHoverHighlight();
        return;
    }

    const ModelingHistory& picked = historyList[hoverIndex];
    if (!renderStateFor(picked).actor || renderStateFor(picked).actor->GetVisibility() == 0 || picked.type == DATUM_PLANE
        || !ModelDisplayStyle::needsBoundaryOutline(picked.type)) {
        clearModelHoverHighlight();
        return;
    }

    if (hoveredModelIndex_ == hoverIndex && hoverBaseSelectedIndex_ == currentSelectedIndex) {
        return;
    }

    const int selectedIndex = currentSelectedIndex;
    hoveredModelIndex_ = -1;
    hoverBaseSelectedIndex_ = -2;
    highlightModel(selectedIndex);

    if (hoverIndex < 0 || hoverIndex >= historyList.size() || !renderStateFor(historyList[hoverIndex]).actor) {
        return;
    }

    ModelingHistory& rec = historyList[hoverIndex];
    hoveredModelIndex_ = hoverIndex;
    hoverBaseSelectedIndex_ = selectedIndex;

    ModelDisplayStyle::applyHoverModelAppearance(renderStateFor(rec).actor->GetProperty());

    ensureModelBoundaryOutline(hoverIndex);
    if (renderStateFor(rec).outlineActor) {
        ModelDisplayStyle::applyHoverBoundaryOutlineStyle(
            renderStateFor(rec).outlineActor->GetProperty());
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    syncMirrorWindows();
}

// 清空历史记录
void Widget::handleClearHistory()
{
    // 检查历史记录是否为空
    if (historyList.isEmpty()) {
        QMessageBox::information(this, "提示", "历史记录已经是空的！");
        return;
    }

    // 创建确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认清空",
                                  "确定要清空所有历史记录吗？\n此操作不可撤销！",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No); // 默认选择"No"

    // 如果用户点击"No"，则取消操作
    if (reply != QMessageBox::Yes) {
        return;
    }

    // 从渲染器中移除所有actor和轮廓actor
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor) removeSceneActor(renderStateFor(historyList[i]).actor);
        if (renderStateFor(historyList[i]).outlineActor) removeSceneActor(renderStateFor(historyList[i]).outlineActor);
        if (renderStateFor(historyList[i]).highlightActor) removeSceneActor(renderStateFor(historyList[i]).highlightActor);
    }

    // 记录清空前的记录数量用于日志
    int recordsCount = historyList.size();

    modelDocument_.clear();
    geometryStore_.clear();
    renderStore_.clear();
    markDocumentModified(true);
    
    // 清空特征树，但保留"模型历史"父节点结构
    QTreeWidgetItem* modelHistoryRoot = nullptr;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
        if (item && item->text(0) == "模型历史") {
            modelHistoryRoot = item;
            break;
        }
    }
    if (modelHistoryRoot) {
        modelHistoryRoot->takeChildren();  // 只清除子项，保留父节点
    } else {
        ui->treeWidget->clear();  // 如果父节点不存在，清空整个树
    }

    // 重置选择状态
    currentSelectedIndex = -1;
    selectedTargetIndex = -1;
    selectedToolIndices.clear();
    currentSelectionMode = None;
    extrusionSelectedIndices.clear();

    // 清空Undo/Redo栈
    commandManager_.clear();

    // 重新渲染
    vtkWidget->renderWindow()->Render();

    // 更新按钮状态
    emit undoStateChanged(false, false);

    // 显示成功消息
    QMessageBox::information(this, "完成",
                             QString("已清空所有历史记录\n共删除了 %1 个模型").arg(recordsCount));
}
