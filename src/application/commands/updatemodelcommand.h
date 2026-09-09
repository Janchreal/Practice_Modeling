// updatemodelcommand.h
#ifndef UPDATEMODELCOMMAND_H
#define UPDATEMODELCOMMAND_H

#include "command.h"
#include "application/history/model_history_snapshot.h"
#include "common/modeltype.h"

#include <QString>
#include <TopoDS_Shape.hxx>

// 用于“修改既有模型（形状/类型/名称）”的命令，并保存级联依赖的快照
class UpdateModelCommand : public Command {
public:
    UpdateModelCommand(ModelingCommandPort* context,
                       int modelIndex,
                       const TopoDS_Shape& beforeShape,
                       ModelType beforeType,
                       const QString& beforeName,
                       const TopoDS_Shape& afterShape,
                       ModelType afterType,
                       const QString& afterName,
                       const QString& description);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    int modelIndex_ = -1;

    TopoDS_Shape beforeShape_;
    ModelType beforeType_ = CUBOID;
    QString beforeName_;

    TopoDS_Shape afterShape_;
    ModelType afterType_ = CUBOID;
    QString afterName_;

    QString desc_;
    CascadeUndoRecord cascadeRecord_;
};

#endif // UPDATEMODELCOMMAND_H

