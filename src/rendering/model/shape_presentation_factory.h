#ifndef SHAPE_PRESENTATION_FACTORY_H
#define SHAPE_PRESENTATION_FACTORY_H

#include "model_render_state.h"

#include <QColor>

#include <TopoDS_Shape.hxx>
#include <vtkSmartPointer.h>

struct ShapePresentationOptions {
    // Keep display tessellation independent of the model size. The previous
    // defaults were too coarse for zoomed-in intersections.
    static constexpr double kDefaultMeshDeflection = 0.003;
    static constexpr double kDefaultMeshAngle = 0.05;

    QColor color;
    int shapeId = 0;
    double meshDeflection = kDefaultMeshDeflection;
    double meshAngle = kDefaultMeshAngle;
    bool deepCopyPolyData = false;
};

namespace ShapePresentationFactory {

ModelRenderState createSolidModelState(const TopoDS_Shape& shape,
                                       const ShapePresentationOptions& options);

void refreshSolidModelState(ModelRenderState& state,
                            const TopoDS_Shape& shape,
                            const ShapePresentationOptions& options);

} // namespace ShapePresentationFactory

#endif // SHAPE_PRESENTATION_FACTORY_H
