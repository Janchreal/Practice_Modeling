#ifndef RENDERING_MODEL_MODEL_RENDERING_WINDOW_STATE_H
#define RENDERING_MODEL_MODEL_RENDERING_WINDOW_STATE_H

#include "rendering/adapters/occvtkconverter.h"

/** Rendering adapter state used while constructing model presentations. */
class ModelRenderingWindowState {
protected:
    OccShapeToVtkConverter m_occConverter;
};

#endif // RENDERING_MODEL_MODEL_RENDERING_WINDOW_STATE_H
