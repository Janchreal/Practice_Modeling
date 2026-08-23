// command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <TopoDS_Shape.hxx>

class Widget; // 前向声明

class Command {
public:
    virtual ~Command() = default;

    virtual void execute() = 0;    // 执行命令
    virtual void undo() = 0;       // 撤销命令
    virtual QString getDescription() const = 0; // 命令描述
    virtual bool changesGeometry() const { return true; } // 是否改变几何

protected:
    Widget* widget = nullptr;  // 必须是protected，子类才能访问
};

#endif // COMMAND_H
