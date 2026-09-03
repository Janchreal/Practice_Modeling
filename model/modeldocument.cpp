#include "modeldocument.h"

#include "featurerecipe_io.h"

#include <BRepTools.hxx>

#include <QColor>
#include <QJsonObject>
#include <QString>

#include <sstream>

namespace {

QJsonObject colorToJson(const QColor& c)
{
    QJsonObject o;
    o["r"] = c.red();
    o["g"] = c.green();
    o["b"] = c.blue();
    o["a"] = c.alpha();
    return o;
}

} // namespace

ModelDocument::HistoryList& ModelDocument::histories()
{
    return histories_;
}

const ModelDocument::HistoryList& ModelDocument::histories() const
{
    return histories_;
}

int ModelDocument::size() const
{
    return histories_.size();
}

bool ModelDocument::isEmpty() const
{
    return histories_.isEmpty();
}

void ModelDocument::clear()
{
    histories_.clear();
}

int ModelDocument::append(const ModelingHistory& record)
{
    histories_.append(record);
    return histories_.size() - 1;
}

void ModelDocument::insert(int index, const ModelingHistory& record)
{
    if (index < 0 || index > histories_.size()) {
        histories_.append(record);
        return;
    }
    histories_.insert(index, record);
}

void ModelDocument::removeAt(int index)
{
    if (index < 0 || index >= histories_.size()) {
        return;
    }
    histories_.removeAt(index);
}

ModelingHistory& ModelDocument::at(int index)
{
    return histories_[index];
}

const ModelingHistory& ModelDocument::at(int index) const
{
    return histories_[index];
}

QJsonArray ModelDocument::toJsonArray() const
{
    QJsonArray models;
    for (const ModelingHistory& rec : histories_) {
        QJsonObject m = modelingHistoryToJson(rec);
        m["color"] = colorToJson(rec.color);
        if (!rec.occShape.IsNull()) {
            std::ostringstream oss;
            oss.setf(std::ios::fixed);
            BRepTools::Write(rec.occShape, oss);
            m["brep"] = QString::fromStdString(oss.str());
        }
        models.append(m);
    }
    return models;
}
