#ifndef GEOMETRY_PRIMITIVES_PRIMITIVE_H
#define GEOMETRY_PRIMITIVES_PRIMITIVE_H

#include "primitive_build_request.h"

#include <TopoDS_Shape.hxx>

#include <memory>

namespace PrimitiveGeometry {

/**
 * Common contract for parameterized solid primitives.
 *
 * The primitive owns only geometry construction rules. Placement and
 * parameters are supplied by PrimitiveBuildRequest, so this layer remains
 * independent from Qt, the document, and the presentation layer.
 */
class Primitive {
public:
    virtual ~Primitive() = default;

    virtual ModelType modelType() const = 0;
    virtual TopoDS_Shape build(const PrimitiveBuildRequest& request) const = 0;
};

using PrimitivePtr = std::unique_ptr<Primitive>;

} // namespace PrimitiveGeometry

#endif // GEOMETRY_PRIMITIVES_PRIMITIVE_H
