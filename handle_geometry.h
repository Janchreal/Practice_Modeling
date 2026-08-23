/**
 * handle_geometry.h — 控制柄三维几何工厂
 *
 * 对应文档 3.1.4：控制柄样式是三维 PolyData，而非二维图标。
 *   - 球形          → vtkSphereSource
 *   - 箭头形(含箭柄) → vtkArrowSource（圆柱箭柄 + 圆锥箭头）
 *   - 箭头形(不含箭柄) → vtkArrowSource（TipLength=1, ShaftRadius=0）
 *
 * vtkArrowSource 默认沿 +X；用 vtkTransform + vtkTransformPolyDataFilter
 * （或 Actor::UserTransform）旋转到目标方向。多点位可用 makeArrowGlyph 模板。
 *
 * 四状态（默认/悬浮/选择/拖拽）通过 HandleStateStyle 驱动颜色、不透明度与缩放。
 */

#ifndef HANDLE_GEOMETRY_H
#define HANDLE_GEOMETRY_H

#include "handle_spec.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <vtkArrowSource.h>
#include <vtkGlyph3D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

class vtkActor;
class vtkPolyData;
class vtkProperty;

namespace HandleGeom {

/** vtkArrowSource 几何参数（相对单位长度 1 的箭头） */
struct ArrowParams {
    double tipLength = 0.35;   ///< 圆锥箭头长度占比（VTK 默认 0.35）
    double tipRadius = 0.10;   ///< 圆锥底面半径（VTK 默认 0.1）
    double shaftRadius = 0.03; ///< 圆柱箭柄半径（VTK 默认 0.03）
    int tipResolution = 16;    ///< 箭头分辨率（默认提高以改善观感）
    int shaftResolution = 16;  ///< 箭柄分辨率
};

/** vtkSphereSource 几何参数 */
struct SphereParams {
    double radius = 0.10;
    int thetaResolution = 24;
    int phiResolution = 24;
};

/** 交互距离/角度手柄推荐参数（屏幕恒定尺寸，再乘 overlayWorldScale） */
inline ArrowParams defaultShaftArrowParams()
{
    ArrowParams p;
    p.tipLength = 0.30;
    p.tipRadius = 0.08;
    p.shaftRadius = 0.028;
    return p;
}

/** 仅箭头锥体（矢量操作柄 / 轴端尖头） */
inline ArrowParams defaultHeadOnlyArrowParams()
{
    ArrowParams p;
    p.tipLength = 1.0;
    p.tipRadius = 0.12;
    p.shaftRadius = 0.0;
    return p;
}

inline SphereParams defaultHandleSphereParams(double radius = 0.14)
{
    SphereParams p;
    p.radius = radius;
    return p;
}

// ── 几何源 ──────────────────────────────────────────────

/** 按 ControlShape 配置 vtkArrowSource；含箭柄 / 不含箭柄均走同一类 */
vtkSmartPointer<vtkArrowSource> makeArrowSource(ControlShape shape,
                                                const ArrowParams& params = ArrowParams());

vtkSmartPointer<vtkSphereSource> makeSphereSource(const SphereParams& params = SphereParams());

// ── 方向变换（+X → 目标方向） ────────────────────────────

void applyArrowOrientation(vtkTransform* transform,
                           double ox, double oy, double oz,
                           double dx, double dy, double dz,
                           double length);

void applyArrowOrientation(vtkTransform* transform,
                           const gp_Pnt& origin,
                           const gp_Dir& dir,
                           double length);

/** 生成已定向的箭头 PolyData 管线（TransformPolyDataFilter 输出） */
vtkSmartPointer<vtkTransformPolyDataFilter> makeOrientedArrow(
    ControlShape shape,
    const gp_Pnt& origin,
    const gp_Dir& dir,
    double length,
    const ArrowParams& params = ArrowParams());

/**
 * 以 vtkGlyph3D 将箭头模板放置到点集上（文档推荐的多点位用法）。
 * inputPoints 需含点坐标；可选 Vectors 数组指定方向，否则沿 +X。
 */
vtkSmartPointer<vtkGlyph3D> makeArrowGlyph(
    ControlShape shape,
    vtkPolyData* inputPoints,
    double scaleFactor = 1.0,
    const ArrowParams& params = ArrowParams());

// ── 四状态样式 ──────────────────────────────────────────

ControlState resolveControlState(bool dragging, bool selected, bool hovered);

/** 仅写颜色 / 线宽 / 不透明度 */
void applyStateStyle(vtkProperty* prop, const HandleStateStyle& style);

/** 写属性，并按 style.scaleFactor * baseWorldScale 设置 Actor 均匀缩放 */
void applyStateStyle(vtkActor* actor,
                     const HandleStateStyle& style,
                     double baseWorldScale = 1.0,
                     bool applyActorScale = true);

/** 从注册表取控制柄规格；找不到返回 nullptr */
const ControlSpec* findControlSpec(const QString& handleId, const QString& controlId);

/** 便捷：取某控制柄在指定状态下的样式；无规格时回退标准蓝系四状态 */
HandleStateStyle styleForControl(const QString& handleId,
                                 const QString& controlId,
                                 ControlState state);

} // namespace HandleGeom

#endif // HANDLE_GEOMETRY_H
