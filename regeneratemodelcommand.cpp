#include "regeneratemodelcommand.h"

#include "widget.h"

RegenerateModelCommand::RegenerateModelCommand(Widget* widget,
                                               int modelIndex,
                                               double newParam1,
                                               double newParam2,
                                               double newParam3,
                                               const QString& description)
    : modelIndex_(modelIndex)
    , newParam1_(newParam1)
    , newParam2_(newParam2)
    , newParam3_(newParam3)
    , desc_(description)
{
    this->widget = widget;
}

void RegenerateModelCommand::execute()
{
    if (!widget || modelIndex_ < 0 || modelIndex_ >= widget->getHistorySize()) {
        return;
    }

    cascadeRecord_ = widget->beginCascadeUndoCapture(modelIndex_);
    widget->setModelParametersForCommand(modelIndex_, newParam1_, newParam2_, newParam3_);
    widget->regenerateModelForCommand(modelIndex_, true);
    widget->finishCascadeUndoCapture(cascadeRecord_);
}

void RegenerateModelCommand::undo()
{
    if (!widget || !cascadeRecord_.committed) {
        return;
    }
    widget->restoreCascadeUndoStates(cascadeRecord_, true);
}

QString RegenerateModelCommand::getDescription() const
{
    return desc_;
}
