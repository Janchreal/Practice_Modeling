#include "feature_serializer.h"

#include <QJsonArray>

#include <TopAbs_ShapeEnum.hxx>

namespace {

QJsonArray intListToJson(const QList<int>& values)
{
    QJsonArray array;
    for (int value : values) {
        array.append(value);
    }
    return array;
}

QList<int> intListFromJson(const QJsonArray& array)
{
    QList<int> values;
    values.reserve(array.size());
    for (const QJsonValue& value : array) {
        values.append(value.toInt(-1));
    }
    return values;
}

QJsonObject gpPntToJson(const gp_Pnt& point)
{
    QJsonObject obj;
    obj["x"] = point.X();
    obj["y"] = point.Y();
    obj["z"] = point.Z();
    return obj;
}

gp_Pnt gpPntFromJson(const QJsonObject& obj)
{
    return gp_Pnt(obj.value("x").toDouble(), obj.value("y").toDouble(), obj.value("z").toDouble());
}

QJsonObject gpDirToJson(const gp_Dir& dir)
{
    QJsonObject obj;
    obj["x"] = dir.X();
    obj["y"] = dir.Y();
    obj["z"] = dir.Z();
    return obj;
}

gp_Dir gpDirFromJson(const QJsonObject& obj)
{
    const double x = obj.value("x").toDouble(0.0);
    const double y = obj.value("y").toDouble(0.0);
    const double z = obj.value("z").toDouble(1.0);
    const double len2 = x * x + y * y + z * z;
    return len2 > 1e-12 ? gp_Dir(x, y, z) : gp_Dir(0, 0, 1);
}

QJsonArray subShapeRefListToJson(const QList<SubShapeRef>& refs)
{
    QJsonArray array;
    for (const SubShapeRef& ref : refs) {
        array.append(subShapeRefToJson(ref));
    }
    return array;
}

QList<SubShapeRef> subShapeRefListFromJson(const QJsonArray& array)
{
    QList<SubShapeRef> refs;
    refs.reserve(array.size());
    for (const QJsonValue& value : array) {
        refs.append(subShapeRefFromJson(value.toObject()));
    }
    return refs;
}

} // namespace

QJsonObject subShapeRefToJson(const SubShapeRef& ref)
{
    QJsonObject obj;
    obj["parentIndex"] = ref.parentIndex;
    obj["shapeType"] = static_cast<int>(ref.shapeType);
    obj["persistentShapeIndex"] = ref.persistentShapeIndex;
    obj["subShapeId"] = static_cast<qint64>(ref.subShapeId);
    obj["signatureLength"] = ref.signatureLength;
    obj["signatureMidX"] = ref.signatureMidX;
    obj["signatureMidY"] = ref.signatureMidY;
    obj["signatureMidZ"] = ref.signatureMidZ;
    return obj;
}

SubShapeRef subShapeRefFromJson(const QJsonObject& obj)
{
    SubShapeRef ref;
    ref.parentIndex = obj.value("parentIndex").toInt(-1);
    ref.shapeType = static_cast<TopAbs_ShapeEnum>(obj.value("shapeType").toInt(TopAbs_FACE));
    ref.persistentShapeIndex = obj.value("persistentShapeIndex").toInt(-1);
    ref.subShapeId = static_cast<std::int64_t>(obj.value("subShapeId").toInteger(-1));
    ref.signatureLength = obj.value("signatureLength").toDouble(0.0);
    ref.signatureMidX = obj.value("signatureMidX").toDouble(0.0);
    ref.signatureMidY = obj.value("signatureMidY").toDouble(0.0);
    ref.signatureMidZ = obj.value("signatureMidZ").toDouble(0.0);
    return ref;
}

