#include "sketcheditcommand.h"
#include "modeling_command_port.h"

SketchEditCommand::SketchEditCommand(ModelingCommandPort* context,
                                     const QList<TopoDS_Shape>& before,
                                     const QList<TopoDS_Shape>& after,
                                     const QString& desc)
    : Command(context),
      before_(before),
      after_(after),
      desc_(desc)
{
}

void SketchEditCommand::execute()
{
    if (!context_) return;
    context_->setActiveSketchGeometriesForCommand(after_);
}

void SketchEditCommand::undo()
{
    if (!context_) return;
    context_->setActiveSketchGeometriesForCommand(before_);
}

QString SketchEditCommand::getDescription() const
{
    return desc_;
}
