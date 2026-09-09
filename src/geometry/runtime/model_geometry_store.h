#ifndef MODEL_GEOMETRY_STORE_H
#define MODEL_GEOMETRY_STORE_H

#include "model_geometry_state.h"

#include <QHash>
#include <QtGlobal>

class ModelGeometryStore {
public:
    ModelGeometryState& ensure(quint64 recordId);
    const ModelGeometryState* find(quint64 recordId) const;

    void remove(quint64 recordId);
    void clear();

private:
    QHash<quint64, ModelGeometryState> states_;
};

#endif // MODEL_GEOMETRY_STORE_H
