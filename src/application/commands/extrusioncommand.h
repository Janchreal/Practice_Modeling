#ifndef EXTRUSIONCOMMAND_H
#define EXTRUSIONCOMMAND_H

#include "command.h"
#include "domain/features/featurerecipe.h"
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>
#include <QList>
#include <QColor>
#include <list>
#include <string>

// 前向声明
class ModelingCommandPort;

/**
 * @brief 拉伸参数结构体
 * 完全按照FeatureExtrusion中的ExtrusionParameters结构
 */
struct ExtrusionParameters
{
    gp_Dir dir;
    double lengthFwd {0};
    double lengthRev {0};
    bool solid {false};
    double taperAngleFwd {0};  // 弧度
    double taperAngleRev {0};  // 弧度
    std::string faceMakerClass;
};

/**
 * @brief 拉伸命令类
 * 
 * 完全按照FeatureExtrusion的实现逻辑，支持：
 * - 双向拉伸（LengthFwd, LengthRev）
 * - 对称拉伸（Symmetric）
 * - 锥度角（TaperAngle）
 * - 实体/壳体模式（Solid）
 * - 方向反转（Reversed）
 */
class ExtrusionCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param context 建模命令端口
     * @param profileIndices 要拉伸的轮廓索引列表（从历史记录中）
     * @param direction 拉伸方向
     * @param lengthFwd 正向拉伸长度
     * @param lengthRev 反向拉伸长度
     * @param resultName 结果名称
     * @param resultColor 结果颜色
     * @param solid 是否为实体模式（true=实体，false=壳体）
     * @param reversed 是否反转方向
     * @param symmetric 是否对称拉伸
     * @param taperAngle 正向锥度角（度）
     * @param taperAngleRev 反向锥度角（度）
     */
    ExtrusionCommand(ModelingCommandPort* context,
                     const QList<int>& profileIndices,
                     const gp_Dir& direction,
                     double lengthFwd,
                     double lengthRev = 0.0,
                     const QString& resultName = QString(),
                     const QColor& resultColor = QColor(255, 140, 0),
                     bool solid = true,
                     bool reversed = false,
                     bool symmetric = false,
                     double taperAngle = 0.0,
                     double taperAngleRev = 0.0);

    ~ExtrusionCommand() override = default;

    void execute() override;
    void undo() override;
    QString getDescription() const override;

    static void extrudeShape(TopoDS_Shape& result, const TopoDS_Shape& source, const ExtrusionParameters& params);

    static FeatureRecipe buildProfileExtrusionRecipe(int profileIndex, const ExtrusionParameters& params);
    static bool extrudeFromRecipe(ModelingCommandPort* context, const ExtrusionRecipeData& recipe, TopoDS_Shape& resultShape);

private:
    /**
     * @brief 执行拉伸操作
     * 完全按照FeatureExtrusion::execute的逻辑
     */
    void performExtrusion();

    /**
     * @brief 计算最终拉伸参数
     * 完全按照FeatureExtrusion::computeFinalParameters的逻辑
     */
    ExtrusionParameters computeFinalParameters();

    /**
     * @brief 创建带锥度的拉伸（使用ExtrusionHelper::makeDraft的逻辑）
     * @param shape 源形状
     * @param params 拉伸参数
     * @param drafts 输出结果列表
     */
    static void makeDraft(const TopoDS_Shape& shape,
                          const ExtrusionParameters& params,
                          std::list<TopoDS_Shape>& drafts);

    QList<int> profileIndices;      // 轮廓索引列表
    gp_Dir direction;                // 拉伸方向
    double lengthFwd;                // 正向拉伸长度
    double lengthRev;                // 反向拉伸长度
    QString resultName;              // 结果名称
    QColor resultColor;              // 结果颜色
    bool solid;                      // 是否为实体模式
    bool reversed;                   // 是否反转方向
    bool symmetric;                  // 是否对称拉伸
    double taperAngle;               // 正向锥度角（度）
    double taperAngleRev;           // 反向锥度角（度）
    
    QList<int> resultIndices;       // 生成的结果索引（用于撤销）
    bool executed;                   // 是否已执行
};

#endif // EXTRUSIONCOMMAND_H

