// 布尔运算：对话框、执行、目标/工具选择与隐藏（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "bool_operation_dialog.h"
#include "boolean_ops.h"
#include "application/commands/booleancommand.h"
#include "rendering/model/model_display_style.h"

#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QSet>
#include <algorithm>

#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>

#include <vtkRenderWindow.h>
#include <vtkProperty.h>

// 布尔运算按钮点击事件
void Widget::on_boolOperationButton_clicked()
{
    if (historyList.size() < 2) {
        QMessageBox::warning(this, QStringLiteral("警告"),
                             QStringLiteral("至少需要两个物体才能进行布尔运算！"));
        return;
    }

    currentSelectionMode = None;
    selectedTargetIndex = -1;
    selectedToolIndices.clear();

    boolDialog = new BoolOperationDialog(dialogParentWidget());

    connect(boolDialog, &BoolOperationDialog::startSelectionTarget,
            this, &Widget::onBoolSelectionTarget);
    connect(boolDialog, &BoolOperationDialog::startSelectionTool, this, [this]() {
        selectedToolIndices.clear();
        onBoolSelectionTool();
    });
    connect(boolDialog, &BoolOperationDialog::accepted,
            this, &Widget::onBoolOperationConfirmed);
    connect(boolDialog, &BoolOperationDialog::rejected, this, [this]() {
        currentSelectionMode = None;
        selectedTargetIndex = -1;
        selectedToolIndices.clear();
        highlightModel(-1);
        if (boolDialog) {
            boolDialog->disconnect();
            boolDialog->deleteLater();
            boolDialog = nullptr;
        }
    });

    boolDialog->show();
    onBoolSelectionTarget();
}

void Widget::performBooleanOperation(int targetIndex,
                                     const QList<int>& toolIndices,
                                     int operationType,
                                     bool keepTarget,
                                     bool keepTool)
{
    if (targetIndex < 0 || targetIndex >= historyList.size() || toolIndices.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("选择的几何体索引无效！"));
        return;
    }

    QSet<int> uniqueTools;
    for (int toolIndex : toolIndices) {
        if (toolIndex < 0 || toolIndex >= historyList.size()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("选择的几何体索引无效！"));
            return;
        }
        if (toolIndex == targetIndex) {
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("目标体和工具体不能包含同一个物体！"));
            return;
        }
        uniqueTools.insert(toolIndex);
    }

    const QList<int> normalizedTools = uniqueTools.values();

    try {
        const TopoDS_Shape targetShape = getShapeFromHistory(targetIndex);
        if (targetShape.IsNull()) {
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("无法获取目标体\"%1\"的形状数据！")
                                     .arg(historyList[targetIndex].name));
            return;
        }

        QList<TopoDS_Shape> toolShapes;
        QStringList toolNames;
        for (int toolIndex : normalizedTools) {
            const TopoDS_Shape toolShape = getShapeFromHistory(toolIndex);
            if (toolShape.IsNull()) {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("无法获取工具体\"%1\"的形状数据！")
                                         .arg(historyList[toolIndex].name));
                return;
            }
            toolShapes.append(toolShape);
            toolNames << historyList[toolIndex].name;
        }

        if (!BooleanOps::shapesSatisfyBooleanOverlap(targetShape, toolShapes, operationType)) {
            const QString opName = BooleanOps::booleanOperationName(operationType);
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("%1运算要求目标体与工具体之间存在重叠部分，请重新选择！")
                                     .arg(opName));
            return;
        }

        TopoDS_Shape resultShape;
        if (!BooleanOps::executeOccBooleanMulti(targetShape, toolShapes, operationType, resultShape)) {
            const QString opName = BooleanOps::booleanOperationName(operationType);
            if (operationType < 0 || operationType > 2) {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("未知的布尔运算类型！"));
            } else {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("%1运算失败！").arg(opName));
            }
            return;
        }

        if (resultShape.IsNull()) {
            QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("布尔运算结果为空！"));
            return;
        }

        const QString operationName = BooleanOps::booleanOperationName(operationType);
        const QString targetName = historyList[targetIndex].name;
        const QString resultName = QStringLiteral("%1(%2,%3)")
                                       .arg(operationName, targetName, toolNames.join(QStringLiteral("+")));

        displayOccShape(resultShape, resultName, BOOLEAN_RESULT, QColor(255, 140, 0));

        if (!historyList.isEmpty()) {
            const int resultIndex = historyList.size() - 1;
            historyList[resultIndex].booleanTargetIndex = targetIndex;
            historyList[resultIndex].booleanToolIndices = normalizedTools;
            historyList[resultIndex].booleanOperationType = operationType;
            historyList[resultIndex].booleanKeepTarget = keepTarget;
            historyList[resultIndex].booleanKeepTool = keepTool;

            FeatureRecipe recipe;
            recipe.hasRecipe = true;
            recipe.boolean.targetIndex = targetIndex;
            recipe.boolean.toolIndices = normalizedTools;
            recipe.boolean.operationType = operationType;
            recipe.boolean.keepTarget = keepTarget;
            recipe.boolean.keepTool = keepTool;
            recipe.parentIndices.append(targetIndex);
            for (int toolIndex : normalizedTools) {
                recipe.parentIndices.append(toolIndex);
            }
            assignFeatureRecipe(resultIndex, recipe);
        }

        hideOriginalBodies(targetIndex, normalizedTools, keepTarget, keepTool);

        // 不要重建 shapePicker（会导致 g_mainPickerMap 与成员不同步，布尔结果无法拾取/高亮）
        if (shapePicker && renderer) {
            refreshShapePickerBindingsForCurrentContext();
            if (vtkWidget && vtkWidget->renderWindow()) {
                vtkWidget->renderWindow()->Render();
            }
        }
    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, QStringLiteral("错误"),
                              QStringLiteral("布尔运算异常: %1").arg(e.GetMessageString()));
    }
}

