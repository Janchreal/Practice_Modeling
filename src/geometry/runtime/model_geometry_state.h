#ifndef MODEL_GEOMETRY_STATE_H
#define MODEL_GEOMETRY_STATE_H

#include <TopoDS_Shape.hxx>

// Runtime OCC state. It is intentionally absent from ModelingHistory.
struct ModelGeometryState {
    TopoDS_Shape occShape;
};

#endif // MODEL_GEOMETRY_STATE_H
