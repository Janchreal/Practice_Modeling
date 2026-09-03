#ifndef FEATURERECIPE_H
#define FEATURERECIPE_H

#include "platformmath.h"
#include "patternfeaturetypes.h"
#include "modeltype.h"

#include <IVtk_Types.hxx>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <TopAbs_ShapeEnum.hxx>

#include <QList>

// 子形状持久引用：父体更新后按索引或几何签名重新定位面/边/线框
struct SubShapeRef {
    int parentIndex = -1;
    TopAbs_ShapeEnum shapeType = TopAbs_FACE;
    int persistentShapeIndex = -1; // TopTools_IndexedMapOfShape，1-based
    IVtk_IdType subShapeId = -1;

    // 几何签名（索引失效时的 fallback）
    double signatureLength = 0.0;
    double signatureMidX = 0.0;
    double signatureMidY = 0.0;
    double signatureMidZ = 0.0;
};

struct BooleanRecipeData {
    int targetIndex = -1;
    QList<int> toolIndices;
    int operationType = -1;
    bool keepTarget = false;
    bool keepTool = false;
};

struct ExtrusionRecipeData {
    gp_Dir direction = gp_Dir(0, 0, 1);
    double lengthFwd = 0.0;
    double lengthRev = 0.0;
    /** 沿方向相对剖面的起始偏移（对应对话框起始距离） */
    double startOffset = 0.0;
    bool solid = true;
    bool reversed = false;
    bool symmetric = false;
    double taperAngleFwd = 0.0;
    double taperAngleRev = 0.0;
    bool makeSheetBody = false;
    bool useVectorDirection = false;
    bool mergedInPlace = false;
    int mergeTargetIndex = -1;
    /** -1=无布尔新建体；0=Fuse；1=Common；2=Cut */
    int boolOpType = -1;
    int boolTargetIndex = -1;

    QList<SubShapeRef> profiles;
    QList<int> profileModelIndices;

    // 原位合并拉伸时，用于重建合并前的基体
    ModelType baseType = CUBOID;
    double baseParam1 = 0.0;
    double baseParam2 = 0.0;
    double baseParam3 = 0.0;
    bool baseHasOrigin = false;
    double baseOriginX = 0.0;
    double baseOriginY = 0.0;
    double baseOriginZ = 0.0;
};

struct RevolveRecipeData {
    gp_Pnt axisOrigin;
    gp_Dir axisDir = gp_Dir(0, 0, 1);
    double angleDeg = 0.0;
    double startAngleDeg = 0.0;
    double endAngleDeg = 360.0;
    int boolOpType = -1;
    int boolTargetIndex = -1;
    QList<SubShapeRef> profiles;
};

struct FilletRecipeData {
    int targetIndex = -1;
    double radius = 0.0;
    bool isChamfer = false;
    double chamferDistance = 0.0;
    // 非对称倒角：两侧距离（Dis1 / Dis2）。若 isChamferTwoDistances=false，则 chamferDistance2 不使用
    double chamferDistance2 = 0.0;
    bool isChamferTwoDistances = false;
    bool isG2 = false;
    double rho = 0.5;
    double usedRadius = 0.0;
    QList<SubShapeRef> edges;
};

struct PatternRecipeData {
    PatternLayoutType layoutType = PatternLayoutType::Linear;
    QList<int> sourceIndices;
    gp_Pnt origin;
    gp_Dir direction1 = gp_Dir(1, 0, 0);
    gp_Dir direction2 = gp_Dir(0, 1, 0);
    double pitch1 = 10.0;
    double pitch2 = 10.0;
    int count1 = 2;
    int count2 = 2;
    bool useRadialReplication = false;
    double polygonSpanDegrees = 360.0;
    PolygonSpacingMode polygonSpacing = PolygonSpacingMode::CountPerSide;
    int polygonAlongEdgeCount = 2;
    double polygonAlongEdgePitch = 10.0;
};

struct HollowRecipeData {
    int targetIndex = -1;
    double thickness = 0.0;
    QList<SubShapeRef> faces;
};

struct SketchRecipeData {
    int datumPlaneIndex = -1;
    gp_Pnt planeOrigin;
    gp_Dir planeNormal = gp_Dir(0, 0, 1);
    gp_Dir planeXDir = gp_Dir(1, 0, 0);
};

struct FeatureRecipe {
    bool hasRecipe = false;
    QList<int> parentIndices;
    BooleanRecipeData boolean;
    ExtrusionRecipeData extrusion;
    RevolveRecipeData revolve;
    FilletRecipeData fillet;
    PatternRecipeData pattern;
    HollowRecipeData hollow;
    SketchRecipeData sketch;
};

#endif // FEATURERECIPE_H
