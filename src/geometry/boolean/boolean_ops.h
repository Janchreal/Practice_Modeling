#ifndef BOOLEAN_OPS_H
#define BOOLEAN_OPS_H

#include "platformmath.h"

#include <QList>
#include <QString>

#include <TopoDS_Shape.hxx>

namespace BooleanOps {

bool executeOccBoolean(const TopoDS_Shape& targetShape,
                       const TopoDS_Shape& toolShape,
                       int operationType,
                       TopoDS_Shape& resultShape);

bool executeOccBooleanChecked(const TopoDS_Shape& targetShape,
                              const TopoDS_Shape& toolShape,
                              int operationType,
                              TopoDS_Shape& resultShape,
                              double fuzzyValue = 1.0e-4);

bool executeOccBooleanMulti(const TopoDS_Shape& targetShape,
                            const QList<TopoDS_Shape>& toolShapes,
                            int operationType,
                            TopoDS_Shape& resultShape);

bool shapesSatisfyBooleanOverlap(const TopoDS_Shape& targetShape,
                                 const QList<TopoDS_Shape>& toolShapes,
                                 int operationType);

QString booleanOperationName(int operationType);

} // namespace BooleanOps

#endif // BOOLEAN_OPS_H
