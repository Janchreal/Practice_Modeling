#ifndef MODEL_DOCUMENT_SERIALIZER_H
#define MODEL_DOCUMENT_SERIALIZER_H

#include "geometry/placement/axis_placement.h"
#include "application/history/modeling_history_record.h"

#include <functional>
#include <QJsonArray>
#include <QJsonObject>
#include <TopoDS_Shape.hxx>

class ModelDocument;

// Persistence belongs to infrastructure, not to the domain document.
using ModelShapeResolver = std::function<TopoDS_Shape(const ModelingHistory&)>;
using ModelVisibilityResolver = std::function<bool(const ModelingHistory&)>;

QJsonArray modelDocumentToJson(const ModelDocument& document,
                              const ModelShapeResolver& shapeResolver,
                              const ModelVisibilityResolver& visibilityResolver);

GeometryPlacement::AxisPlacement modelPlacementFromJson(const QJsonObject& modelObject);

#endif // MODEL_DOCUMENT_SERIALIZER_H
