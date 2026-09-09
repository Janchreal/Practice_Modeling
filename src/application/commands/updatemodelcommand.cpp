// updatemodelcommand.cpp
#include "updatemodelcommand.h"
#include "modeling_command_port.h"

UpdateModelCommand::UpdateModelCommand(ModelingCommandPort* context,
                                       int modelIndex,
                                       const TopoDS_Shape& beforeShape,
                                       ModelType beforeType,
                                       const QString& beforeName,
                                       const TopoDS_Shape& afterShape,
                                       ModelType afterType,
                                       const QString& afterName,
                                       const QString& description)
    : Command(context),
      modelIndex_(modelIndex),
      beforeShape_(beforeShape),
      beforeType_(beforeType),
      beforeName_(beforeName),
      afterShape_(afterShape),
      afterType_(afterType),
      afterName_(afterName),
      desc_(description)
{
}

void UpdateModelCommand::execute()
{
    if (!context_) return;

    cascadeRecord_ = context_->beginCascadeUndoCapture(modelIndex_);
    context_->applyModelStateForCommand(modelIndex_, afterShape_, afterType_, afterName_, true);
    context_->finishCascadeUndoCapture(cascadeRecord_);
}

void UpdateModelCommand::undo()
{
    if (!context_ || !cascadeRecord_.committed) return;
    context_->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString UpdateModelCommand::getDescription() const
{
    return desc_;
}


