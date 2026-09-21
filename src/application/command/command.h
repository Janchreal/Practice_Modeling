// command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
//向前声明，告诉编译器有一个叫做ModelingCommanddPort的类后面会用到
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
    // Command 不直接依赖具体的 Qt Widget，而是依赖一个“应用接口”
    ModelingCommandPort* context_ = nullptr;
};

#endif // COMMAND_H

