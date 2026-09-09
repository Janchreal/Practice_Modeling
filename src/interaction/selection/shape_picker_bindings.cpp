#include "main_window.h"

void Widget::refreshShapePickerBindingsForCurrentContext(double tolerance, bool updateDataSources)
{
    if (!shapePicker || !renderer) {
        return;
    }

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(tolerance);
    prepareShapePickerBindingsForCurrentContext();

    if (!updateDataSources) {
        return;
    }

    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).shapeDataSource) {
            renderStateFor(historyList[i]).shapeDataSource->Modified();
            renderStateFor(historyList[i]).shapeDataSource->Update();
        }
    }
}
