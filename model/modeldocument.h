#ifndef MODELDOCUMENT_H
#define MODELDOCUMENT_H

#include "modelinghistory.h"

#include <QJsonArray>
#include <QList>

class ModelDocument {
public:
    using HistoryList = QList<ModelingHistory>;

    HistoryList& histories();
    const HistoryList& histories() const;

    int size() const;
    bool isEmpty() const;
    void clear();

    int append(const ModelingHistory& record);
    void insert(int index, const ModelingHistory& record);
    void removeAt(int index);

    ModelingHistory& at(int index);
    const ModelingHistory& at(int index) const;

    QJsonArray toJsonArray() const;

private:
    HistoryList histories_;
};

#endif // MODELDOCUMENT_H
