#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include "command.h"

#include <QList>

class CommandManager {
public:
    explicit CommandManager(int maxUndoStackSize = 50);
    ~CommandManager();

    void execute(Command* command);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;
    void clear();

    const Command* undoCommand() const;
    const Command* redoCommand() const;

private:
    void pushToUndoStack(Command* command);
    void clearRedoStack();

    QList<Command*> undoStack_;
    QList<Command*> redoStack_;
    int maxUndoStackSize_ = 50;
};

#endif // COMMANDMANAGER_H