QJsonObject featureRecipeToJson(const FeatureRecipe& recipe)
{
    QJsonObject obj;
    obj["hasRecipe"] = recipe.hasRecipe;
    obj["parentIndices"] = intListToJson(recipe.parentIndices);

    QJsonObject booleanObj;
    booleanObj["targetIndex"] = recipe.boolean.targetIndex;
    booleanObj["toolIndices"] = intListToJson(recipe.boolean.toolIndices);
    booleanObj["operationType"] = recipe.boolean.operationType;
    booleanObj["keepTarget"] = recipe.boolean.keepTarget;
    booleanObj["keepTool"] = recipe.boolean.keepTool;
    obj["boolean"] = booleanObj;

    QJsonObject extrusionObj;
    extrusionObj["direction"] = gpDirToJson(recipe.extrusion.direction);
    extrusionObj["lengthFwd"] = recipe.extrusion.lengthFwd;
    extrusionObj["lengthRev"] = recipe.extrusion.lengthRev;
    extrusionObj["startOffset"] = recipe.extrusion.startOffset;
    extrusionObj["solid"] = recipe.extrusion.solid;
    extrusionObj["reversed"] = recipe.extrusion.reversed;
    extrusionObj["symmetric"] = recipe.extrusion.symmetric;
    extrusionObj["taperAngleFwd"] = recipe.extrusion.taperAngleFwd;
    extrusionObj["taperAngleRev"] = recipe.extrusion.taperAngleRev;
    extrusionObj["makeSheetBody"] = recipe.extrusion.makeSheetBody;
    extrusionObj["useVectorDirection"] = recipe.extrusion.useVectorDirection;
    extrusionObj["mergedInPlace"] = recipe.extrusion.mergedInPlace;
    extrusionObj["mergeTargetIndex"] = recipe.extrusion.mergeTargetIndex;
    extrusionObj["boolOpType"] = recipe.extrusion.boolOpType;
    extrusionObj["boolTargetIndex"] = recipe.extrusion.boolTargetIndex;
    extrusionObj["profiles"] = subShapeRefListToJson(recipe.extrusion.profiles);
    extrusionObj["profileModelIndices"] = intListToJson(recipe.extrusion.profileModelIndices);
    extrusionObj["baseType"] = static_cast<int>(recipe.extrusion.baseType);
    extrusionObj["baseParam1"] = recipe.extrusion.baseParam1;
    extrusionObj["baseParam2"] = recipe.extrusion.baseParam2;
    extrusionObj["baseParam3"] = recipe.extrusion.baseParam3;
    extrusionObj["baseHasOrigin"] = recipe.extrusion.baseHasOrigin;
    extrusionObj["baseOriginX"] = recipe.extrusion.baseOriginX;
    extrusionObj["baseOriginY"] = recipe.extrusion.baseOriginY;
    extrusionObj["baseOriginZ"] = recipe.extrusion.baseOriginZ;
    obj["extrusion"] = extrusionObj;

    QJsonObject revolveObj;
    revolveObj["axisOrigin"] = gpPntToJson(recipe.revolve.axisOrigin);
    revolveObj["axisDir"] = gpDirToJson(recipe.revolve.axisDir);
    revolveObj["angleDeg"] = recipe.revolve.angleDeg;
    revolveObj["startAngleDeg"] = recipe.revolve.startAngleDeg;
    revolveObj["endAngleDeg"] = recipe.revolve.endAngleDeg;
    revolveObj["boolOpType"] = recipe.revolve.boolOpType;
    revolveObj["boolTargetIndex"] = recipe.revolve.boolTargetIndex;
    revolveObj["profiles"] = subShapeRefListToJson(recipe.revolve.profiles);
    obj["revolve"] = revolveObj;

    QJsonObject filletObj;
    filletObj["targetIndex"] = recipe.fillet.targetIndex;
    filletObj["radius"] = recipe.fillet.radius;
    filletObj["isChamfer"] = recipe.fillet.isChamfer;
    filletObj["chamferDistance"] = recipe.fillet.chamferDistance;
    filletObj["chamferDistance2"] = recipe.fillet.chamferDistance2;
    filletObj["isChamferTwoDistances"] = recipe.fillet.isChamferTwoDistances;
    filletObj["isG2"] = recipe.fillet.isG2;
    filletObj["rho"] = recipe.fillet.rho;
    filletObj["usedRadius"] = recipe.fillet.usedRadius;
    filletObj["edges"] = subShapeRefListToJson(recipe.fillet.edges);
    obj["fillet"] = filletObj;

    QJsonObject patternObj;
    patternObj["layoutType"] = static_cast<int>(recipe.pattern.layoutType);
    patternObj["sourceIndices"] = intListToJson(recipe.pattern.sourceIndices);
    patternObj["origin"] = gpPntToJson(recipe.pattern.origin);
    patternObj["direction1"] = gpDirToJson(recipe.pattern.direction1);
    patternObj["direction2"] = gpDirToJson(recipe.pattern.direction2);
    patternObj["pitch1"] = recipe.pattern.pitch1;
    patternObj["pitch2"] = recipe.pattern.pitch2;
    patternObj["count1"] = recipe.pattern.count1;
    patternObj["count2"] = recipe.pattern.count2;
    patternObj["useRadialReplication"] = recipe.pattern.useRadialReplication;
    patternObj["polygonSpanDegrees"] = recipe.pattern.polygonSpanDegrees;
    patternObj["polygonSpacing"] = static_cast<int>(recipe.pattern.polygonSpacing);
    patternObj["polygonAlongEdgeCount"] = recipe.pattern.polygonAlongEdgeCount;
    patternObj["polygonAlongEdgePitch"] = recipe.pattern.polygonAlongEdgePitch;
    obj["pattern"] = patternObj;

    QJsonObject hollowObj;
    hollowObj["targetIndex"] = recipe.hollow.targetIndex;
    hollowObj["thickness"] = recipe.hollow.thickness;
    hollowObj["faces"] = subShapeRefListToJson(recipe.hollow.faces);
    obj["hollow"] = hollowObj;

    QJsonObject sketchObj;
    sketchObj["datumPlaneIndex"] = recipe.sketch.datumPlaneIndex;
    sketchObj["planeOrigin"] = gpPntToJson(recipe.sketch.planeOrigin);
    sketchObj["planeNormal"] = gpDirToJson(recipe.sketch.planeNormal);
    sketchObj["planeXDir"] = gpDirToJson(recipe.sketch.planeXDir);
    obj["sketch"] = sketchObj;

    return obj;
}

