#include "sketcheditcommand.h"

SketchEditCommand::SketchEditCommand(CommandContext* widget,
                                     const QList<TopoDS_Shape>& before,
                                     const QList<TopoDS_Shape>& after,
                                     const QString& desc)
    : before_(before),
      after_(after),
      desc_(desc)
{
    this->widget = widget;
}

void SketchEditCommand::execute()
{
    if (!widget) return;
    widget->setActiveSketchGeometriesForCommand(after_);
}

void SketchEditCommand::undo()
{
    if (!widget) return;
    widget->setActiveSketchGeometriesForCommand(before_);
}

QString SketchEditCommand::getDescription() const
{
    return desc_;
}
