// updatemodelcommand.cpp
#include "updatemodelcommand.h"


UpdateModelCommand::UpdateModelCommand(CommandContext* widget,
                                       int modelIndex,
                                       const TopoDS_Shape& beforeShape,
                                       ModelType beforeType,
                                       const QString& beforeName,
                                       const TopoDS_Shape& afterShape,
                                       ModelType afterType,
                                       const QString& afterName,
                                       const QString& description)
    : modelIndex_(modelIndex),
      beforeShape_(beforeShape),
      beforeType_(beforeType),
      beforeName_(beforeName),
      afterShape_(afterShape),
      afterType_(afterType),
      afterName_(afterName),
      desc_(description)
{
    this->widget = widget;
}

void UpdateModelCommand::execute()
{
    if (!widget) return;

    cascadeRecord_ = widget->beginCascadeUndoCapture(modelIndex_);
    widget->applyModelStateForCommand(modelIndex_, afterShape_, afterType_, afterName_, true);
    widget->finishCascadeUndoCapture(cascadeRecord_);
}

void UpdateModelCommand::undo()
{
    if (!widget || !cascadeRecord_.committed) return;
    widget->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString UpdateModelCommand::getDescription() const
{
    return desc_;
}