FeatureRecipe featureRecipeFromJson(const QJsonObject& obj)
{
    FeatureRecipe recipe;
    recipe.hasRecipe = obj.value("hasRecipe").toBool(false);
    recipe.parentIndices = intListFromJson(obj.value("parentIndices").toArray());

    const QJsonObject booleanObj = obj.value("boolean").toObject();
    recipe.boolean.targetIndex = booleanObj.value("targetIndex").toInt(-1);
    recipe.boolean.toolIndices = intListFromJson(booleanObj.value("toolIndices").toArray());
    recipe.boolean.operationType = booleanObj.value("operationType").toInt(-1);
    recipe.boolean.keepTarget = booleanObj.value("keepTarget").toBool(false);
    recipe.boolean.keepTool = booleanObj.value("keepTool").toBool(false);

    const QJsonObject extrusionObj = obj.value("extrusion").toObject();
    recipe.extrusion.direction = gpDirFromJson(extrusionObj.value("direction").toObject());
    recipe.extrusion.lengthFwd = extrusionObj.value("lengthFwd").toDouble(0.0);
    recipe.extrusion.lengthRev = extrusionObj.value("lengthRev").toDouble(0.0);
    recipe.extrusion.startOffset = extrusionObj.value("startOffset").toDouble(0.0);
    recipe.extrusion.solid = extrusionObj.value("solid").toBool(true);
    recipe.extrusion.reversed = extrusionObj.value("reversed").toBool(false);
    recipe.extrusion.symmetric = extrusionObj.value("symmetric").toBool(false);
    recipe.extrusion.taperAngleFwd = extrusionObj.value("taperAngleFwd").toDouble(0.0);
    recipe.extrusion.taperAngleRev = extrusionObj.value("taperAngleRev").toDouble(0.0);
    recipe.extrusion.makeSheetBody = extrusionObj.value("makeSheetBody").toBool(false);
    recipe.extrusion.useVectorDirection = extrusionObj.value("useVectorDirection").toBool(false);
    recipe.extrusion.mergedInPlace = extrusionObj.value("mergedInPlace").toBool(false);
    recipe.extrusion.mergeTargetIndex = extrusionObj.value("mergeTargetIndex").toInt(-1);
    recipe.extrusion.boolOpType = extrusionObj.value("boolOpType").toInt(-1);
    recipe.extrusion.boolTargetIndex = extrusionObj.value("boolTargetIndex").toInt(-1);
    recipe.extrusion.profiles = subShapeRefListFromJson(extrusionObj.value("profiles").toArray());
    recipe.extrusion.profileModelIndices = intListFromJson(extrusionObj.value("profileModelIndices").toArray());
    recipe.extrusion.baseType = static_cast<ModelType>(extrusionObj.value("baseType").toInt(CUBOID));
    recipe.extrusion.baseParam1 = extrusionObj.value("baseParam1").toDouble(0.0);
    recipe.extrusion.baseParam2 = extrusionObj.value("baseParam2").toDouble(0.0);
    recipe.extrusion.baseParam3 = extrusionObj.value("baseParam3").toDouble(0.0);
    recipe.extrusion.baseHasOrigin = extrusionObj.value("baseHasOrigin").toBool(false);
    recipe.extrusion.baseOriginX = extrusionObj.value("baseOriginX").toDouble(0.0);
    recipe.extrusion.baseOriginY = extrusionObj.value("baseOriginY").toDouble(0.0);
    recipe.extrusion.baseOriginZ = extrusionObj.value("baseOriginZ").toDouble(0.0);

    const QJsonObject revolveObj = obj.value("revolve").toObject();
    recipe.revolve.axisOrigin = gpPntFromJson(revolveObj.value("axisOrigin").toObject());
    recipe.revolve.axisDir = gpDirFromJson(revolveObj.value("axisDir").toObject());
    recipe.revolve.angleDeg = revolveObj.value("angleDeg").toDouble(0.0);
    recipe.revolve.startAngleDeg = revolveObj.value("startAngleDeg").toDouble(0.0);
    recipe.revolve.endAngleDeg = revolveObj.value("endAngleDeg").toDouble(recipe.revolve.angleDeg);
    recipe.revolve.boolOpType = revolveObj.value("boolOpType").toInt(-1);
    recipe.revolve.boolTargetIndex = revolveObj.value("boolTargetIndex").toInt(-1);
    recipe.revolve.profiles = subShapeRefListFromJson(revolveObj.value("profiles").toArray());

    const QJsonObject filletObj = obj.value("fillet").toObject();
    recipe.fillet.targetIndex = filletObj.value("targetIndex").toInt(-1);
    recipe.fillet.radius = filletObj.value("radius").toDouble(0.0);
    recipe.fillet.isChamfer = filletObj.value("isChamfer").toBool(false);
    recipe.fillet.chamferDistance = filletObj.value("chamferDistance").toDouble(0.0);
    recipe.fillet.chamferDistance2 = filletObj.value("chamferDistance2").toDouble(0.0);
    recipe.fillet.isChamferTwoDistances = filletObj.value("isChamferTwoDistances").toBool(false);
    recipe.fillet.isG2 = filletObj.value("isG2").toBool(false);
    recipe.fillet.rho = filletObj.value("rho").toDouble(0.5);
    recipe.fillet.usedRadius = filletObj.value("usedRadius").toDouble(0.0);
    recipe.fillet.edges = subShapeRefListFromJson(filletObj.value("edges").toArray());

    const QJsonObject patternObj = obj.value("pattern").toObject();
    recipe.pattern.layoutType = static_cast<PatternLayoutType>(patternObj.value("layoutType").toInt(0));
    recipe.pattern.sourceIndices = intListFromJson(patternObj.value("sourceIndices").toArray());
    recipe.pattern.origin = gpPntFromJson(patternObj.value("origin").toObject());
    recipe.pattern.direction1 = gpDirFromJson(patternObj.value("direction1").toObject());
    recipe.pattern.direction2 = gpDirFromJson(patternObj.value("direction2").toObject());
    recipe.pattern.pitch1 = patternObj.value("pitch1").toDouble(10.0);
    recipe.pattern.pitch2 = patternObj.value("pitch2").toDouble(10.0);
    recipe.pattern.count1 = patternObj.value("count1").toInt(2);
    recipe.pattern.count2 = patternObj.value("count2").toInt(2);
    recipe.pattern.useRadialReplication = patternObj.value("useRadialReplication").toBool(false);
    recipe.pattern.polygonSpanDegrees = patternObj.value("polygonSpanDegrees").toDouble(360.0);
    recipe.pattern.polygonSpacing = static_cast<PolygonSpacingMode>(patternObj.value("polygonSpacing").toInt(0));
    recipe.pattern.polygonAlongEdgeCount = patternObj.value("polygonAlongEdgeCount").toInt(2);
    recipe.pattern.polygonAlongEdgePitch = patternObj.value("polygonAlongEdgePitch").toDouble(10.0);

    const QJsonObject hollowObj = obj.value("hollow").toObject();
    recipe.hollow.targetIndex = hollowObj.value("targetIndex").toInt(-1);
    recipe.hollow.thickness = hollowObj.value("thickness").toDouble(0.0);
    recipe.hollow.faces = subShapeRefListFromJson(hollowObj.value("faces").toArray());

    const QJsonObject sketchObj = obj.value("sketch").toObject();
    recipe.sketch.datumPlaneIndex = sketchObj.value("datumPlaneIndex").toInt(-1);
    recipe.sketch.planeOrigin = gpPntFromJson(sketchObj.value("planeOrigin").toObject());
    recipe.sketch.planeNormal = gpDirFromJson(sketchObj.value("planeNormal").toObject());
    recipe.sketch.planeXDir = gpDirFromJson(sketchObj.value("planeXDir").toObject());

    return recipe;
}

