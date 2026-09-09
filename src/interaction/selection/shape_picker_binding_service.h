#ifndef INTERACTION_SELECTION_SHAPE_PICKER_BINDING_SERVICE_H
#define INTERACTION_SELECTION_SHAPE_PICKER_BINDING_SERVICE_H

#include <functional>

class IVtkTools_ShapePicker;
class vtkRenderer;

struct ShapePickerBindingContext {
    IVtkTools_ShapePicker* picker = nullptr;
    vtkRenderer* renderer = nullptr;
    std::function<void()> prepareBindings;
    std::function<void()> updateDataSources;
};

/** Rebinds a shape picker to the active renderer without knowing the window. */
class ShapePickerBindingService {
public:
    static void refresh(const ShapePickerBindingContext& context,
                        double tolerance,
                        bool updateDataSources);
};

#endif // INTERACTION_SELECTION_SHAPE_PICKER_BINDING_SERVICE_H
