// addmodelcommand.h
#ifndef ADDMODELCOMMAND_H
#define ADDMODELCOMMAND_H

#include "command.h"
#include "modeltype.h"
#include "featurerecipe.h"

#include <QColor>
#include <QMap>
#include <QString>
#include <TopoDS_Shape.hxx>

// 用于“直接生成一个新模型记录”的命令：
// - execute(): displayOccShape() 生成新历史记录；可选隐藏某个源模型
// - undo(): 删除该新记录；恢复源模型可见性
class AddModelCommand : public Command {
public:
    AddModelCommand(Widget* widget,
                    const TopoDS_Shape& shape,
                    const QString& name,
                    ModelType type,
                    const QColor& color,
                    double param1 = 0.0,
                    double param2 = 0.0,
                    double param3 = 0.0,
    const QList<int>& hideSourceIndices = QList<int>(),
    bool hideSourceOnExecute = false,
    const FeatureRecipe* recipe = nullptr);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    TopoDS_Shape shape_;
    QString name_;
    ModelType type_ = CUBOID;
    QColor color_;
    double param1_ = 0.0;
    double param2_ = 0.0;
    double param3_ = 0.0;

    int createdIndex_ = -1;

    QList<int> hideSourceIndices_;
    bool hideSourceOnExecute_ = false;
    QMap<int, bool> sourceWasVisibleBefore_;
    FeatureRecipe recipe_;
    bool hasRecipe_ = false;
};

#endif // ADDMODELCOMMAND_H

