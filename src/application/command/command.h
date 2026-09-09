// command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <QString>

class ModelingCommandPort;

class Command {
public:
    explicit Command(ModelingCommandPort* context)
        : context_(context)
    {
    }

    virtual ~Command() = default;

    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;

    virtual void execute() = 0;    // 执行命令
    virtual void undo() = 0;       // 撤销命令
    virtual QString getDescription() const = 0; // 命令描述
    virtual bool changesGeometry() const { return true; } // 是否改变几何

protected:
    // Commands depend on an application port, not on a concrete widget.
    ModelingCommandPort* context_ = nullptr;
};

#endif // COMMAND_H

