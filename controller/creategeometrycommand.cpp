#include "creategeometrycommand.h"

CreateGeometryCommand::CreateGeometryCommand(CommandContext* widget, ModelType type,
                                             const QString& name, const QColor& color,
                                             double param1, double param2, double param3,
                                             bool hasOrigin,
                                             double originX, double originY, double originZ,
                                             AxisDirection axisDirection,
                                              bool axisReversed,
                                              bool hasCustomVectorDir,
                                              const gp_Dir& customVectorDir)
{
    this->widget = widget;
    this->type = type;
    this->name = name;
    this->color = color;
    this->param1 = param1;
    this->param2 = param2;
    this->param3 = param3;
    this->hasOrigin = hasOrigin;
    this->originX = originX;
    this->originY = originY;
    this->originZ = originZ;
    this->axisDirection = axisDirection;
    this->axisReversed = axisReversed;
    this->hasCustomVectorDir = hasCustomVectorDir;
    this->customVectorDir = customVectorDir;
}

void CreateGeometryCommand::execute()
{
    if (!widget) return;

    // 记录执行前的历史记录大小
    int previousSize = widget->getHistorySize();

    // 直接创建几何体（支持任意方向 gp_Dir）
    modelIndex = widget->createGeometryDirectly(type, name, color,
                                                param1, param2, param3,
                                                hasOrigin, originX, originY, originZ,
                                                axisDirection, axisReversed,
                                                hasCustomVectorDir, customVectorDir);

    // 验证创建是否成功
    if (modelIndex == -1) {
        // 创建失败：上层 UI 会给出提示/处理
    } else if (modelIndex != previousSize) {
        // 索引不一致：不影响功能，避免输出调试信息
    }
}

void CreateGeometryCommand::undo()
{
    if (!widget || modelIndex == -1) return;

    // 删除我们创建的那个模型
    widget->removeModelByIndex(modelIndex);
}

QString CreateGeometryCommand::getDescription() const
{
    return QString("创建%1").arg(name);
}


