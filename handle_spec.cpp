/**
 * handle_spec.cpp — 操作柄规格注册表实现 + 项目已有操作柄的规格声明
 *
 * 规格与实际交互代码的关系：
 *   - handle_spec 只负责"声明"（COND/CR/GR/样式），不直接持有 VTK actor。
 *   - 实际渲染代码通过 HandleGeom（handle_geometry）创建
 *     vtkSphereSource / vtkArrowSource 几何，并从
 *     HandleSpecRegistry::instance().findSpec(id) 取得规格，
 *     再从 ControlSpec::styleFor(state) 读取颜色/尺寸驱动 actor。
 *   - COND 在 updateXxxHandles() 调用前执行，决定是否创建/显示 actor。
 *   - CR 在拖拽值更新时执行，对原始值做约束并返回合法值。
 *   - GR 在每次参数刷新后执行，更新导引角色几何。
 */

#include "handle_spec.h"

// ──────────────────────────────────────────────
// HandleSpecRegistry 实现
// ──────────────────────────────────────────────

HandleSpecRegistry& HandleSpecRegistry::instance()
{
    static HandleSpecRegistry s_instance;
    return s_instance;
}

void HandleSpecRegistry::registerSpec(const HandleSpec& spec)
{
    for (auto& s : m_specs) {
        if (s.id == spec.id) {
            s = spec;
            return;
        }
    }
    m_specs.append(spec);
}

const HandleSpec* HandleSpecRegistry::findSpec(const QString& id) const
{
    for (const auto& s : m_specs) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

const QList<HandleSpec>& HandleSpecRegistry::allSpecs() const
{
    return m_specs;
}

// ──────────────────────────────────────────────
// 内部工具：构造四状态风格（默认/悬浮/选择/拖拽）
// ──────────────────────────────────────────────

namespace {

// 默认蓝色（操作柄基色）
static const QColor kDefaultBlue(64, 140, 242);

HandleStateStyle makeStyle(QColor color, double scaleFactor, float lineWidth = 2.0f, double opacity = 1.0)
{
    HandleStateStyle s;
    s.color = color;
    s.scaleFactor = scaleFactor;
    s.lineWidth = lineWidth;
    s.opacity = opacity;
    return s;
}

/** 标准四状态：默认蓝/悬浮琥珀黄/选择翠绿/拖拽橙红，尺寸依次加大 */
void applyStandardFourStates(ControlSpec& c, float defaultLineWidth = 2.0f)
{
    c.styleDefault  = makeStyle(QColor( 64, 140, 242), 1.00, defaultLineWidth);
    // 悬浮：高饱和黄 + 明显放大，保证“可操作”反馈一眼可见
    c.styleHover    = makeStyle(QColor(255, 230,  40), 1.22, defaultLineWidth + 0.8f);
    c.styleSelected = makeStyle(QColor( 38, 255,  64), 1.28, defaultLineWidth + 1.0f);
    c.styleDrag     = makeStyle(QColor(255,  64,  26), 1.32, defaultLineWidth + 1.3f);
}

/** 导引角色标准样式：浅蓝半透明虚线 */
GuideSpec makeGuideSpec(const QString& id, const QString& desc, bool dashed = false)
{
    GuideSpec g;
    g.id = id;
    g.description = desc;
    g.color = QColor(115, 191, 255);
    g.opacity = 0.40;
    g.lineWidth = 1.5f;
    g.dashed = dashed;
    return g;
}

} // namespace

// ──────────────────────────────────────────────
// 各操作柄规格构造
// ──────────────────────────────────────────────

/**
 * R-Handle-Extrusion-Distance
 * 拉伸特征距离操作柄（距离操作柄）
 *
 * COND: 拉伸对话框已打开 且 已选择截面 且 未处于"结果预览锁定"状态
 * CR  : 起始球 → 起始距离不得大于终止距离；终止箭头 → 终止距离不得小于起始距离
 * GR  : 导引线：自截面中心起点沿拉伸方向延伸至起始平面（可视化"起始偏移"语义）
 */
static HandleSpec buildExtrusionDistanceSpec()
{
    HandleSpec h;
    h.id = QStringLiteral("extrusion_distance");
    h.type = HandleType::Distance;
    h.featureType = QStringLiteral("Extrusion");

    // COND: 由 Widget::updateExtrusionHandles() 外部判断，这里保留占位符（始终 true）
    // 真正的条件绑定在 registerAllHandleSpecs() 之后由 Widget 覆写
    h.cond = []{ return true; };

    // 控制柄 1：起始球（球形，可拖拽）
    {
        ControlSpec c;
        c.id = QStringLiteral("start_sphere");
        c.shape = ControlShape::Sphere;
        c.draggable = true;
        applyStandardFourStates(c, 2.0f);
        // CR：起始距离不超过终止距离（实际约束由 setStartDistance 内部保证，这里记录语义）
        c.cr = [](double v) { return v; }; // passthrough，Widget 层面做具体约束
        h.controls.append(c);
    }

    // 控制柄 2：终止箭头（箭头形含箭柄，可拖拽）
    {
        ControlSpec c;
        c.id = QStringLiteral("end_arrow");
        c.shape = ControlShape::ArrowWithShaft;
        c.draggable = true;
        applyStandardFourStates(c, 2.0f);
        c.cr = [](double v) { return v; };
        h.controls.append(c);
    }

    // 导引角色：截面中心→起始面 距离矢量（标识"起始偏移"的几何意义）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("start_offset_vector"),
            QStringLiteral("自截面中心至拉伸起点的距离矢量"),
            /*dashed=*/true);
        // GR：占位符，实际由 Widget::updateExtrusionHandles 调用时内联更新 actor
        g.gr = []{ };
        h.guides.append(g);
    }

    return h;
}

