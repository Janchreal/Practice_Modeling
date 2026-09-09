#include "main_window.h"

ModelGeometryState& Widget::geometryStateFor(const ModelingHistory& record)
{
    return geometryStore_.ensure(record.id);
}

const ModelGeometryState& Widget::geometryStateFor(const ModelingHistory& record) const
{
    static const ModelGeometryState emptyState;
    const ModelGeometryState* state = geometryStore_.find(record.id);
    return state ? *state : emptyState;
}

ModelRenderState& Widget::renderStateFor(const ModelingHistory& record)
{
    return renderStore_.ensure(record.id);
}

const ModelRenderState& Widget::renderStateFor(const ModelingHistory& record) const
{
    static const ModelRenderState emptyState;
    const ModelRenderState* state = renderStore_.find(record.id);
    return state ? *state : emptyState;
}

void Widget::removeRuntimeStateFor(const ModelingHistory& record)
{
    if (record.id == 0) {
        return;
    }
    geometryStore_.remove(record.id);
    renderStore_.remove(record.id);
}
