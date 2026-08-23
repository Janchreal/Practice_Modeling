#ifndef DELETEMODELCOMMAND_H
#define DELETEMODELCOMMAND_H

#include "command.h"
#include "modelhistorysnapshot.h"

class Widget;

class DeleteModelCommand : public Command {
public:
    DeleteModelCommand(Widget* widget, int modelIndex);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    int modelIndex_ = -1;
    ModelHistorySnapshot deletedSnapshot_;
    CascadeUndoRecord cascadeRecord_;
};

#endif // DELETEMODELCOMMAND_H
