/**
 * handle_spec.h — 操作柄规格字段定义
 *
 * 对应文档要求：
 *   操作柄类型（HandleType）：点、矢量、距离、角度
 *   控制柄形状（ControlShape）：球形、箭头形(含箭柄)、箭头形(不含箭柄)
 *   四状态风格（HandleStyle）：默认、悬浮、选择、拖拽
 *   显示条件 COND、控制柄规则 CR、导引角色规则 GR
 *
 * 设计原则：
 *   - 规格是纯描述性/声明性数据结构，不持有 VTK actor 或 Qt 对象。
 *   - 渲染/交互代码从规格中读取样式参数，不再硬编码颜色常量。
 *   - COND/CR/GR 用函数指针（std::function）表示，调用方负责绑定具体逻辑。
 */

#ifndef HANDLE_SPEC_H
#define HANDLE_SPEC_H

#include <functional>
#include <QString>
#include <QColor>

// ──────────────────────────────────────────────
// 1. 类型枚举
// ──────────────────────────────────────────────

/** 操作柄类型 */
enum class HandleType {
    Point,    ///< 点操作柄：表示世界空间中的一个位置
    Vector,   ///< 矢量操作柄：表示方向（不可拖拽，除非特别说明）
    Distance, ///< 距离操作柄：可沿轴拖拽改变标量距离
    Angle,    ///< 角度操作柄：可沿弧线拖拽改变旋转角度
};

/** 控制柄形状 */
enum class ControlShape {
    Sphere,         ///< 球形
    ArrowWithShaft, ///< 箭头形（含箭柄）
    ArrowHeadOnly,  ///< 箭头形（不含箭柄）
};

/** 控制柄状态 */
enum class ControlState {
    Default,  ///< 默认状态
    Hover,    ///< 悬浮状态（鼠标悬停）
    Selected, ///< 选择状态（按下但未拖拽）
    Drag,     ///< 拖拽状态（正在拖拽）
};

// ──────────────────────────────────────────────
// 2. 单状态风格（颜色 + 大小因子）
// ──────────────────────────────────────────────

struct HandleStateStyle {
    QColor  color = QColor(64, 140, 242);  ///< 渲染颜色
    double  scaleFactor = 1.0;             ///< 相对于默认几何尺寸的缩放因子（>1 表示放大）
    float   lineWidth = 2.0f;             ///< 仅对含线段的控制柄有效
    double  opacity = 1.0;                ///< 不透明度（0=全透明，1=不透明）
};

// ──────────────────────────────────────────────
// 3. 控制柄规格（一个操作柄可包含多个控制柄，例如"起始球 + 终止箭头"）
// ──────────────────────────────────────────────

struct ControlSpec {
    QString      id;          ///< 控制柄标识符，在同一操作柄内唯一（例如 "start_sphere"）
    ControlShape shape = ControlShape::Sphere;
    bool         draggable = true;  ///< 是否可拖拽（矢量操作柄的控制柄通常为 false）

    HandleStateStyle styleDefault;
    HandleStateStyle styleHover;
    HandleStateStyle styleSelected;
    HandleStateStyle styleDrag;

    /** CR — 控制柄规则（可选）
     *  函数签名：(当前值) -> 新值
     *  例如距离操作柄：限制拖拽范围；角度操作柄：钳制 ±360°。
     *  若为空，则由调用方自行处理约束。
     */
    std::function<double(double rawValue)> cr;

    /** 获取指定状态的样式 */
    const HandleStateStyle& styleFor(ControlState s) const {
        switch (s) {
        case ControlState::Hover:    return styleHover;
        case ControlState::Selected: return styleSelected;
        case ControlState::Drag:     return styleDrag;
        default:                     return styleDefault;
        }
    }
};

// ──────────────────────────────────────────────
// 4. 导引角色规格（不可交互，仅用于可视化）
// ──────────────────────────────────────────────

struct GuideSpec {
    QString id;              ///< 导引角色标识符
    QString description;     ///< 人类可读说明，例如"自曲线簇中心至拉伸起点的距离矢量"

    /** GR — 导引角色规则（可选）
     *  函数签名：void (当前特征参数) → 无返回值
     *  调用方在每次参数更新后调用，以便刷新导引角色的几何或位置。
     */
    std::function<void()> gr;

    /** 导引角色的基础样式（只有一种，不随交互状态变化）*/
    QColor  color = QColor(180, 180, 180);
    double  opacity = 0.4;
    float   lineWidth = 1.5f;
    bool    dashed = false;
};

// ──────────────────────────────────────────────
// 5. 操作柄规格（顶层描述结构）
// ──────────────────────────────────────────────

struct HandleSpec {
    QString    id;           ///< 操作柄唯一标识符，例如 "extrusion_distance"
    HandleType type = HandleType::Distance;
    QString    featureType;  ///< 所属特征类型，例如 "Extrusion"、"Revolve"

    /** COND — 显示条件
     *  函数签名：() -> bool
     *  返回 true 时操作柄显示，否则隐藏。
     *  若为空，默认始终显示。
     */
    std::function<bool()> cond;

    QList<ControlSpec> controls;  ///< 该操作柄包含的所有控制柄
    QList<GuideSpec>   guides;    ///< 该操作柄包含的所有导引角色（可为空）

    /** 检查 COND（若未设置则返回 true） */
    bool isVisible() const {
        return cond ? cond() : true;
    }
};

// ──────────────────────────────────────────────
// 6. 操作柄规格注册表（全局单例，按 id 索引）
// ──────────────────────────────────────────────

class HandleSpecRegistry {
public:
    static HandleSpecRegistry& instance();

    /** 注册一个操作柄规格（重复 id 会覆盖） */
    void registerSpec(const HandleSpec& spec);

    /** 按 id 查找操作柄规格，找不到返回 nullptr */
    const HandleSpec* findSpec(const QString& id) const;

    /** 返回全部已注册规格 */
    const QList<HandleSpec>& allSpecs() const;

private:
    HandleSpecRegistry() = default;
    QList<HandleSpec> m_specs;
};

// ──────────────────────────────────────────────
// 7. 便捷工厂函数（在 handle_spec.cpp 中实现）
//    用于快速构造项目里现有操作柄的规格，并注册到全局表
// ──────────────────────────────────────────────

/** 注册项目中所有操作柄规格（在程序启动时调用一次） */
void registerAllHandleSpecs();

#endif // HANDLE_SPEC_H