void Widget::onBoolSelectionTarget()
{
    currentSelectionMode = SelectTarget;
    if (boolDialog) {
        boolDialog->setTargetBodyName(QStringLiteral("请点击选择目标体..."));
        boolDialog->enableConfirmButton(false);
    }
}

void Widget::onBoolSelectionTool()
{
    currentSelectionMode = SelectTool;
    if (!boolDialog) {
        return;
    }

    if (selectedToolIndices.isEmpty()) {
        boolDialog->setToolSelectionPrompt();
        boolDialog->enableConfirmButton(false);
        return;
    }

    QStringList names;
    for (int index : selectedToolIndices) {
        if (index >= 0 && index < historyList.size()) {
            names << historyList[index].name;
        }
    }
    boolDialog->setToolBodyNames(names);
    boolDialog->enableConfirmButton(false);
}

void Widget::onBoolOperationConfirmed()
{
    if (selectedTargetIndex == -1 || selectedToolIndices.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("请先选择目标体和至少一个工具体！"));
        return;
    }

    if (!boolDialog) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("对话框未初始化！"));
        return;
    }

    const int operationType = boolDialog->getOperationType();
    const bool keepTarget = boolDialog->getKeepTarget();
    const bool keepTool = boolDialog->getKeepTool();
    const QString operationName = BooleanOps::booleanOperationName(operationType);

    Command* command = new BooleanCommand(
        this, selectedTargetIndex, selectedToolIndices, operationType,
        keepTarget, keepTool, operationName);

    executeCommand(command);

    currentSelectionMode = None;
    selectedTargetIndex = -1;
    selectedToolIndices.clear();

    if (boolDialog) {
        boolDialog->disconnect();
        boolDialog->close();
        boolDialog->deleteLater();
        boolDialog = nullptr;
    }

    highlightModel(-1);
}

void Widget::handleBooleanSelection(vtkActor* selectedActor)
{
    if (!selectedActor || currentSelectionMode == None) {
        return;
    }

    const int selectedIndex = resolveHistoryIndexByActor(selectedActor);
    if (selectedIndex == -1) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法找到选中的模型！"));
        currentSelectionMode = None;
        return;
    }

    switch (currentSelectionMode) {
    case SelectTarget:
        selectedTargetIndex = selectedIndex;
        if (boolDialog) {
            boolDialog->setTargetBodyName(historyList[selectedIndex].name);
        }
        highlightBooleanOperands();
        onBoolSelectionTool();
        return;

    case SelectTool:
        if (selectedIndex == selectedTargetIndex) {
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("工具体不能与目标体相同！"));
            return;
        }
        if (!selectedToolIndices.contains(selectedIndex)) {
            selectedToolIndices.append(selectedIndex);
        }
        if (boolDialog) {
            QStringList names;
            for (int index : selectedToolIndices) {
                names << historyList[index].name;
            }
            boolDialog->setToolBodyNames(names);
        }
        highlightBooleanOperands();
        if (selectedTargetIndex != -1 && !selectedToolIndices.isEmpty() && boolDialog) {
            boolDialog->enableConfirmButton(true);
        }
        return;

    default:
        break;
    }
}

void Widget::highlightBooleanOperands()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (!renderStateFor(historyList[i]).actor) {
            continue;
        }

        ModelDisplayStyle::restoreModelAppearance(
            renderStateFor(historyList[i]).actor->GetProperty(),
            historyList[i].type,
            historyList[i].color);

        if (renderStateFor(historyList[i]).outlineActor) {
            renderer->RemoveActor(renderStateFor(historyList[i]).outlineActor);
            renderStateFor(historyList[i]).outlineActor = nullptr;
        }
    }

    auto highlightOne = [this](int index, const QColor& color) {
        if (index < 0 || index >= historyList.size() || !renderStateFor(historyList[index]).actor) {
            return;
        }
        ModelDisplayStyle::applyModelColorOpacity(
            renderStateFor(historyList[index]).actor->GetProperty(), color, 0.5);
    };

    highlightOne(selectedTargetIndex, QColor::fromRgbF(0.3, 1.0, 0.3));
    for (int index : selectedToolIndices) {
        highlightOne(index, QColor::fromRgbF(1.0, 0.3, 0.3));
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::hideOriginalBodies(int targetIndex,
                                const QList<int>& toolIndices,
                                bool keepTarget,
                                bool keepTool)
{
    auto hideBody = [this](int index) {
        if (index < 0 || index >= historyList.size()) {
            return;
        }
        ModelingHistory& record = historyList[index];
        if (renderStateFor(record).actor) {
            renderStateFor(record).actor->SetVisibility(false);
            renderStateFor(record).actor->SetPickable(false);
            IVtkTools_ShapeObject::SetShapeSource(nullptr, renderStateFor(record).actor);
        }
        if (renderStateFor(record).outlineActor) {
            renderStateFor(record).outlineActor->SetVisibility(false);
        }
        if (renderStateFor(record).highlightActor) {
            renderStateFor(record).highlightActor->SetVisibility(false);
        }
    };

    if (!keepTarget) {
        hideBody(targetIndex);
    }

    if (!keepTool) {
        for (int toolIndex : toolIndices) {
            hideBody(toolIndex);
        }
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    updateHistoryListVisibility();
}
