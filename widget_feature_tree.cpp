// 特征树、可见性、右键/上下文菜单（从 widget.cpp 拆出）
#include "widget.h"
#include "ui_widget.h"
#include "cuboidparamsdialog.h"
#include "cylinderdialog.h"
#include "coneparamsdialog.h"
#include "sphereparamsdialog.h"
#include "regeneratemodelcommand.h"
#include "deletemodelcommand.h"
#include "command.h"

#include <QColor>
#include <QBrush>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMap>
#include <QPoint>
#include <QTreeWidgetItem>
#include <QVector>
#include <QWidget>

#include <IVtk_Types.hxx>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

namespace {

bool isUnderModelHistoryRoot(const QTreeWidgetItem* item)
{
    for (const QTreeWidgetItem* node = item; node; node = node->parent()) {
        if (node->text(0) == QStringLiteral("模型历史")) {
            return true;
        }
    }
    return false;
}

bool shouldNestFeatureUnderParent(const ModelingHistory& record)
{
    switch (record.type) {
    case EXTRUSION:
    case REVOLUTION:
    case FILLET:
    case HOLLOW:
    case PATTERN:
    case BOOLEAN_RESULT:
    case SKETCH:
        return true;
    default:
        return false;
    }
}

QTreeWidgetItem* findFeatureTreeItemByIndex(QTreeWidgetItem* root, int modelIndex)
{
    if (!root) {
        return nullptr;
    }
    if (root->data(0, Qt::UserRole).toInt() == modelIndex) {
        return root;
    }
    for (int i = 0; i < root->childCount(); ++i) {
        if (QTreeWidgetItem* found = findFeatureTreeItemByIndex(root->child(i), modelIndex)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

// 更新特征树显示
void Widget::updateFeatureTree()
{
    // 保存当前可见性状态
    QVector<bool> visibilityStates;
    for (int i = 0; i < historyList.size(); ++i) {
        bool isVisible = true;
        if (historyList[i].actor) {
            isVisible = (historyList[i].actor->GetVisibility() != 0);
        }
        visibilityStates.append(isVisible);
    }
    
    // 保存当前选中的索引
    int previousSelectedIndex = currentSelectedIndex;
    
    // 查找或创建"模型历史"父节点
    QTreeWidgetItem* modelHistoryRoot = nullptr;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
        if (item && item->text(0) == "模型历史") {
            modelHistoryRoot = item;
            break;
        }
    }
    
    // 如果不存在，创建"模型历史"父节点
    if (!modelHistoryRoot) {
        modelHistoryRoot = new QTreeWidgetItem(ui->treeWidget);
        modelHistoryRoot->setText(0, "模型历史");
        modelHistoryRoot->setExpanded(true);  // 默认展开
        ui->treeWidget->addTopLevelItem(modelHistoryRoot);
    }
    
    // 清除"模型历史"节点的所有子项
    modelHistoryRoot->takeChildren();  // 移除所有子项

    QMap<int, QTreeWidgetItem*> indexToItem;

    auto createFeatureItem = [this](int i) -> QTreeWidgetItem* {
        const ModelingHistory& record = historyList[i];
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setData(0, Qt::UserRole, i);
        if (record.featureRegenerateFailed) {
            item->setText(0, record.name + tr(" [更新失败]"));
            item->setForeground(0, QBrush(QColor(200, 40, 40)));
            item->setToolTip(0, tr("父特征变更后未能重新生成几何，请编辑或重建该特征。"));
        } else {
            item->setText(0, record.name);
            item->setToolTip(0, QString());
        }
        return item;
    };

    // 为每个历史记录创建树项：子特征嵌套在父特征下（类似 NX 建模历史）
    for (int i = 0; i < historyList.size(); ++i) {
        const ModelingHistory& record = historyList[i];
        QTreeWidgetItem* item = createFeatureItem(i);

        int parentIndex = -1;
        if (shouldNestFeatureUnderParent(record)) {
            parentIndex = primaryParentIndex(record);
        }

        if (parentIndex >= 0 && parentIndex < i && indexToItem.contains(parentIndex)) {
            indexToItem[parentIndex]->addChild(item);
            indexToItem[parentIndex]->setExpanded(true);
        } else {
            modelHistoryRoot->addChild(item);
        }
        indexToItem.insert(i, item);

        QCheckBox* checkBox = new QCheckBox();
        checkBox->blockSignals(true);
        checkBox->setChecked(i < visibilityStates.size() ? visibilityStates[i] : true);
        checkBox->blockSignals(false);
        checkBox->setProperty("modelIndex", i);
        connect(checkBox, &QCheckBox::toggled, this, [this, i](bool checked) {
            onFeatureTreeVisibilityChanged(i, checked);
        });
        ui->treeWidget->setItemWidget(item, 1, checkBox);

        if (i == previousSelectedIndex) {
            item->setSelected(true);
            currentSelectedIndex = i;
        }
    }
    
    // 如果有两个或更多模型，确保父节点可以展开/折叠
    if (historyList.size() >= 2) {
        modelHistoryRoot->setExpanded(true);  // 默认展开
    }
}

// 特征树单击事件处理
void Widget::onFeatureTreeItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    // 如果点击的是父项（没有父项，即topLevelItem），直接返回，不执行任何操作
    QTreeWidgetItem* parent = item->parent();
    if (!parent) {
        // 点击的是父项：仅“摄像机”允许点击展开/折叠，其它父项不执行操作
        if (item->text(0) == "摄像机") {
            item->setExpanded(!item->isExpanded());
        }
        return;
    }
    
    // 检查是否是"模型视图"的子项
    if (parent->text(0) == "模型视图") {
        // 点击的是模型视图的子项，只切换视图，不高亮
        QString viewName = item->text(0);
        switchToView(viewName);
        syncViewSelectionInTree(viewName);
        return;
    }
    
    // 检查是否是"摄像机"的子项
    if (parent->text(0) == "摄像机") {
        // 点击的是摄像机的子项：暂时不执行任何操作
        return;
    }
    
    // 只有"模型历史"下的项（含嵌套子项）才执行高亮
    if (isUnderModelHistoryRoot(item)) {
        int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0 && index < historyList.size()) {
            currentSelectedIndex = index;
            highlightModel(index);  // 高亮显示模型
        }
    }
}

// 特征树双击事件处理
void Widget::onFeatureTreeItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    // 检查是否是"模型历史"下的项（含嵌套子项）
    if (!isUnderModelHistoryRoot(item)) {
        return;
    }
    
