#include "regeneratemodelcommand.h"
#include "modeling_command_port.h"

RegenerateModelCommand::RegenerateModelCommand(ModelingCommandPort* context,
                                               int modelIndex,
                                               double newParam1,
                                               double newParam2,
                                               double newParam3,
                                               const QString& description)
    : Command(context)
    , modelIndex_(modelIndex)
    , newParam1_(newParam1)
    , newParam2_(newParam2)
    , newParam3_(newParam3)
    , desc_(description)
{
}

void RegenerateModelCommand::execute()
{
    if (!context_ || modelIndex_ < 0 || modelIndex_ >= context_->getHistorySize()) {
        return;
    }

    cascadeRecord_ = context_->beginCascadeUndoCapture(modelIndex_);
    context_->setModelParametersForCommand(modelIndex_, newParam1_, newParam2_, newParam3_);
    context_->regenerateModelForCommand(modelIndex_, true);
    context_->finishCascadeUndoCapture(cascadeRecord_);
}

void RegenerateModelCommand::undo()
{
    if (!context_ || !cascadeRecord_.committed) {
        return;
    }
    context_->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString RegenerateModelCommand::getDescription() const
{
    return desc_;
}


