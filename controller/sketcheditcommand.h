#ifndef SKETCHEDITCOMMAND_H
#define SKETCHEDITCOMMAND_H

#include "command.h"

#include <QList>
#include <QString>

#include <TopoDS_Shape.hxx>

// 草图编辑命令：用于修剪/延伸后的几何替换与撤销
class SketchEditCommand : public Command {
public:
    SketchEditCommand(CommandContext* widget,
                      const QList<TopoDS_Shape>& before,
                      const QList<TopoDS_Shape>& after,
                      const QString& desc);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    QList<TopoDS_Shape> before_;
    QList<TopoDS_Shape> after_;
    QString desc_;
};

#endif // SKETCHEDITCOMMAND_H