    int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= historyList.size()) return;
    
    const ModelingHistory& record = historyList[index];
    
    // 根据模型类型打开相应的编辑对话框
    switch (record.type) {
    case CUBOID: {
        CuboidParamsDialog* dialog = new CuboidParamsDialog(dialogParentWidget());
        dialog->setModal(true);
        // 设置当前参数值
        dialog->setLength(record.param1);
        dialog->setWidth(record.param2);
        dialog->setHeight(record.param3);
        if (dialog->exec() == QDialog::Accepted) {
            // 获取新参数值
            double newLength = dialog->getLength();
            double newWidth = dialog->getWidth();
            double newHeight = dialog->getHeight();
            
            // 检查参数是否改变
            if (newLength != record.param1 || newWidth != record.param2 || newHeight != record.param3) {
                executeCommand(new RegenerateModelCommand(
                    this, index, newLength, newWidth, newHeight,
                    QString("修改%1参数").arg(record.name)));
            }
        }
        delete dialog;
        break;
    }
    case CYLINDER: {
        CylinderDialog* dialog = new CylinderDialog(dialogParentWidget());
        dialog->setModal(true);
        // 设置当前参数值
        dialog->setRadius(record.param1);
        dialog->setHeight(record.param2);
        if (dialog->exec() == QDialog::Accepted) {
            // 获取新参数值
            double newRadius = dialog->getRadius();
            double newHeight = dialog->getHeight();
            
            // 检查参数是否改变
            if (newRadius != record.param1 || newHeight != record.param2) {
                executeCommand(new RegenerateModelCommand(
                    this, index, newRadius, newHeight, record.param3,
                    QString("修改%1参数").arg(record.name)));
            }
        }
        delete dialog;
        break;
    }
    case CONE: {
        ConeParamsDialog* dialog = new ConeParamsDialog(dialogParentWidget());
        dialog->setModal(true);
        // 设置当前参数值
        dialog->setRadius1(record.param1);
        dialog->setRadius2(record.param2);
        dialog->setHeight(record.param3);
        if (dialog->exec() == QDialog::Accepted) {
            // 获取新参数值
            double newRadius1 = dialog->getRadius1();
            double newRadius2 = dialog->getRadius2();
            double newHeight = dialog->getHeight();
            
            // 检查参数是否改变
            if (newRadius1 != record.param1 || newRadius2 != record.param2 || newHeight != record.param3) {
                executeCommand(new RegenerateModelCommand(
                    this, index, newRadius1, newRadius2, newHeight,
                    QString("修改%1参数").arg(record.name)));
            }
        }
        delete dialog;
        break;
    }
    case SPHERE: {
        SphereParamsDialog* dialog = new SphereParamsDialog(dialogParentWidget());
        dialog->setModal(true);
        // 设置当前参数值
        dialog->setRadius(record.param1);
        dialog->setThetaResolution((int)record.param2);
        dialog->setPhiResolution((int)record.param3);
        if (dialog->exec() == QDialog::Accepted) {
            // 获取新参数值
            double newRadius = dialog->getRadius();
            int newThetaRes = dialog->getThetaResolution();
            int newPhiRes = dialog->getPhiResolution();
            
            // 检查参数是否改变
            if (newRadius != record.param1 || newThetaRes != (int)record.param2 || newPhiRes != (int)record.param3) {
                executeCommand(new RegenerateModelCommand(
                    this, index, newRadius, static_cast<double>(newThetaRes), static_cast<double>(newPhiRes),
                    QString("修改%1参数").arg(record.name)));
            }
        }
        delete dialog;
        break;
    }
    default:
        // 对于其他类型，可以打开表达式对话框或简单的名称编辑
        bool ok;
        QString newName = QInputDialog::getText(this, "编辑名称", "输入新名称:", 
                                                QLineEdit::Normal, record.name, &ok);
        if (ok && !newName.isEmpty()) {
            historyList[index].name = newName;
            updateFeatureTree();
            updateHistoryList();
        }
        break;
    }
}

