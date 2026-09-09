#include "model_display_style.h"

#include <vtkActor.h>
#include <vtkMapper.h>
#include <vtkProperty.h>
#include <vtkVersionMacros.h>

namespace {

void applyBoundaryLineMaterial(vtkProperty* prop,
                               double r,
                               double g,
                               double b,
                               double lineWidth)
{
    if (!prop) {
        return;
    }

    prop->SetRepresentationToWireframe();
    prop->SetColor(r, g, b);
    prop->SetLineWidth(lineWidth);
    prop->SetLighting(false);
    prop->SetAmbient(1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
    prop->RenderLinesAsTubesOff();
}

} // namespace

namespace ModelDisplayStyle {

void applySolidActorMaterial(vtkProperty* prop)
{
    if (!prop) {
        return;
    }

    prop->SetInterpolationToPhong();
    prop->SetAmbient(0.18);
    prop->SetDiffuse(0.82);
    prop->SetSpecular(0.42);
    prop->SetSpecularPower(42);
    prop->SetSpecularColor(1.0, 1.0, 1.0);
    prop->SetLighting(true);
    prop->SetRepresentationToSurface();
    prop->SetEdgeVisibility(0);
    prop->SetEdgeColor(0.0, 0.0, 0.0);
    prop->SetLineWidth(1.4);
}

void configureSolidMapperForBoundaryOutline(vtkMapper* mapper)
{
    if (!mapper) {
        return;
    }

    // factor 必须为 0：factor*DZ 会随相机拉远变大，导致背面轮廓“穿透变完整”。
    // 仅用很小的 units 后推面，减轻与可见棱的共面闪烁。
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0.0, 1.0);
}

bool needsBoundaryOutline(ModelType type)
{
    return type != SKETCH
        && type != DATUM_PLANE
        && type != DATUM_AXIS
        && type != WORK_CSYS
        && type != REFERENCE_CSYS;
}

bool isInteractiveModelType(ModelType type)
{
    return type != DATUM_PLANE
        && type != DATUM_AXIS
        && type != WORK_CSYS
        && type != REFERENCE_CSYS;
}

void applyModelColorOpacity(vtkProperty* prop, const QColor& color, double opacity)
{
    if (!prop) {
        return;
    }

    prop->SetColor(color.redF(), color.greenF(), color.blueF());
    prop->SetOpacity(opacity);
}

void applyDimmedModelAppearance(vtkProperty* prop, const QColor& color, double colorScale, double opacity)
{
    if (!prop) {
        return;
    }

    prop->SetColor(color.redF() * colorScale,
                   color.greenF() * colorScale,
                   color.blueF() * colorScale);
    prop->SetOpacity(opacity);
}

void restoreModelAppearance(vtkProperty* prop, ModelType type, const QColor& color)
{
    if (type == SKETCH) {
        applySketchModelAppearance(prop, color);
        return;
    }

    restoreSolidModelAppearance(prop, color);
}

void restoreSolidModelAppearance(vtkProperty* prop, const QColor& color)
{
    if (!prop) {
        return;
    }

    applySolidActorMaterial(prop);
    applyModelColorOpacity(prop, color, 1.0);
    prop->SetBackfaceCulling(false);
    prop->SetFrontfaceCulling(false);
}

void restoreOpaqueSolidActorAppearance(vtkActor* actor, const QColor& color)
{
    if (!actor) {
        return;
    }

    restoreSolidModelAppearance(actor->GetProperty(), color);
    configureSolidMapperForBoundaryOutline(actor->GetMapper());
#if VTK_MAJOR_VERSION >= 9
    actor->ForceTranslucentOff();
#endif
}

void applySketchModelAppearance(vtkProperty* prop, const QColor& color)
{
    if (!prop) {
        return;
    }

    prop->SetRepresentationToWireframe();
    prop->SetLineWidth(2.0);
    prop->SetLighting(false);
    prop->SetRenderLinesAsTubes(1);
    prop->SetAmbient(1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
    applyModelColorOpacity(prop, color, 1.0);
}

void applySelectedModelAppearance(vtkProperty* prop, const QColor& color, bool isSketch)
{
    if (!prop) {
        return;
    }

    applySolidActorMaterial(prop);
    prop->SetColor(color.redF(), color.greenF(), color.blueF());
    prop->SetOpacity(isSketch ? 1.0 : 0.5);
    prop->SetEdgeVisibility(0);
    prop->SetAmbient(0.26);
    prop->SetDiffuse(0.88);
    prop->SetSpecular(0.48);
    prop->SetSpecularPower(36);
}

void applyHoverModelAppearance(vtkProperty* prop)
{
    if (!prop) {
        return;
    }

    applySolidActorMaterial(prop);
    prop->SetColor(0.0, 0.62, 1.0);
    prop->SetOpacity(0.82);
    prop->SetAmbient(0.30);
    prop->SetDiffuse(0.84);
    prop->SetSpecular(0.55);
    prop->SetSpecularPower(44);
    prop->SetEdgeVisibility(0);
}

void applyBoundaryOutlineStyle(vtkProperty* prop,
                               bool selectedHighlight,
                               bool featureOperationGhostMode)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double lineWidth = 1.6;

    if (selectedHighlight) {
        r = 0.0;
        g = 1.0;
        b = 1.0;
        lineWidth = 2.8;
    } else if (featureOperationGhostMode) {
        r = 1.0;
        g = 0.55;
        b = 0.12;
        lineWidth = 2.4;
    }

    applyBoundaryLineMaterial(prop, r, g, b, lineWidth);
}

void applyHoverBoundaryOutlineStyle(vtkProperty* prop)
{
    applyBoundaryLineMaterial(prop, 0.0, 0.90, 1.0, 2.4);
}

void applyFeaturePickHighlight(vtkActor* actor, bool isLine)
{
    if (!actor) {
        return;
    }

    vtkProperty* prop = actor->GetProperty();
    prop->SetLighting(false);
    prop->SetAmbient(1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
    prop->SetBackfaceCulling(false);
#if VTK_MAJOR_VERSION >= 9
    actor->ForceOpaqueOn();
#endif

    vtkMapper* mapper = actor->GetMapper();
    if (!mapper) {
        return;
    }

    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    if (isLine) {
        mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-24.0, -24.0);
    } else {
        mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-16.0, -16.0);
    }
}

void configureTranslucentSolidMapper(vtkMapper* mapper, double factor, double units)
{
    if (!mapper) {
        return;
    }

    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(factor, units);
}

void applyTranslucentSolidActorAppearance(vtkActor* actor, double opacity)
{
    if (!actor) {
        return;
    }

    vtkProperty* prop = actor->GetProperty();
    prop->SetOpacity(opacity);
    prop->SetBackfaceCulling(false);
    prop->SetFrontfaceCulling(false);
    prop->SetEdgeVisibility(0);
    prop->SetRepresentationToSurface();
#if VTK_MAJOR_VERSION >= 9
    actor->ForceTranslucentOn();
#endif
    configureTranslucentSolidMapper(actor->GetMapper(), 1.0, 1.0);
}

} // namespace ModelDisplayStyle
