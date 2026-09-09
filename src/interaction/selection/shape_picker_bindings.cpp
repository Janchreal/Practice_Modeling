#include "shape_picker_binding_service.h"

#include <IVtkTools_ShapePicker.hxx>
#include <vtkRenderer.h>

void ShapePickerBindingService::refresh(const ShapePickerBindingContext& context,
                                        double tolerance,
                                        bool updateDataSources)
{
    if (!context.picker || !context.renderer) {
        return;
    }

    context.picker->SetRenderer(context.renderer);
    context.picker->SetTolerance(tolerance);

    if (context.prepareBindings) {
        context.prepareBindings();
    }

    if (updateDataSources && context.updateDataSources) {
        context.updateDataSources();
    }
}