void Widget::onFeatureTreeContextMenuRequested(const QPoint& pos)
{
    if (!ui || !ui->treeWidget) return;

    QTreeWidgetItem* item = ui->treeWidget->itemAt(pos);
    if (!item) return;

    // 只对"模型历史"下的项（含嵌套子项）提供菜单
    if (!isUnderModelHistoryRoot(item)) {
        return;
    }

    const int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= historyList.size()) return;

    // 右键时选中该项，便于用户确认操作对象
    ui->treeWidget->setCurrentItem(item);
    currentSelectedIndex = index;

    bool isVisible = true;
    const ModelingHistory& record = historyList[index];
    if (record.type == WORK_CSYS) {
        isVisible = workCsysAxisXActor_ ? (workCsysAxisXActor_->GetVisibility() != 0) : true;
    } else if (record.type == REFERENCE_CSYS) {
        isVisible = refCsysAxisXActor_ ? (refCsysAxisXActor_->GetVisibility() != 0) : true;
    } else if (record.actor) {
        isVisible = (record.actor->GetVisibility() != 0);
    }

    QMenu menu(ui->treeWidget);
    QAction* visibilityAction = menu.addAction(isVisible ? tr("隐藏") : tr("显示"));
    QAction* deleteAction = menu.addAction(tr("删除"));
    QAction* renameAction = menu.addAction(tr("修改名称"));

    QAction* chosen = menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == visibilityAction) {
        toggleModelVisibilityFromContextMenu(index);
    } else if (chosen == deleteAction) {
        onDeleteHistoryItem(index);
    } else if (chosen == renameAction) {
        bool ok = false;
        const QString newName = QInputDialog::getText(
            this,
            tr("修改名称"),
            tr("输入新名称:"),
            QLineEdit::Normal,
            historyList[index].name,
            &ok);
        if (ok) {
            const QString trimmed = newName.trimmed();
            if (trimmed.isEmpty()) {
                QMessageBox::warning(this, tr("提示"), tr("名称不能为空。"));
                return;
            }
            if (trimmed != historyList[index].name) {
                historyList[index].name = trimmed;
                updateFeatureTree();
                updateHistoryList();
            }
        }
    }
}

