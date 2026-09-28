#ifndef EXTRUSION_GEOMETRY_H
#define EXTRUSION_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <list>
#include <string>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>

namespace ExtrusionGeometry {
//把一个已经“解析好”的截面沿 direction 拉伸
/*
profile：截面，可能是面、线、边等
direction：拉伸方向
length：拉伸长度
startOffset：起始偏移，先沿拉伸方向平移截面，再拉伸
makeSheetBody：
true：尽量生成片体/壳体
false：尽量生成实体
*/
TopoDS_Shape extrudeResolvedProfile(const TopoDS_Shape& profile,
                                    const gp_Dir& direction,
                                    double length,
                                    double startOffset,
                                    bool makeSheetBody);
//作用：对多个截面分别做直拉伸，然后简单放入一个 TopoDS_Compound
/*
只是组合，不是布尔融合。
拉伸长度 = endDistance - startDistance。
每个截面从 startDistance 位置开始拉伸。
只要至少一个截面拉伸成功，就返回 true。
*/
bool buildExtrusionCompound(const QList<TopoDS_Shape>& profiles,
                            const gp_Dir& direction,
                            double startDistance,
                            double endDistance,
                            bool makeSheetBody,
                            TopoDS_Compound& outCompound);
//两个重载，作用：把输入整理成适合实体拉伸的截面
/*
如果已经是 TopAbs_FACE，直接返回。
如果是 TopAbs_WIRE，尝试修复并生成面；失败则返回线。
如果是其他形状，提取所有边，组成 wire，再尝试生成面。
如果无法组成 wire，就返回原形状或空。
一句话：优先把边/线整理成面，面用于实体拉伸；实在不行就退化为 wire。
*/
TopoDS_Shape prepareSolidExtrusionProfile(const TopoDS_Shape& sourceShape);
TopoDS_Shape prepareSolidExtrusionProfile(const QList<TopoDS_Edge>& edges);
/*
支持正向拉伸 lengthFwd
支持反向拉伸 lengthRev
支持正向拔模角 taperAngleFwd
支持反向拔模角 taperAngleRev
solid = true 生成实体
solid = false 生成片体/壳体
输出到 drafts
失败时写 errorMessage
它本质上是一个“双向 + 拔模 + 放样 + 内孔处理”的拉伸生成器
*/
bool buildTaperedExtrusionDrafts(const TopoDS_Shape& sourceShape,
                                 const gp_Dir& direction,
                                 double lengthFwd,
                                 double lengthRev,
                                 bool solid,
                                 double taperAngleFwd,
                                 double taperAngleRev,
                                 std::list<TopoDS_Shape>& drafts,
                                 std::string* errorMessage = nullptr);

} // namespace ExtrusionGeometry

#endif // EXTRUSION_GEOMETRY_H
