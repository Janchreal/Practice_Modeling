#include "model_geometry_store.h"

ModelGeometryState& ModelGeometryStore::ensure(quint64 recordId)
{
    return states_[recordId];
}

const ModelGeometryState* ModelGeometryStore::find(quint64 recordId) const
{
    const auto it = states_.constFind(recordId);
    return it == states_.constEnd() ? nullptr : &it.value();
}

void ModelGeometryStore::remove(quint64 recordId)
{
    states_.remove(recordId);
}

void ModelGeometryStore::clear()
{
    states_.clear();
}
