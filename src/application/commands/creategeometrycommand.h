#ifndef CREATEGEOMETRYCOMMAND_H
#define CREATEGEOMETRYCOMMAND_H

#include "command.h"
#include "geometry/primitives/primitive_build_request.h"

#include <QColor>
#include <QString>

// 前向声明
class ModelingCommandPort;

class CreateGeometryCommand : public Command {
public:
    CreateGeometryCommand(ModelingCommandPort* context,
                          const QString& name,
                          const QColor& color,
                          const PrimitiveGeometry::PrimitiveBuildRequest& request);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    QString name;
    QColor color;
    PrimitiveGeometry::PrimitiveBuildRequest request;
    int modelIndex = -1; // 使用更清晰的变量名
};

#endif // CREATEGEOMETRYCOMMAND_H
