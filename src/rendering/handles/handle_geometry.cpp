/**
 * handle_geometry.cpp — 控制柄三维几何工厂实现
 */

#include "handle_geometry.h"

#include <algorithm>
#include <cmath>

#include <gp_Vec.hxx>

#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkLineSource.h>

namespace HandleGeom {
namespace {

HandleStateStyle fallbackStyle(ControlState state)
{
    HandleStateStyle s;
    switch (state) {
    case ControlState::Hover:
        s.color = QColor(255, 230, 40);
        s.scaleFactor = 1.22;
        s.lineWidth = 2.8f;
        break;
    case ControlState::Selected:
        s.color = QColor(38, 255, 64);
        s.scaleFactor = 1.28;
        s.lineWidth = 3.0f;
        break;
    case ControlState::Drag:
        s.color = QColor(255, 64, 26);
        s.scaleFactor = 1.32;
        s.lineWidth = 3.3f;
        break;
    default:
        s.color = QColor(64, 140, 242);
        s.scaleFactor = 1.0;
        s.lineWidth = 2.0f;
        break;
    }
    s.opacity = 1.0;
    return s;
}

} // namespace

vtkSmartPointer<vtkArrowSource> makeArrowSource(ControlShape shape, const ArrowParams& params)
{
    auto arrow = vtkSmartPointer<vtkArrowSource>::New();

    ArrowParams p = params;
    if (shape == ControlShape::ArrowHeadOnly || shape == ControlShape::Cone) {
        // 不含箭柄：整段为单位圆锥；箭柄半径置 0
        p.tipLength = 1.0;
        p.shaftRadius = 0.0;
    } else if (shape == ControlShape::Sphere) {
        // 误用时仍产出含箭柄箭头，调用方应改用 makeSphereSource
        p = defaultShaftArrowParams();
    }

    arrow->SetTipLength(p.tipLength);
    arrow->SetTipRadius(p.tipRadius);
    arrow->SetShaftRadius(p.shaftRadius);
    arrow->SetTipResolution(std::max(3, p.tipResolution));
    arrow->SetShaftResolution(std::max(3, p.shaftResolution));
    arrow->InvertOff();
    return arrow;
}

vtkSmartPointer<vtkSphereSource> makeSphereSource(const SphereParams& params)
{
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetCenter(0.0, 0.0, 0.0);
    sphere->SetRadius(std::max(1e-6, params.radius));
    sphere->SetThetaResolution(std::max(4, params.thetaResolution));
    sphere->SetPhiResolution(std::max(4, params.phiResolution));
    sphere->LatLongTessellationOff();
    return sphere;
}

vtkSmartPointer<vtkConeSource> makeConeWithBaseCenter(
    const gp_Pnt& baseCenter,
    const gp_Dir& dir,
    const ConeParams& params)
{
    auto cone = vtkSmartPointer<vtkConeSource>::New();
    const double height = std::max(1e-6, params.height);
    const gp_Pnt center = baseCenter.Translated(gp_Vec(dir) * (height * 0.5));

    cone->SetCenter(center.X(), center.Y(), center.Z());
    cone->SetDirection(dir.X(), dir.Y(), dir.Z());
    cone->SetHeight(height);
    cone->SetRadius(std::max(1e-6, params.radius));
    cone->SetResolution(std::max(3, params.resolution));
    cone->SetCapping(params.capping ? 1 : 0);
    return cone;
}

vtkSmartPointer<vtkTubeFilter> makeCylinderBetween(
    const gp_Pnt& a,
    const gp_Pnt& b,
    const CylinderParams& params)
{
    gp_Pnt end = b;
    if (a.Distance(b) < 1e-9) {
        end = a.Translated(gp_Vec(1e-6, 0.0, 0.0));
    }

    auto line = vtkSmartPointer<vtkLineSource>::New();
    line->SetPoint1(a.X(), a.Y(), a.Z());
    line->SetPoint2(end.X(), end.Y(), end.Z());

    auto tube = vtkSmartPointer<vtkTubeFilter>::New();
    tube->SetInputConnection(line->GetOutputPort());
    tube->SetRadius(std::max(1e-6, params.radius));
    tube->SetNumberOfSides(std::max(3, params.resolution));
    if (params.capping) {
        tube->CappingOn();
    } else {
        tube->CappingOff();
    }
    return tube;
}

void applyArrowOrientation(vtkTransform* transform,
                           double ox, double oy, double oz,
                           double dx, double dy, double dz,
                           double length)
{
    if (!transform) return;
    transform->Identity();
    transform->Translate(ox, oy, oz);

    const double from[3] = {1.0, 0.0, 0.0};
    double to[3] = {dx, dy, dz};
    const double toLen = vtkMath::Norm(to);
    if (toLen < 1e-12) {
        transform->Scale(length, length, length);
        return;
    }
    to[0] /= toLen;
    to[1] /= toLen;
    to[2] /= toLen;

    double axis[3] = {0.0, 0.0, 0.0};
    vtkMath::Cross(from, to, axis);
    const double axisLen = vtkMath::Norm(axis);
    const double dot = std::max(-1.0, std::min(1.0, vtkMath::Dot(from, to)));

    if (axisLen > 1e-12) {
        axis[0] /= axisLen;
        axis[1] /= axisLen;
        axis[2] /= axisLen;
        const double angleDeg = vtkMath::DegreesFromRadians(std::acos(dot));
        transform->RotateWXYZ(angleDeg, axis[0], axis[1], axis[2]);
    } else if (dot < 0.0) {
        transform->RotateWXYZ(180.0, 0.0, 1.0, 0.0);
    }

    transform->Scale(length, length, length);
}

void applyArrowOrientation(vtkTransform* transform,
                           const gp_Pnt& origin,
                           const gp_Dir& dir,
                           double length)
{
    applyArrowOrientation(transform,
                          origin.X(), origin.Y(), origin.Z(),
                          dir.X(), dir.Y(), dir.Z(),
                          length);
}

vtkSmartPointer<vtkTransformPolyDataFilter> makeOrientedArrow(
    ControlShape shape,
    const gp_Pnt& origin,
    const gp_Dir& dir,
    double length,
    const ArrowParams& params)
{
    auto arrow = makeArrowSource(shape, params);
    auto xf = vtkSmartPointer<vtkTransform>::New();
    applyArrowOrientation(xf, origin, dir, length);
    auto tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    tf->SetTransform(xf);
    tf->SetInputConnection(arrow->GetOutputPort());
    return tf;
}

vtkSmartPointer<vtkGlyph3D> makeArrowGlyph(
    ControlShape shape,
    vtkPolyData* inputPoints,
    double scaleFactor,
    const ArrowParams& params)
{
    auto glyph = vtkSmartPointer<vtkGlyph3D>::New();
    if (!inputPoints) return glyph;

    auto arrow = makeArrowSource(shape, params);
    glyph->SetSourceConnection(arrow->GetOutputPort());
    glyph->SetInputData(inputPoints);
    glyph->SetScaleModeToDataScalingOff();
    glyph->SetScaleFactor(scaleFactor);
    glyph->OrientOn();
    glyph->SetVectorModeToUseVector();
    return glyph;
}

ControlState resolveControlState(bool dragging, bool selected, bool hovered)
{
    if (dragging) return ControlState::Drag;
    if (selected) return ControlState::Selected;
    if (hovered) return ControlState::Hover;
    return ControlState::Default;
}

void applyStateStyle(vtkProperty* prop, const HandleStateStyle& style)
{
    if (!prop) return;
    prop->SetColor(style.color.redF(), style.color.greenF(), style.color.blueF());
    prop->SetOpacity(style.opacity);
    prop->SetLineWidth(style.lineWidth);
    // 提高环境光占比，避免光照把悬浮黄/选择绿“冲淡”成几乎不变
    prop->SetLighting(true);
    prop->SetAmbient(0.88);
    prop->SetDiffuse(0.35);
    prop->SetSpecular(0.05);
    prop->SetSpecularPower(10.0);
}

void applyStateStyle(vtkActor* actor,
                     const HandleStateStyle& style,
                     double baseWorldScale,
                     bool applyActorScale)
{
    if (!actor) return;
    applyStateStyle(actor->GetProperty(), style);
    if (applyActorScale) {
        const double s = baseWorldScale * style.scaleFactor;
        actor->SetScale(s, s, s);
    }
}

const ControlSpec* findControlSpec(const QString& handleId, const QString& controlId)
{
    const HandleSpec* hs = HandleSpecRegistry::instance().findSpec(handleId);
    if (!hs) return nullptr;
    for (const ControlSpec& c : hs->controls) {
        if (c.id == controlId) return &c;
    }
    return nullptr;
}

HandleStateStyle styleForControl(const QString& handleId,
                                 const QString& controlId,
                                 ControlState state)
{
    if (const ControlSpec* c = findControlSpec(handleId, controlId)) {
        return c->styleFor(state);
    }
    return fallbackStyle(state);
}

} // namespace HandleGeom
