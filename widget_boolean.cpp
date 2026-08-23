// 布尔运算：对话框、执行、目标/工具选择与隐藏（从 widget.cpp 拆出）
#include "widget.h"
#include "ui_widget.h"
#include "booloperationdialog.h"
#include "booleancommand.h"
#include "command.h"

#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QSet>
#include <algorithm>

#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>

#include <vtkRenderWindow.h>
#include <vtkProperty.h>

namespace {

bool boundingBoxesOverlap(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    if (a.IsNull() || b.IsNull()) {
        return false;
    }

    Bnd_Box boxA;
    Bnd_Box boxB;
    BRepBndLib::Add(a, boxA);
    BRepBndLib::Add(b, boxB);
    boxA.Enlarge(Precision::Confusion() * 10.0);
    return !boxA.IsOut(boxB);
}

bool commonSolidVolumePositive(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    if (a.IsNull() || b.IsNull()) {
        return false;
    }

    BRepAlgoAPI_Common common(a, b);
    if (!common.IsDone()) {
        return false;
    }

    const TopoDS_Shape inter = common.Shape();
    if (inter.IsNull()) {
        return false;
    }

    GProp_GProps props;
    BRepGProp::VolumeProperties(inter, props);
    return props.Mass() > Precision::Confusion();
}

TopoDS_Shape fuseShapes(const TopoDS_Shape& first, const TopoDS_Shape& second)
{
    if (first.IsNull()) {
        return second;
    }
    if (second.IsNull()) {
        return first;
    }

    BRepAlgoAPI_Fuse fuse(first, second);
    if (!fuse.IsDone()) {
        return TopoDS_Shape();
    }
    return fuse.Shape();
}

} // namespace

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

bool executeOccBoolean(const TopoDS_Shape& targetShape,
                       const TopoDS_Shape& toolShape,
                       int operationType,
                       TopoDS_Shape& resultShape)
{
    switch (operationType) {
    case 0: {
        BRepAlgoAPI_Fuse op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    case 1: {
        BRepAlgoAPI_Common op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    case 2: {
        BRepAlgoAPI_Cut op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    default:
        return false;
    }
}

bool executeOccBooleanMulti(const TopoDS_Shape& targetShape,
                            const QList<TopoDS_Shape>& toolShapes,
                            int operationType,
                            TopoDS_Shape& resultShape)
{
    if (targetShape.IsNull() || toolShapes.isEmpty()) {
        return false;
    }

    switch (operationType) {
    case 0: {
        TopoDS_Shape fused = targetShape;
        for (const TopoDS_Shape& toolShape : toolShapes) {
            if (toolShape.IsNull()) {
                return false;
            }
            fused = fuseShapes(fused, toolShape);
            if (fused.IsNull()) {
                return false;
            }
        }
        resultShape = fused;
        return true;
    }
    case 1: {
        TopoDS_Shape common = targetShape;
        for (const TopoDS_Shape& toolShape : toolShapes) {
            if (toolShape.IsNull()) {
                return false;
            }
            BRepAlgoAPI_Common op(common, toolShape);
            if (!op.IsDone()) {
                return false;
            }
            common = op.Shape();
            if (common.IsNull()) {
                return false;
            }
        }
        resultShape = common;
        return true;
    }
    case 2: {
        TopoDS_Shape combinedTools = toolShapes.first();
        for (int i = 1; i < toolShapes.size(); ++i) {
            combinedTools = fuseShapes(combinedTools, toolShapes.at(i));
            if (combinedTools.IsNull()) {
                return false;
            }
        }
        BRepAlgoAPI_Cut op(targetShape, combinedTools);
        if (!op.IsDone()) {
            return false;
        }
        resultShape = op.Shape();
        return true;
    }
    default:
        return false;
    }
}

bool shapesSatisfyBooleanOverlap(const TopoDS_Shape& targetShape,
                                 const QList<TopoDS_Shape>& toolShapes,
                                 int operationType)
{
    if (targetShape.IsNull() || toolShapes.isEmpty()) {
        return false;
    }

    for (const TopoDS_Shape& toolShape : toolShapes) {
        if (toolShape.IsNull()) {
            return false;
        }

        switch (operationType) {
        case 0:
            if (!boundingBoxesOverlap(targetShape, toolShape)) {
                return false;
            }
            break;
        case 1:
        case 2:
            if (!commonSolidVolumePositive(targetShape, toolShape)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }

    return true;
}

QString booleanOperationName(int operationType)
{
    switch (operationType) {
    case 0: return QStringLiteral("并集");
    case 1: return QStringLiteral("交集");
    case 2: return QStringLiteral("差集");
    default: return QStringLiteral("未知");
    }
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

        if (!shapesSatisfyBooleanOverlap(targetShape, toolShapes, operationType)) {
            const QString opName = booleanOperationName(operationType);
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("%1运算要求目标体与工具体之间存在重叠部分，请重新选择！")
                                     .arg(opName));
            return;
        }

        TopoDS_Shape resultShape;
        if (!executeOccBooleanMulti(targetShape, toolShapes, operationType, resultShape)) {
            const QString opName = booleanOperationName(operationType);
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

        const QString operationName = booleanOperationName(operationType);
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
            shapePicker->SetRenderer(renderer);
            shapePicker->SetTolerance(0.05);
            prepareShapePickerBindingsForCurrentContext();
            for (int i = 0; i < historyList.size(); ++i) {
                if (historyList[i].shapeDataSource) {
                    historyList[i].shapeDataSource->Modified();
                    historyList[i].shapeDataSource->Update();
                }
            }
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
    const QString operationName = booleanOperationName(operationType);

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
        if (!historyList[i].actor) {
            continue;
        }

        const QColor originalColor = historyList[i].color;
        historyList[i].actor->GetProperty()->SetColor(
            originalColor.redF(), originalColor.greenF(), originalColor.blueF());
        historyList[i].actor->GetProperty()->SetOpacity(1.0);
        historyList[i].actor->GetProperty()->SetEdgeVisibility(0);
        historyList[i].actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        historyList[i].actor->GetProperty()->SetLineWidth(1.4);

        if (historyList[i].outlineActor) {
            renderer->RemoveActor(historyList[i].outlineActor);
            historyList[i].outlineActor = nullptr;
        }
        historyList[i].outlinePolyData = nullptr;
        historyList[i].outlineSourcePolyData = nullptr;
    }

    auto highlightOne = [this](int index, double r, double g, double b) {
        if (index < 0 || index >= historyList.size() || !historyList[index].actor) {
            return;
        }
        historyList[index].actor->GetProperty()->SetColor(r, g, b);
        historyList[index].actor->GetProperty()->SetOpacity(0.5);
    };

    highlightOne(selectedTargetIndex, 0.3, 1.0, 0.3);
    for (int index : selectedToolIndices) {
        highlightOne(index, 1.0, 0.3, 0.3);
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
        if (record.actor) {
            record.actor->SetVisibility(false);
            record.actor->SetPickable(false);
            IVtkTools_ShapeObject::SetShapeSource(nullptr, record.actor);
        }
        if (record.outlineActor) {
            record.outlineActor->SetVisibility(false);
        }
        if (record.highlightActor) {
            record.highlightActor->SetVisibility(false);
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
