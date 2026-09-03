#ifndef MODELHISTORYSNAPSHOT_H
#define MODELHISTORYSNAPSHOT_H

#include "platformmath.h"
#include "axisdirection.h"
#include "featurerecipe.h"
#include "modeltype.h"

#include <QColor>
#include <QList>
#include <QString>

#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

// 建模历史条目快照：用于撤销/重做时完整恢复形状、参数、配方与可见性
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

// 级联更新快照：记录父特征及其所有下游依赖在操作前后的完整状态
struct CascadeUndoRecord {
    QList<int> affectedIndices;
    QList<ModelHistorySnapshot> beforeStates;
    QList<ModelHistorySnapshot> afterStates;
    bool committed = false;
};

#endif // MODELHISTORYSNAPSHOT_H
