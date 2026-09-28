#ifndef BOOLEAN_OPS_H
#define BOOLEAN_OPS_H

#include "platformmath.h"//项目内数学/精度相关定义

#include <QList>
#include <QString>

#include <TopoDS_Shape.hxx>//OCC拓扑形状

namespace BooleanOps {
//基础单工具布尔运算，直接执行OCC Fuse/Common/Cut
bool executeOccBoolean(const TopoDS_Shape& targetShape,
                       const TopoDS_Shape& toolShape,
                       int operationType,
                       TopoDS_Shape& resultShape);
//增强版布尔运算，带模糊容差、异常捕获、修复、同域合并、差集有效性检查
bool executeOccBooleanChecked(const TopoDS_Shape& targetShape,
                              const TopoDS_Shape& toolShape,
                              int operationType,
                              TopoDS_Shape& resultShape,
                              double fuzzyValue = 1.0e-4);//executeOccBooleanChecked的默认模糊容差：
//多工具布尔运算，QList<TopoDS_Shape> 作为工具集合
bool executeOccBooleanMulti(const TopoDS_Shape& targetShape,
                            const QList<TopoDS_Shape>& toolShapes,
                            int operationType,
                            TopoDS_Shape& resultShape);
//布尔运算前预检查，判断形状是否满足重叠/相交条件
bool shapesSatisfyBooleanOverlap(const TopoDS_Shape& targetShape,
                                 const QList<TopoDS_Shape>& toolShapes,
                                 int operationType);
//将操作类型转为中文名
QString booleanOperationName(int operationType);

} // namespace BooleanOps

#endif // BOOLEAN_OPS_H
