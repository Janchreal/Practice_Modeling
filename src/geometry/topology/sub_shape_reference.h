#ifndef SUB_SHAPE_REFERENCE_H
#define SUB_SHAPE_REFERENCE_H

#include <TopAbs_ShapeEnum.hxx>

#include <cstdint>

// Persistent reference to a sub-shape of a parent feature result.
struct SubShapeRef {
    int parentIndex = -1;
    TopAbs_ShapeEnum shapeType = TopAbs_FACE;
    int persistentShapeIndex = -1; // TopTools_IndexedMapOfShape, 1-based
    // Rendering adapters may assign a sub-shape ID, but the persisted
    // reference must not depend on VTK's typedefs.
    std::int64_t subShapeId = -1;

    double signatureLength = 0.0;
    double signatureMidX = 0.0;
    double signatureMidY = 0.0;
    double signatureMidZ = 0.0;
};

#endif // SUB_SHAPE_REFERENCE_H