/**
 * R-Handle-Revolve-Angle
 * 旋转特征角度操作柄（角度操作柄）
 *
 * COND: 旋转对话框已打开 且 已选择截面 且 未处于"结果预览锁定"状态
 * CR  : 起始球 → |sweep| ≤ 360°；终止箭头 → 同上（连续解包防止过 ±180° 跳变）
 * GR  : 圆弧（旋转范围可视化）+ 径向辅助线（起/终角度参考）
 */
static HandleSpec buildRevolveAngleSpec()
{
    HandleSpec h;
    h.id = QStringLiteral("revolve_angle");
    h.type = HandleType::Angle;
    h.featureType = QStringLiteral("Revolve");

    h.cond = []{ return true; }; // 同上，由 Widget 覆写绑定实际条件

    // 控制柄 1：起始球（球形，可拖拽，对应起始角度）
    {
        ControlSpec c;
        c.id = QStringLiteral("start_sphere");
        c.shape = ControlShape::Sphere;
        c.draggable = true;
        applyStandardFourStates(c, 2.0f);
        // CR：sweep 范围 [-360, +360]
        c.cr = [](double v) { return v; };
        h.controls.append(c);
    }

    // 控制柄 2：终止箭头（箭头形含箭柄，切向，可拖拽，对应终止角度）
    {
        ControlSpec c;
        c.id = QStringLiteral("end_arrow");
        c.shape = ControlShape::ArrowWithShaft;
        c.draggable = true;
        applyStandardFourStates(c, 2.0f);
        // EndArrow 默认偏绿色以区分 StartSphere
        c.styleDefault.color = QColor(51, 217, 89);
        c.cr = [](double v) { return v; };
        h.controls.append(c);
    }

    // 导引角色 1：旋转弧（可视化旋转角度范围）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("revolve_arc"),
            QStringLiteral("旋转角度范围弧（起始→终止角度的扫掠区域）"),
            /*dashed=*/false);
        g.opacity = 0.35;
        g.gr = []{ };
        h.guides.append(g);
    }

    // 导引角色 2：径向辅助线（起始角度参考）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("start_radial_line"),
            QStringLiteral("起始角度参考线（旋转中心→起始点）"),
            /*dashed=*/false);
        g.lineWidth = 1.5f;
        g.gr = []{ };
        h.guides.append(g);
    }

    // 导引角色 3：径向辅助线（终止角度参考）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("end_radial_line"),
            QStringLiteral("终止角度参考线（旋转中心→终止点）"),
            /*dashed=*/false);
        g.lineWidth = 1.5f;
        g.gr = []{ };
        h.guides.append(g);
    }

    // 导引角色 4：旋转中心球（不可交互，标识旋转轴足点）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("revolve_center"),
            QStringLiteral("旋转中心（轴在截面平面上的投影点）"));
        g.color = QColor(255, 140, 38);
        g.opacity = 0.85;
        g.gr = []{ };
        h.guides.append(g);
    }

    return h;
}

/**
 * R-Handle-Cuboid-Length/Width/Height
 * 长方体三轴尺寸操作柄（距离操作柄）
 *
 * COND: 长方体交互式预览激活
 * CR  : 各轴尺寸不小于 kCuboidMinDim（0.1）
 * GR  : 轴线 + 轴端箭头 + 轴标签（X/Y/Z），原点球（均不可交互）
 */
