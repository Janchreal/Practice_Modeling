#include "booleancommand.h"
#include "modeling_command_port.h"

BooleanCommand::BooleanCommand(ModelingCommandPort* context,
                               int targetIndex,
                               const QList<int>& toolIndices,
                               int operationType,
                               bool keepTarget,
                               bool keepTool,
                               const QString& operationName)
    : Command(context)
    , targetIndex(targetIndex)
    , toolIndices(toolIndices)
    , operationType(operationType)
    , keepTarget(keepTarget)
    , keepTool(keepTool)
    , operationName(operationName)
{
}

void BooleanCommand::execute()
{
    if (!context_) return;

    context_->performBooleanOperation(targetIndex, toolIndices, operationType,
                                    keepTarget, keepTool);
    resultIndex = context_->getHistorySize() - 1;
}

void BooleanCommand::undo()
{
    if (!context_ || resultIndex == -1) return;

    context_->removeModelByIndex(resultIndex);

    if (!keepTarget) {
        context_->showModel(targetIndex);
    }
    if (!keepTool) {
        for (int toolIndex : toolIndices) {
            context_->showModel(toolIndex);
        }
    }
}

QString BooleanCommand::getDescription() const
{
    return QStringLiteral("布尔运算: %1").arg(operationName);
}


