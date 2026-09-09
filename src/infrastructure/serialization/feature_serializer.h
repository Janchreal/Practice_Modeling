#ifndef FEATURE_SERIALIZER_H
#define FEATURE_SERIALIZER_H

#include "domain/features/featurerecipe.h"
#include "application/history/modeling_history_record.h"

#include <QJsonObject>

QJsonObject subShapeRefToJson(const SubShapeRef& ref);
SubShapeRef subShapeRefFromJson(const QJsonObject& obj);

QJsonObject featureRecipeToJson(const FeatureRecipe& recipe);
FeatureRecipe featureRecipeFromJson(const QJsonObject& obj);

QJsonObject modelingHistoryToJson(const ModelingHistory& record);
void applyModelingHistoryFromJson(const QJsonObject& obj, ModelingHistory& record);

#endif // FEATURE_SERIALIZER_H
