#ifndef MODEL_RENDER_STORE_H
#define MODEL_RENDER_STORE_H

#include "model_render_state.h"

#include <QHash>
#include <QtGlobal>

class ModelRenderStore {
public:
    ModelRenderState& ensure(quint64 recordId);
    const ModelRenderState* find(quint64 recordId) const;

    void remove(quint64 recordId);
    void clear();

private:
    QHash<quint64, ModelRenderState> states_;
};

#endif // MODEL_RENDER_STORE_H
