// 历史列表、删除、子形状/模型高亮、清空历史（从 widget.cpp 拆出）
#include "widget.h"
#include "ui_widget.h"
#include "deletemodelcommand.h"
#include "command.h"

#include <QDateTime>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QtAlgorithms>

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
    record.actor = actor;
    record.color = color;
    record.param1=param1;
    record.param2=param2;
    record.param3=param3;
    record.polyData = polyData;  // 存储多边形数据
    record.occShape = occShape;  // 存储形状
    record.shapeWrapper = shapeWrapper;  // 保存OCCT Handle，防止被释放！
    record.shapeDataSource = dataSource;  // 保存数据源引用，防止被释放

    // 暂时禁用轮廓边缘actor，避免崩溃
    // TODO: 后续使用VIS的SubPolyDataFilter实现轮廓高亮
    record.outlineActor = nullptr;

    historyList.append(record);
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

    if (record.highlightFilter.GetPointer() == nullptr ||
        record.highlightActor.GetPointer() == nullptr) {
        return;
    }

    vtkPolyData* inputData = vtkPolyData::SafeDownCast(
        record.highlightFilter->GetInputDataObject(0, 0));
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

    record.highlightFilter->SetData(dataToKeep);
    record.highlightFilter->Modified();

    if (vtkPolyDataMapper* hm = vtkPolyDataMapper::SafeDownCast(record.highlightActor->GetMapper())) {
        hm->Update();
    }

    addAppearanceActor(record.highlightActor);
    record.highlightActor->SetVisibility(true);
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
        if (record.highlightActor) {
            record.highlightActor->SetVisibility(false);
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
            if (historyList[i].highlightActor) {
                historyList[i].highlightActor->SetVisibility(false);
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
        if (historyList[i].actor) {
            // 恢复原始颜色和透明度
            QColor originalColor = historyList[i].color;
            historyList[i].actor->GetProperty()->SetColor(
                originalColor.redF(), originalColor.greenF(), originalColor.blueF());
            historyList[i].actor->GetProperty()->SetOpacity(1.0);

            // 隐藏表面边缘
            applySolidActorMaterial(historyList[i].actor->GetProperty());
            historyList[i].actor->GetProperty()->SetColor(
                originalColor.redF(), originalColor.greenF(), originalColor.blueF());
        }

        // 清除VIS高亮
        if (historyList[i].highlightActor) {
            historyList[i].highlightActor->SetVisibility(false);
        }

        // 保持/重建黑色轮廓（不再删除）
        ensureModelBoundaryOutline(i);
        styleModelBoundaryOutline(i, false);
    }

    // 高亮选中的模型
    if (index >= 0 && index < historyList.size() && historyList[index].actor) {
        // 设置主模型的颜色（使用明显不同的高亮色）
        if (index == selectedTargetIndex) {
            historyList[index].actor->GetProperty()->SetColor(0.3, 1.0, 0.3);
        } else if (selectedToolIndices.contains(index)) {
            historyList[index].actor->GetProperty()->SetColor(1.0, 0.3, 0.3);
        } else {
            historyList[index].actor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 亮黄色
        }

        // 降低主模型不透明度，让背面的轮廓线能透过来（草图线框不降透明度，否则容易“看不见”）
        if (historyList[index].type == SKETCH) {
            historyList[index].actor->GetProperty()->SetOpacity(1.0);
        } else {
            historyList[index].actor->GetProperty()->SetOpacity(0.5);  // 更透明，可以清楚看到背面
        }

        // 不显示三角网格边缘
        historyList[index].actor->GetProperty()->SetEdgeVisibility(0);
        
        // 增强光照效果使高亮更明显（保持 Phong，避免整模变“塑料板”）
        historyList[index].actor->GetProperty()->SetAmbient(0.26);
        historyList[index].actor->GetProperty()->SetDiffuse(0.88);
        historyList[index].actor->GetProperty()->SetSpecular(0.48);
        historyList[index].actor->GetProperty()->SetSpecularPower(36);

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

void Widget::updateModelHoverHighlight(int x, int y)
{
    if (featureOperationGhostMode_ || currentSelectionMode != None || snap_.armed) {
        clearModelHoverHighlight();
        return;
    }

    auto pickByShapePicker = [&]() -> int {
        if (!shapePicker || !renderer) return -1;
        prepareShapePickerBindingsForCurrentContext();
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);

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
    if (!picked.actor || picked.actor->GetVisibility() == 0 || picked.type == DATUM_PLANE
        || picked.type == DATUM_AXIS || picked.type == WORK_CSYS || picked.type == REFERENCE_CSYS
        || picked.type == SKETCH) {
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

    if (hoverIndex < 0 || hoverIndex >= historyList.size() || !historyList[hoverIndex].actor) {
        return;
    }

    ModelingHistory& rec = historyList[hoverIndex];
    hoveredModelIndex_ = hoverIndex;
    hoverBaseSelectedIndex_ = selectedIndex;

    applySolidActorMaterial(rec.actor->GetProperty());
    rec.actor->GetProperty()->SetColor(0.0, 0.62, 1.0);
    rec.actor->GetProperty()->SetOpacity(0.82);
    rec.actor->GetProperty()->SetAmbient(0.30);
    rec.actor->GetProperty()->SetDiffuse(0.84);
    rec.actor->GetProperty()->SetSpecular(0.55);
    rec.actor->GetProperty()->SetSpecularPower(44);
    rec.actor->GetProperty()->SetEdgeVisibility(0);

    ensureModelBoundaryOutline(hoverIndex);
    if (rec.outlineActor) {
        vtkProperty* p = rec.outlineActor->GetProperty();
        p->SetRepresentationToWireframe();
        p->SetColor(0.0, 0.90, 1.0);
        p->SetLineWidth(2.4);
        p->SetLighting(false);
        p->SetAmbient(1.0);
        p->SetDiffuse(0.0);
        p->SetSpecular(0.0);
        p->RenderLinesAsTubesOff();
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
        if (historyList[i].actor) removeSceneActor(historyList[i].actor);
        if (historyList[i].outlineActor) removeSceneActor(historyList[i].outlineActor);
        if (historyList[i].highlightActor) removeSceneActor(historyList[i].highlightActor);
    }

    // 记录清空前的记录数量用于日志
    int recordsCount = historyList.size();

    historyList.clear();
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
    clearRedoStack();
    qDeleteAll(undoStack);
    undoStack.clear();

    // 重新渲染
    vtkWidget->renderWindow()->Render();

    // 更新按钮状态
    emit undoStateChanged(false, false);

    // 显示成功消息
    QMessageBox::information(this, "完成",
                             QString("已清空所有历史记录\n共删除了 %1 个模型").arg(recordsCount));
}
