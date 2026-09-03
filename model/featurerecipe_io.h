#ifndef FEATURERECIPE_IO_H
#define FEATURERECIPE_IO_H

#include "featurerecipe.h"
#include "modelinghistory.h"

#include <QJsonObject>

QJsonObject subShapeRefToJson(const SubShapeRef& ref);
SubShapeRef subShapeRefFromJson(const QJsonObject& obj);

QJsonObject featureRecipeToJson(const FeatureRecipe& recipe);
FeatureRecipe featureRecipeFromJson(const QJsonObject& obj);

QJsonObject modelingHistoryToJson(const ModelingHistory& record);
void applyModelingHistoryFromJson(const QJsonObject& obj, ModelingHistory& record);

#endif // FEATURERECIPE_IO_H
