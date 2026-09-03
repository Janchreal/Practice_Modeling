#include "deletemodelcommand.h"

DeleteModelCommand::DeleteModelCommand(CommandContext* widget, int modelIndex)
    : modelIndex_(modelIndex)
{
    this->widget = widget;
    if (!widget || modelIndex < 0 || modelIndex >= widget->getHistorySize()) {
        return;
    }

    deletedSnapshot_ = widget->captureModelSnapshot(modelIndex);
    cascadeRecord_ = widget->beginCascadeUndoCapture(modelIndex);
}

void DeleteModelCommand::execute()
{
    if (!widget || modelIndex_ < 0 || modelIndex_ >= widget->getHistorySize()) {
        return;
    }

    widget->removeModelByIndex(modelIndex_);
}

void DeleteModelCommand::undo()
{
    if (!widget) {
        return;
    }
    if (deletedSnapshot_.occShape.IsNull()
        && deletedSnapshot_.type != WORK_CSYS
        && deletedSnapshot_.type != REFERENCE_CSYS) {
        return;
    }

    widget->restoreModelFromSnapshot(deletedSnapshot_.historyIndex, deletedSnapshot_);
    widget->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString DeleteModelCommand::getDescription() const
{
    return QString("删除%1").arg(deletedSnapshot_.name);
}


