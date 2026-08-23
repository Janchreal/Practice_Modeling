#ifndef CREATEGEOMETRYCOMMAND_H
#define CREATEGEOMETRYCOMMAND_H

#include "command.h"
#include "modeltype.h"  // 包含ModelType定义
#include "axisdirection.h"

#include <gp_Dir.hxx>

#include <QColor>
#include <QString>

// 前向声明
class Widget;

class CreateGeometryCommand : public Command {
public:
    CreateGeometryCommand(Widget* widget, ModelType type,
                          const QString& name, const QColor& color,
                          double param1, double param2 = 0, double param3 = 0,
                          bool hasOrigin = false,
                          double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                          AxisDirection axisDirection = AxisDirection::Z,
                          bool axisReversed = false,
                          bool hasCustomVectorDir = false,
                          const gp_Dir& customVectorDir = gp_Dir(0, 0, 1));

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    ModelType type;
    QString name;
    QColor color;
    double param1, param2, param3;
    int modelIndex = -1; // 使用更清晰的变量名

    bool hasOrigin = false;
    double originX = 0.0, originY = 0.0, originZ = 0.0;
    AxisDirection axisDirection = AxisDirection::Z;
    bool axisReversed = false;
    bool hasCustomVectorDir = false;
    gp_Dir customVectorDir = gp_Dir(0, 0, 1);
};

#endif // CREATEGEOMETRYCOMMAND_H
