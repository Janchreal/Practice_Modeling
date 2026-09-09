#ifndef MODEL_HISTORY_SNAPSHOT_H
#define MODEL_HISTORY_SNAPSHOT_H

#include "common/axisdirection.h"
#include "domain/features/featurerecipe.h"
#include "common/modeltype.h"
#include "platformmath.h"

#include <QColor>
#include <QList>
#include <QString>

#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

// Application-level undo data for restoring a modeling history entry.
// The OCC shape remains transitional state until snapshots become serializable.
struct ModelHistorySnapshot {
    int historyIndex = -1;
    ModelType type = CUBOID;
    QString name;
    QColor color;
    double param1 = 0.0;
    double param2 = 0.0;
    double param3 = 0.0;
    TopoDS_Shape occShape;

    FeatureRecipe recipe;
    bool featureRegenerateFailed = false;

    int booleanTargetIndex = -1;
    QList<int> booleanToolIndices;
    int booleanOperationType = -1;
    bool booleanKeepTarget = false;
    bool booleanKeepTool = false;

    bool hasOrigin = false;
    double originX = 0.0;
    double originY = 0.0;
    double originZ = 0.0;
    AxisDirection axisDirection = AxisDirection::Z;
    bool axisReversed = false;
    bool hasCustomVectorDir = false;
    gp_Dir customVectorDir = gp_Dir(0, 0, 1);

    bool visible = true;
};

// Captures a root feature and all downstream states for one undoable change.
struct CascadeUndoRecord {
    QList<int> affectedIndices;
    QList<ModelHistorySnapshot> beforeStates;
    QList<ModelHistorySnapshot> afterStates;
    bool committed = false;
};

#endif // MODEL_HISTORY_SNAPSHOT_H