// 特征树可见性改变处理
void Widget::onFeatureTreeVisibilityChanged(int index, bool visible)
{
    if (index < 0 || index >= historyList.size()) return;

    setModelVisibility(index, visible);

    if (!visible) {
        const QList<int> dependents = collectDependentFeatureIndices(index);
        for (int depIndex : dependents) {
            setModelVisibility(depIndex, false);
        }
    }
    updateFeatureTree();
}

// 显示右键菜单
void Widget::showContextMenu(int x, int y, int modelIndex, const QPoint& globalPos)
{
    QWidget* menuHost = this;
    if (vtkWidget) {
        if (QWidget* w = vtkWidget->window()) {
            menuHost = w;
        }
    }
    QMenu contextMenu(menuHost);
    
    if (modelIndex >= 0 && modelIndex < historyList.size()) {
        // 拾取到模型，显示编辑、删除、隐藏选项
        QAction* editAction = contextMenu.addAction("编辑");
        QAction* deleteAction = contextMenu.addAction("删除");
        
        // 根据当前可见性显示"隐藏"或"显示"
        bool isVisible = true;
        if (historyList[modelIndex].actor) {
            isVisible = (historyList[modelIndex].actor->GetVisibility() != 0);
        }
        QAction* visibilityAction = contextMenu.addAction(isVisible ? "隐藏" : "显示");
        
        // 连接信号
        connect(editAction, &QAction::triggered, this, [this, modelIndex]() {
            editModelFromContextMenu(modelIndex);
        });
        
        connect(deleteAction, &QAction::triggered, this, [this, modelIndex]() {
            deleteModelFromContextMenu(modelIndex);
        });
        
        connect(visibilityAction, &QAction::triggered, this, [this, modelIndex]() {
            toggleModelVisibilityFromContextMenu(modelIndex);
        });
    } else {
        // 未拾取到模型，显示视图操作选项
        QAction* frontViewAction = contextMenu.addAction("正视图");
        QAction* topViewAction = contextMenu.addAction("俯视图");
        QAction* leftViewAction = contextMenu.addAction("左视图");
        QAction* rightViewAction = contextMenu.addAction("右视图");
        QAction* backViewAction = contextMenu.addAction("后视图");
        QAction* bottomViewAction = contextMenu.addAction("仰视图");
        QAction* isoViewAction = contextMenu.addAction("正三轴测视图");
        
        contextMenu.addSeparator();
        QAction* resetViewAction = contextMenu.addAction("重置视图");
        
        // 连接信号
        connect(frontViewAction, &QAction::triggered, this, [this]() {
            switchToView("正视图");
        });
        
        connect(topViewAction, &QAction::triggered, this, [this]() {
            switchToView("俯视图");
        });
        
        connect(leftViewAction, &QAction::triggered, this, [this]() {
            switchToView("左视图");
        });
        
        connect(rightViewAction, &QAction::triggered, this, [this]() {
            switchToView("右视图");
        });
        
        connect(backViewAction, &QAction::triggered, this, [this]() {
            switchToView("后视图");
        });
        
        connect(bottomViewAction, &QAction::triggered, this, [this]() {
            switchToView("仰视图");
        });
        
        connect(isoViewAction, &QAction::triggered, this, [this]() {
            switchToView("正三轴测视图");
        });
        
        connect(resetViewAction, &QAction::triggered, this, [this]() {
            if (renderer) {
                renderer->ResetCamera();
                refreshCameraClippingRange();
                syncCenterAxisCamera();
                refreshOverlayScreenScale();
                if (vtkWidget && vtkWidget->renderWindow()) {
                    vtkWidget->renderWindow()->Render();
                }
            }
        });
    }
    
    // 显示菜单
    // 如果提供了全局位置，直接使用；否则转换VTK坐标为Qt坐标
    QPoint menuPos;
    if (!globalPos.isNull()) {
        menuPos = globalPos;
    } else {
        // 将VTK坐标转换为Qt坐标
        // VTK的y坐标原点在左下角，Qt在左上角
        int qtY = vtkWidget->height() - y;
        QPoint localPos = QPoint(x, qtY);
        menuPos = vtkWidget->mapToGlobal(localPos);
    }
    contextMenu.exec(menuPos);
}

