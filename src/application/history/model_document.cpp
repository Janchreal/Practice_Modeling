#include "model_document.h"

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
    nextId_ = 1;
}

int ModelDocument::append(const ModelingHistory& record)
{
    ModelingHistory entry = record;
    assignIdentity(entry);
    histories_.append(entry);
    return histories_.size() - 1;
}

void ModelDocument::insert(int index, const ModelingHistory& record)
{
    ModelingHistory entry = record;
    assignIdentity(entry);
    if (index < 0 || index > histories_.size()) {
        histories_.append(entry);
        return;
    }
    histories_.insert(index, entry);
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

void ModelDocument::assignIdentity(ModelingHistory& record)
{
    if (record.id == 0) {
        record.id = nextId_++;
        return;
    }

    if (record.id >= nextId_) {
        nextId_ = record.id + 1;
    }
}
