#ifndef FEATURE_TOPOLOGY_H
#define FEATURE_TOPOLOGY_H

#include "commandcontext.h"
#include "featurerecipe.h"

#include <TopoDS_Shape.hxx>

SubShapeRef makeSubShapeRef(int parentIndex,
                            const TopoDS_Shape& parentShape,
                            const TopoDS_Shape& subShape,
                            TopAbs_ShapeEnum shapeType,
                            IVtk_IdType subShapeId = -1);

TopoDS_Shape resolveSubShapeRef(CommandContext* context, const SubShapeRef& ref);

#endif // FEATURE_TOPOLOGY_H
