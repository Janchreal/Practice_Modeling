// Main-window adapter for the generic shape-picker binding service.
#include "main_window.h"

#include "interaction/selection/shape_picker_binding_service.h"

void Widget::refreshShapePickerBindingsForCurrentContext(double tolerance,
                                                          bool updateDataSources)
{
    ShapePickerBindingContext context;
    context.picker = shapePicker.GetPointer();
    context.renderer = renderer.GetPointer();
    context.prepareBindings = [this]() {
        prepareShapePickerBindingsForCurrentContext();
    };
    context.updateDataSources = [this]() {
        for (int i = 0; i < historyList.size(); ++i) {
            if (renderStateFor(historyList[i]).shapeDataSource) {
                renderStateFor(historyList[i]).shapeDataSource->Modified();
                renderStateFor(historyList[i]).shapeDataSource->Update();
            }
        }
    };

    ShapePickerBindingService::refresh(context, tolerance, updateDataSources);
}
