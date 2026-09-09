#include "intersection_render_store.h"

#include <algorithm>

IntersectionRenderStore::Key IntersectionRenderStore::canonicalKey(
    quint64 firstId, quint64 secondId)
{
    if (firstId <= secondId) {
        return qMakePair(firstId, secondId);
    }
    return qMakePair(secondId, firstId);
}

IntersectionRenderState& IntersectionRenderStore::ensure(
    quint64 firstId, quint64 secondId)
{
    return states_[canonicalKey(firstId, secondId)];
}

IntersectionRenderState* IntersectionRenderStore::find(
    quint64 firstId, quint64 secondId)
{
    const Key key = canonicalKey(firstId, secondId);
    auto it = states_.find(key);
    return it == states_.end() ? nullptr : &it.value();
}

const IntersectionRenderState* IntersectionRenderStore::find(
    quint64 firstId, quint64 secondId) const
{
    const Key key = canonicalKey(firstId, secondId);
    const auto it = states_.constFind(key);
    return it == states_.constEnd() ? nullptr : &it.value();
}

void IntersectionRenderStore::remove(quint64 firstId, quint64 secondId)
{
    states_.remove(canonicalKey(firstId, secondId));
}

QList<IntersectionRenderStore::Key> IntersectionRenderStore::keysForRecord(
    quint64 recordId) const
{
    QList<Key> keys;
    for (auto it = states_.constBegin(); it != states_.constEnd(); ++it) {
        if (it.key().first == recordId || it.key().second == recordId) {
            keys.append(it.key());
        }
    }
    return keys;
}

void IntersectionRenderStore::clear()
{
    states_.clear();
}
