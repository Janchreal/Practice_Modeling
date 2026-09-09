#ifndef INTERSECTION_RENDER_STORE_H
#define INTERSECTION_RENDER_STORE_H

#include <QHash>
#include <QPair>
#include <QList>
#include <QtGlobal>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

struct IntersectionRenderState {
    vtkSmartPointer<vtkPolyData> polyData;
    vtkSmartPointer<vtkActor> actor;
};

class IntersectionRenderStore {
public:
    using Key = QPair<quint64, quint64>;

    static Key canonicalKey(quint64 firstId, quint64 secondId);

    IntersectionRenderState& ensure(quint64 firstId, quint64 secondId);
    IntersectionRenderState* find(quint64 firstId, quint64 secondId);
    const IntersectionRenderState* find(quint64 firstId, quint64 secondId) const;

    void remove(quint64 firstId, quint64 secondId);
    QList<Key> keysForRecord(quint64 recordId) const;
    void clear();

private:
    QHash<Key, IntersectionRenderState> states_;
};

#endif // INTERSECTION_RENDER_STORE_H
