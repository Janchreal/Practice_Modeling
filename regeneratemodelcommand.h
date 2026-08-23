#ifndef REGENERATEMODELCOMMAND_H
#define REGENERATEMODELCOMMAND_H

#include "command.h"
#include "modelhistorysnapshot.h"

// 参数化修改既有模型并触发级联更新的命令（特征树/表达式编辑）
class RegenerateModelCommand : public Command {
public:
    RegenerateModelCommand(Widget* widget,
                           int modelIndex,
                           double newParam1,
                           double newParam2,
                           double newParam3,
                           const QString& description);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    int modelIndex_ = -1;
    double newParam1_ = 0.0;
    double newParam2_ = 0.0;
    double newParam3_ = 0.0;
    QString desc_;
    CascadeUndoRecord cascadeRecord_;
};

#endif // REGENERATEMODELCOMMAND_H
