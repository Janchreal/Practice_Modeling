#include "deletemodelcommand.h"
#include "modeling_command_port.h"

DeleteModelCommand::DeleteModelCommand(ModelingCommandPort* context, int modelIndex)
    : Command(context)
    , modelIndex_(modelIndex)
{
    if (!context_ || modelIndex < 0 || modelIndex >= context_->getHistorySize()) {
        return;
    }

    deletedSnapshot_ = context_->captureModelSnapshot(modelIndex);
    cascadeRecord_ = context_->beginCascadeUndoCapture(modelIndex);
}

void DeleteModelCommand::execute()
{
    if (!context_ || modelIndex_ < 0 || modelIndex_ >= context_->getHistorySize()) {
        return;
    }

    context_->removeModelByIndex(modelIndex_);
}

void DeleteModelCommand::undo()
{
    if (!context_) {
        return;
    }
    if (deletedSnapshot_.occShape.IsNull()
        && deletedSnapshot_.type != WORK_CSYS
        && deletedSnapshot_.type != REFERENCE_CSYS) {
        return;
    }

    context_->restoreModelFromSnapshot(deletedSnapshot_.historyIndex, deletedSnapshot_);
    context_->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString DeleteModelCommand::getDescription() const
{
    return QString("删除%1").arg(deletedSnapshot_.name);
}
