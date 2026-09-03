#include "commandmanager.h"

#include <QtAlgorithms>

CommandManager::CommandManager(int maxUndoStackSize)
    : maxUndoStackSize_(maxUndoStackSize)
{
}

CommandManager::~CommandManager()
{
    clear();
}

void CommandManager::execute(Command* command)
{
    if (!command) {
        return;
    }

    command->execute();
    pushToUndoStack(command);
    clearRedoStack();
}

void CommandManager::undo()
{
    if (!canUndo()) {
        return;
    }

    Command* command = undoStack_.takeLast();
    command->undo();
    redoStack_.append(command);
}

void CommandManager::redo()
{
    if (!canRedo()) {
        return;
    }

    Command* command = redoStack_.takeLast();
    command->execute();
    pushToUndoStack(command);
}

bool CommandManager::canUndo() const
{
    return !undoStack_.isEmpty();
}

bool CommandManager::canRedo() const
{
    return !redoStack_.isEmpty();
}

void CommandManager::clear()
{
    qDeleteAll(undoStack_);
    undoStack_.clear();
    qDeleteAll(redoStack_);
    redoStack_.clear();
}

const Command* CommandManager::undoCommand() const
{
    return undoStack_.isEmpty() ? nullptr : undoStack_.last();
}

const Command* CommandManager::redoCommand() const
{
    return redoStack_.isEmpty() ? nullptr : redoStack_.last();
}

void CommandManager::pushToUndoStack(Command* command)
{
    undoStack_.append(command);
    if (undoStack_.size() > maxUndoStackSize_) {
        delete undoStack_.takeFirst();
    }
}

void CommandManager::clearRedoStack()
{
    qDeleteAll(redoStack_);
    redoStack_.clear();
}