static HandleSpec buildCuboidAxisSpec(int axisIndex)
{
    static const char* kAxisIds[] = { "cuboid_length", "cuboid_width", "cuboid_height" };
    static const char* kAxisLabels[] = { "长度轴（X方向）", "宽度轴（Y方向）", "高度轴（Z方向）" };

    HandleSpec h;
    h.id = QString::fromLatin1(kAxisIds[axisIndex]);
    h.type = HandleType::Distance;
    h.featureType = QStringLiteral("Cuboid");
    h.cond = []{ return true; };

    // 控制柄：轴端点（箭头不含箭柄——轴端是球形+独立箭头，这里只建模"可拖拽端点"）
    {
        ControlSpec c;
        c.id = QStringLiteral("axis_tip");
        c.shape = ControlShape::ArrowHeadOnly;
        c.draggable = true;
        applyStandardFourStates(c, 1.8f);
        c.cr = [](double v) { return v < 0.1 ? 0.1 : v; }; // CR: 最小尺寸约束
        h.controls.append(c);
    }

    // 控制柄：原点点操作柄（球形）；COND=已选点，CR=始终位于原点
    if (axisIndex == 0) {
        ControlSpec c;
        c.id = QStringLiteral("origin_point");
        c.shape = ControlShape::Sphere;
        c.draggable = true;
        applyStandardFourStates(c, 1.0f);
        c.cr = [](double v) { return v; };
        h.controls.append(c);
    }

    // 导引角色：轴线段
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("axis_line"),
            QString::fromUtf8(kAxisLabels[axisIndex]));
        g.color = QColor(140, 174, 224);
        g.lineWidth = 1.8f;
        g.gr = []{ };
        h.guides.append(g);
    }

    return h;
}

/**
 * R-Handle-Point
 * 点操作柄（用于点捕捉拾取，例如基准点/原点选择）
 *
 * COND: 处于 PointSelection 模式
 * CR  : 无约束（拾取即确认）
 * GR  : 无导引角色
 */
static HandleSpec buildPointSpec()
{
    HandleSpec h;
    h.id = QStringLiteral("point_selection");
    h.type = HandleType::Point;
    h.featureType = QStringLiteral("Generic");
    h.cond = []{ return true; };

    ControlSpec c;
    c.id = QStringLiteral("pick_sphere");
    c.shape = ControlShape::Sphere;
    c.draggable = false; // 点操作柄：点击确认，不支持拖拽
    c.styleDefault  = makeStyle(QColor( 64, 140, 242), 1.00);
    c.styleHover    = makeStyle(QColor(255, 217,  51), 1.15);
    c.styleSelected = makeStyle(QColor( 38, 255,  64), 1.25);
    c.styleDrag     = c.styleSelected; // 不拖拽，选中即"确认"
    h.controls.append(c);

    return h;
}

/**
 * R-Handle-Vector
 * 矢量操作柄（方向拾取，不可拖拽）
 *
 * COND: 处于 VectorDialogPickDirection / VectorDialogPickStartPoint / EndPoint 等模式
 * CR  : 无（方向由拾取逻辑决定，不由拖拽产生）
 * GR  : 矢量箭头（不含箭柄）预览当前方向
 */
static HandleSpec buildVectorSpec()
{
    HandleSpec h;
    h.id = QStringLiteral("vector_direction");
    h.type = HandleType::Vector;
    h.featureType = QStringLiteral("Generic");
    h.cond = []{ return true; };

    // 矢量操作柄的控制柄：悬浮预览箭头（不可拖拽）
    {
        ControlSpec c;
        c.id = QStringLiteral("direction_arrow");
        c.shape = ControlShape::ArrowHeadOnly;
        c.draggable = false; // 文档规定：矢量操作柄不可拖拽
        c.styleDefault  = makeStyle(QColor(64,  140, 242), 1.00);
        c.styleHover    = makeStyle(QColor(255, 217,  51), 1.12);
        c.styleSelected = makeStyle(QColor( 38, 255,  64), 1.20);
        c.styleDrag     = c.styleSelected; // 不拖拽，选中即确认
        h.controls.append(c);
    }

    // 导引角色：箭头预览（已确认的方向，颜色偏绿以区分悬浮预览）
    {
        GuideSpec g = makeGuideSpec(
            QStringLiteral("confirmed_direction"),
            QStringLiteral("已确认的矢量方向可视化（蓝色箭头）"));
        g.color = QColor(64, 160, 242);
        g.opacity = 0.85;
        g.gr = []{ };
        h.guides.append(g);
    }

    return h;
}

// ──────────────────────────────────────────────
// 公共入口：注册全部规格
// ──────────────────────────────────────────────

void registerAllHandleSpecs()
{
    auto& reg = HandleSpecRegistry::instance();
    reg.registerSpec(buildExtrusionDistanceSpec());
    reg.registerSpec(buildRevolveAngleSpec());
    reg.registerSpec(buildCuboidAxisSpec(0)); // cuboid_length
    reg.registerSpec(buildCuboidAxisSpec(1)); // cuboid_width
    reg.registerSpec(buildCuboidAxisSpec(2)); // cuboid_height
    reg.registerSpec(buildPointSpec());
    reg.registerSpec(buildVectorSpec());
}
