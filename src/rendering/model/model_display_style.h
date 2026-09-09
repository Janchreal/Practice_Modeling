#ifndef MODEL_DISPLAY_STYLE_H
#define MODEL_DISPLAY_STYLE_H

#include "common/modeltype.h"

#include <QColor>

class vtkProperty;
class vtkActor;
class vtkMapper;

namespace ModelDisplayStyle {

bool needsBoundaryOutline(ModelType type);
bool isInteractiveModelType(ModelType type);

void applySolidActorMaterial(vtkProperty* prop);
void configureSolidMapperForBoundaryOutline(vtkMapper* mapper);

void applyModelColorOpacity(vtkProperty* prop, const QColor& color, double opacity);
void applyDimmedModelAppearance(vtkProperty* prop, const QColor& color, double colorScale, double opacity);
void restoreModelAppearance(vtkProperty* prop, ModelType type, const QColor& color);
void restoreSolidModelAppearance(vtkProperty* prop, const QColor& color);
void restoreOpaqueSolidActorAppearance(vtkActor* actor, const QColor& color);
void applySketchModelAppearance(vtkProperty* prop, const QColor& color);
void applySelectedModelAppearance(vtkProperty* prop, const QColor& color, bool isSketch);
void applyHoverModelAppearance(vtkProperty* prop);
void applyBoundaryOutlineStyle(vtkProperty* prop, bool selectedHighlight, bool featureOperationGhostMode);
void applyHoverBoundaryOutlineStyle(vtkProperty* prop);
void applyFeaturePickHighlight(vtkActor* actor, bool isLine);
void configureTranslucentSolidMapper(vtkMapper* mapper, double factor, double units);
void applyTranslucentSolidActorAppearance(vtkActor* actor, double opacity);

} // namespace ModelDisplayStyle

#endif // MODEL_DISPLAY_STYLE_H
