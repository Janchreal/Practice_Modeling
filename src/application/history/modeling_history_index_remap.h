#ifndef MODELING_HISTORY_INDEX_REMAP_H
#define MODELING_HISTORY_INDEX_REMAP_H

#include "modeling_history_record.h"

#include <QList>

namespace ModelingHistoryIndexRemap {

void afterRemoval(QList<ModelingHistory>& records, int removedIndex);
void afterInsertion(QList<ModelingHistory>& records, int insertedIndex);

} // namespace ModelingHistoryIndexRemap

#endif // MODELING_HISTORY_INDEX_REMAP_H
