#include "model_render_store.h"

ModelRenderState& ModelRenderStore::ensure(quint64 recordId)
{
    return states_[recordId];
}

const ModelRenderState* ModelRenderStore::find(quint64 recordId) const
{
    const auto it = states_.constFind(recordId);
    return it == states_.constEnd() ? nullptr : &it.value();
}

void ModelRenderStore::remove(quint64 recordId)
{
    states_.remove(recordId);
}

void ModelRenderStore::clear()
{
    states_.clear();
}
