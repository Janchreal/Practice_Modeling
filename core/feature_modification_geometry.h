#ifndef FEATURE_MODIFICATION_GEOMETRY_H
#define FEATURE_MODIFICATION_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <string>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopTools_ListOfShape.hxx>

namespace FeatureModificationGeometry {

bool buildFilletShape(const TopoDS_Shape& targetShape,
                      const QList<TopoDS_Edge>& edges,
                      double radius,
                      bool isG2,
                      double rho,
                      TopoDS_Shape& outShape,
                      double* usedRadius = nullptr,
                      std::string* errorMessage = nullptr);

bool buildChamferShape(const TopoDS_Shape& targetShape,
                       const QList<TopoDS_Edge>& edges,
                       double distance1,
                       double distance2,
                       bool twoDistances,
                       TopoDS_Shape& outShape,
                       std::string* errorMessage = nullptr);

bool buildHollowShape(const TopoDS_Shape& targetShape,
                      const TopTools_ListOfShape& facesToRemove,
                      double thickness,
                      TopoDS_Shape& outShape,
                      std::string* errorMessage = nullptr);

} // namespace FeatureModificationGeometry

#endif // FEATURE_MODIFICATION_GEOMETRY_H
