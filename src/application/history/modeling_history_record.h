#ifndef MODELING_HISTORY_RECORD_H
#define MODELING_HISTORY_RECORD_H

#include "common/axisdirection.h"
#include "domain/features/featurerecipe.h"
#include "common/modeltype.h"

#include <QColor>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <gp_Dir.hxx>

// Application history entry for one model in the document.
// It is persisted as part of the document, but it is not a domain entity.
struct ModelingHistory {
    quint64 id = 0;
    ModelType type = CUBOID;
    QString name;
    QDateTime timestamp;
    QColor color;
    double param1 = 0.0;
    double param2 = 0.0;
    double param3 = 0.0;
    int booleanTargetIndex = -1;
    QList<int> booleanToolIndices;
    int booleanOperationType = -1;
    bool booleanKeepTarget = false;
    bool booleanKeepTool = false;

    FeatureRecipe recipe;
    bool featureRegenerateFailed = false;

    bool hasOrigin = false;
    double originX = 0.0, originY = 0.0, originZ = 0.0;
    AxisDirection axisDirection = AxisDirection::Z;
    bool axisReversed = false;

    bool hasCustomVectorDir = false;
    gp_Dir customVectorDir = gp_Dir(0, 0, 1);
};

#endif // MODELING_HISTORY_RECORD_H
