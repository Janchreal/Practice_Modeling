#ifndef MODELDOCUMENT_H
#define MODELDOCUMENT_H

#include "modelinghistory.h"

#include <QList>

// 持有建模条目列表；后续可在此集中序列化/依赖图等逻辑
class ModelDocument {
public:
    QList<ModelingHistory> histories;
};

#endif
