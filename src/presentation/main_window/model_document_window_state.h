#ifndef PRESENTATION_MAIN_WINDOW_MODEL_DOCUMENT_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_MODEL_DOCUMENT_WINDOW_STATE_H

#include "application/history/commandmanager.h"
#include "application/history/model_document.h"
#include "geometry/runtime/model_geometry_store.h"
#include "rendering/model/model_render_store.h"

#include <QList>

/** Document model, runtime stores, and undo state shared by window adapters. */
class ModelDocumentWindowState {
protected:
    ModelDocument modelDocument_;
    ModelGeometryStore geometryStore_;
    ModelRenderStore renderStore_;
    QList<ModelingHistory>& historyList;
    CommandManager commandManager_;
    int cascadeUpdateGuard_ = 0;
    static int shapeIDCounter;

    ModelDocumentWindowState()
        : historyList(modelDocument_.histories())
    {
    }
};

#endif // PRESENTATION_MAIN_WINDOW_MODEL_DOCUMENT_WINDOW_STATE_H
