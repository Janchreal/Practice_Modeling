#ifndef SHAPE_PRESENTATION_FACTORY_H
#define SHAPE_PRESENTATION_FACTORY_H

#include "model_render_state.h"

#include <QColor>

#include <TopoDS_Shape.hxx>
#include <vtkSmartPointer.h>

struct ShapePresentationOptions {
    QColor color;
    int shapeId = 0;
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