QJsonObject modelingHistoryToJson(const ModelingHistory& record)
{
    QJsonObject obj;
    obj["type"] = static_cast<int>(record.type);
    obj["name"] = record.name;
    obj["param1"] = record.param1;
    obj["param2"] = record.param2;
    obj["param3"] = record.param3;
    obj["hasOrigin"] = record.hasOrigin;
    obj["originX"] = record.originX;
    obj["originY"] = record.originY;
    obj["originZ"] = record.originZ;
    obj["axisDirection"] = static_cast<int>(record.axisDirection);
    obj["axisReversed"] = record.axisReversed;
    obj["hasCustomVectorDir"] = record.hasCustomVectorDir;
    obj["dirX"] = record.customVectorDir.X();
    obj["dirY"] = record.customVectorDir.Y();
    obj["dirZ"] = record.customVectorDir.Z();
    obj["featureRegenerateFailed"] = record.featureRegenerateFailed;
    obj["booleanTargetIndex"] = record.booleanTargetIndex;
    obj["booleanToolIndices"] = intListToJson(record.booleanToolIndices);
    obj["booleanOperationType"] = record.booleanOperationType;
    obj["booleanKeepTarget"] = record.booleanKeepTarget;
    obj["booleanKeepTool"] = record.booleanKeepTool;
    obj["recipe"] = featureRecipeToJson(record.recipe);
    return obj;
}

void applyModelingHistoryFromJson(const QJsonObject& obj, ModelingHistory& record)
{
    record.featureRegenerateFailed = obj.value("featureRegenerateFailed").toBool(false);
    record.booleanTargetIndex = obj.value("booleanTargetIndex").toInt(-1);
    record.booleanToolIndices = intListFromJson(obj.value("booleanToolIndices").toArray());
    record.booleanOperationType = obj.value("booleanOperationType").toInt(-1);
    record.booleanKeepTarget = obj.value("booleanKeepTarget").toBool(false);
    record.booleanKeepTool = obj.value("booleanKeepTool").toBool(false);
    if (obj.contains("recipe")) {
        record.recipe = featureRecipeFromJson(obj.value("recipe").toObject());
    }
}
