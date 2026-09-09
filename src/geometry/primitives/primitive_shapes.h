#ifndef GEOMETRY_PRIMITIVES_PRIMITIVE_SHAPES_H
#define GEOMETRY_PRIMITIVES_PRIMITIVE_SHAPES_H

#include "primitive.h"

namespace PrimitiveGeometry {

class CuboidPrimitive final : public Primitive {
public:
    ModelType modelType() const override;
    TopoDS_Shape build(const PrimitiveBuildRequest& request) const override;
};

class CylinderPrimitive final : public Primitive {
public:
    ModelType modelType() const override;
    TopoDS_Shape build(const PrimitiveBuildRequest& request) const override;
};

class ConePrimitive final : public Primitive {
public:
    ModelType modelType() const override;
    TopoDS_Shape build(const PrimitiveBuildRequest& request) const override;
};

class SpherePrimitive final : public Primitive {
public:
    ModelType modelType() const override;
    TopoDS_Shape build(const PrimitiveBuildRequest& request) const override;
};

} // namespace PrimitiveGeometry

#endif // GEOMETRY_PRIMITIVES_PRIMITIVE_SHAPES_H
