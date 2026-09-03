#include "booleancommand.h"

BooleanCommand::BooleanCommand(CommandContext* widget,
                               int targetIndex,
                               const QList<int>& toolIndices,
                               int operationType,
                               bool keepTarget,
                               bool keepTool,
                               const QString& operationName)
{
    this->widget = widget;
    this->targetIndex = targetIndex;
    this->toolIndices = toolIndices;
    this->operationType = operationType;
    this->keepTarget = keepTarget;
    this->keepTool = keepTool;
    this->operationName = operationName;
}

void BooleanCommand::execute()
{
    if (!widget) return;

    widget->performBooleanOperation(targetIndex, toolIndices, operationType,
                                    keepTarget, keepTool);
    resultIndex = widget->getHistorySize() - 1;
}

void BooleanCommand::undo()
{
    if (!widget || resultIndex == -1) return;

    widget->removeModelByIndex(resultIndex);

    if (!keepTarget) {
        widget->showModel(targetIndex);
    }
    if (!keepTool) {
        for (int toolIndex : toolIndices) {
            widget->showModel(toolIndex);
        }
    }
}

QString BooleanCommand::getDescription() const
{
    return QStringLiteral("布尔运算: %1").arg(operationName);
}