// 从右键菜单编辑模型
void Widget::editModelFromContextMenu(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }
    
    // 触发双击事件来编辑模型
    // 查找特征树中对应的项
    QTreeWidgetItem* modelHistoryRoot = nullptr;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
        if (item && item->text(0) == "模型历史") {
            modelHistoryRoot = item;
            break;
        }
    }
    
    if (modelHistoryRoot) {
        if (QTreeWidgetItem* featureItem = findFeatureTreeItemByIndex(modelHistoryRoot, index)) {
            onFeatureTreeItemDoubleClicked(featureItem, 0);
        }
    }
}

// 从右键菜单删除模型
void Widget::deleteModelFromContextMenu(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }
    
    // 确认删除
    int ret = QMessageBox::question(this, "确认删除", 
                                    QString("确定要删除模型 \"%1\" 吗？").arg(historyList[index].name),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        executeCommand(new DeleteModelCommand(this, index));
    }
}

// 从右键菜单切换模型可见性
void Widget::toggleModelVisibilityFromContextMenu(int index)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }

    // 工作坐标系：单独控制 3 个箭头 + 3 个标签
    if (historyList[index].type == WORK_CSYS) {
        const bool isVisible = (workCsysAxisXActor_ ? (workCsysAxisXActor_->GetVisibility() != 0) : true);
        const bool newVisible = !isVisible;
        setWorkCsysVisible(newVisible);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        // 同步复选框
        QTreeWidgetItem* modelHistoryRoot = nullptr;
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
            if (item && item->text(0) == "模型历史") {
                modelHistoryRoot = item;
                break;
            }
        }
        if (modelHistoryRoot) {
            for (int i = 0; i < modelHistoryRoot->childCount(); ++i) {
                QTreeWidgetItem* child = modelHistoryRoot->child(i);
                int childIndex = child->data(0, Qt::UserRole).toInt();
                if (childIndex == index) {
                    QCheckBox* checkBox = qobject_cast<QCheckBox*>(ui->treeWidget->itemWidget(child, 1));
                    if (checkBox) checkBox->setChecked(newVisible);
                    break;
                }
            }
        }
        return;
    }
    if (historyList[index].type == REFERENCE_CSYS) {
        const bool isVisible = (refCsysAxisXActor_ ? (refCsysAxisXActor_->GetVisibility() != 0) : true);
        const bool newVisible = !isVisible;
        setReferenceCsysVisible(newVisible);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        QTreeWidgetItem* modelHistoryRoot = nullptr;
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
            if (item && item->text(0) == "模型历史") {
                modelHistoryRoot = item;
                break;
            }
        }
        if (modelHistoryRoot) {
            for (int i = 0; i < modelHistoryRoot->childCount(); ++i) {
                QTreeWidgetItem* child = modelHistoryRoot->child(i);
                int childIndex = child->data(0, Qt::UserRole).toInt();
                if (childIndex == index) {
                    QCheckBox* checkBox = qobject_cast<QCheckBox*>(ui->treeWidget->itemWidget(child, 1));
                    if (checkBox) checkBox->setChecked(newVisible);
                    break;
                }
            }
        }
        return;
    }
    
    const bool newVisible = !isModelVisibleForCommand(index);
    onFeatureTreeVisibilityChanged(index, newVisible);
}

