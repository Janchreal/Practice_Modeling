#include "model_document_serializer.h"

#include "feature_serializer.h"
#include "application/history/model_document.h"

#include <BRepTools.hxx>

#include <QColor>
#include <QJsonObject>
#include <QString>

#include <sstream>

namespace {

QJsonObject colorToJson(const QColor& color)
{
    QJsonObject object;
    object["r"] = color.red();
    object["g"] = color.green();
    object["b"] = color.blue();
    object["a"] = color.alpha();
    return object;
}

} // namespace

GeometryPlacement::AxisPlacement modelPlacementFromJson(const QJsonObject& modelObject)
{
    const bool hasOrigin = modelObject.value("hasOrigin").toBool(false);
    const double originX = modelObject.value("originX").toDouble(0.0);
    const double originY = modelObject.value("originY").toDouble(0.0);
    const double originZ = modelObject.value("originZ").toDouble(0.0);
    const AxisDirection axisDirection =
        static_cast<AxisDirection>(modelObject.value("axisDirection").toInt(2));
    const bool axisReversed = modelObject.value("axisReversed").toBool(false);
    const bool hasCustomVectorDir = modelObject.value("hasCustomVectorDir").toBool(false);
    const double dirX = modelObject.value("dirX").toDouble(0.0);
    const double dirY = modelObject.value("dirY").toDouble(0.0);
    const double dirZ = modelObject.value("dirZ").toDouble(1.0);
    const double dirLengthSquared = dirX * dirX + dirY * dirY + dirZ * dirZ;
    const gp_Dir customVectorDir =
        (dirLengthSquared > 1e-12) ? gp_Dir(dirX, dirY, dirZ) : gp_Dir(0, 0, 1);

    return GeometryPlacement::makeAxisPlacement(hasOrigin,
                                                originX,
                                                originY,
                                                originZ,
                                                axisDirection,
                                                axisReversed,
                                                hasCustomVectorDir,
                                                customVectorDir);
}

QJsonArray modelDocumentToJson(const ModelDocument& document,
                               const ModelShapeResolver& shapeResolver,
                               const ModelVisibilityResolver& visibilityResolver)
{
    QJsonArray models;
    for (const ModelingHistory& record : document.histories()) {
        QJsonObject model = modelingHistoryToJson(record);
        model["color"] = colorToJson(record.color);
        model["visible"] = visibilityResolver ? visibilityResolver(record) : true;

        const TopoDS_Shape shape = shapeResolver ? shapeResolver(record) : TopoDS_Shape();
        if (!shape.IsNull()) {
            std::ostringstream stream;
            stream.setf(std::ios::fixed);
            BRepTools::Write(shape, stream);
            model["brep"] = QString::fromStdString(stream.str());
        }

        models.append(model);
    }
    return models;
}
