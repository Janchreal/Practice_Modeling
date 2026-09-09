#ifndef MODEL_DOCUMENT_H
#define MODEL_DOCUMENT_H

#include "modeling_history_record.h"

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

private:
    void assignIdentity(ModelingHistory& record);

    HistoryList histories_;
    quint64 nextId_ = 1;
};

#endif // MODEL_DOCUMENT_H
