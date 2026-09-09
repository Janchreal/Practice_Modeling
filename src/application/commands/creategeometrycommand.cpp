#include "creategeometrycommand.h"
#include "modeling_command_port.h"

CreateGeometryCommand::CreateGeometryCommand(ModelingCommandPort* context,
                                             const QString& name,
                                             const QColor& color,
                                             const PrimitiveGeometry::PrimitiveBuildRequest& request)
    : Command(context)
    , name(name)
    , color(color)
    , request(request)
{
}

void CreateGeometryCommand::execute()
{
    if (!context_) return;

    // 记录执行前的历史记录大小
    int previousSize = context_->getHistorySize();

    // 直接创建几何体（支持任意方向 gp_Dir）
    modelIndex = context_->createGeometryDirectly(name, color, request);

    // 验证创建是否成功
    if (modelIndex == -1) {
        // 创建失败：上层 UI 会给出提示/处理
    } else if (modelIndex != previousSize) {
        // 索引不一致：不影响功能，避免输出调试信息
    }
}

void CreateGeometryCommand::undo()
{
    if (!context_ || modelIndex == -1) return;

    // 删除我们创建的那个模型
    context_->removeModelByIndex(modelIndex);
}

QString CreateGeometryCommand::getDescription() const
{
    return QString("创建%1").arg(name);
}
