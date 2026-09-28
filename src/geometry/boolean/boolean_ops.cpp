#include "boolean_ops.h"

#include <algorithm>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>

namespace BooleanOps {

namespace {
//如果传入形状是COMPOUND或COMPSOLID，尝试把它内部的多个SOLID或FACE融合成一个整体形状
/*
空形状直接返回空
如果不是 compound/compsolid，直接返回原形状
先尝试融合所有 TopAbs_SOLID
如果失败，再尝试融合所有TopAbs_FACE
都失败则返回原operand
让工具形状尽量变成单一实体，减少compound参与布尔运算时的不稳定性
*/
TopoDS_Shape coalesceOperandShape(const TopoDS_Shape& operand)
{
    if (operand.IsNull()) {
        return TopoDS_Shape();
    }
    if (operand.ShapeType() != TopAbs_COMPOUND && operand.ShapeType() != TopAbs_COMPSOLID) {
        return operand;
    }

    auto fuseSubshapes = [&operand](TopAbs_ShapeEnum kind) -> TopoDS_Shape {
        TopoDS_Shape fused;
        bool has = false;
        for (TopExp_Explorer ex(operand, kind); ex.More(); ex.Next()) {
            const TopoDS_Shape current = ex.Current();
            if (!has) {
                fused = current;
                has = true;
                continue;
            }
            BRepAlgoAPI_Fuse fuse(fused, current);
            fuse.Build();
            if (!fuse.IsDone() || fuse.Shape().IsNull()) {
                return TopoDS_Shape();
            }
            fused = fuse.Shape();
        }
        return has ? fused : TopoDS_Shape();
    };

    TopoDS_Shape fusedSolids = fuseSubshapes(TopAbs_SOLID);
    if (!fusedSolids.IsNull()) {
        return fusedSolids;
    }

    TopoDS_Shape fusedFaces = fuseSubshapes(TopAbs_FACE);
    if (!fusedFaces.IsNull()) {
        return fusedFaces;
    }

    return operand;
}
//作用：用包围盒快速判断两个形状是否可能重叠
/*
1.分别计算 Bnd_Box
2.对 boxA 放大 Precision::Confusion() * 10.0
3.用 !boxA.IsOut(boxB) 判断是否相交
这是快速预判，不精确，但能排除明显不相交的情况
*/
bool boundingBoxesOverlap(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    if (a.IsNull() || b.IsNull()) {
        return false;
    }

    Bnd_Box boxA;
    Bnd_Box boxB;
    BRepBndLib::Add(a, boxA);
    BRepBndLib::Add(b, boxB);
    boxA.Enlarge(Precision::Confusion() * 10.0);
    return !boxA.IsOut(boxB);
}
//作用：判断两个形状的交集是否有正体积
/*
1.执行 BRepAlgoAPI_Common
2.检查是否完成、交集是否为空
3.用 BRepGProp::VolumeProperties 计算交集体积
4.体积大于 Precision::Confusion() 才返回 true
适用于实体布尔运算，对于面、壳、线，体积通常为 0
*/
bool commonSolidVolumePositive(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    if (a.IsNull() || b.IsNull()) {
        return false;
    }

    BRepAlgoAPI_Common common(a, b);
    if (!common.IsDone()) {
        return false;
    }

    const TopoDS_Shape inter = common.Shape();
    if (inter.IsNull()) {
        return false;
    }

    GProp_GProps props;
    BRepGProp::VolumeProperties(inter, props);
    return props.Mass() > Precision::Confusion();
}
//作用：判断布尔结果是否“有用”只要结果中包含至少一个 SOLID 或 FACE，就认为有效；否则返回 false
bool hasUsefulBooleanShape(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return false;
    }
    for (TopExp_Explorer ex(shape, TopAbs_SOLID); ex.More(); ex.Next()) {
        return true;
    }
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        return true;
    }
    return false;
}
//作用：安全融合两个形状
/*
first 为空，返回 second
second 为空，返回 first
否则执行 BRepAlgoAPI_Fuse
成功返回融合结果，失败返回空形状
*/
TopoDS_Shape fuseShapes(const TopoDS_Shape& first, const TopoDS_Shape& second)
{
    if (first.IsNull()) {
        return second;
    }
    if (second.IsNull()) {
        return first;
    }

    BRepAlgoAPI_Fuse fuse(first, second);
    if (!fuse.IsDone()) {
        return TopoDS_Shape();
    }
    return fuse.Shape();
}

} // namespace
//基础版布尔运算
/*
根据 operationType：
0：BRepAlgoAPI_Fuse
1：BRepAlgoAPI_Common
2：BRepAlgoAPI_Cut
执行后检查 IsDone()，成功则把结果写入 resultShape
特点：
1.简单直接
2.没有异常捕获
3.没有设置模糊容差
4.没有结果修复
5.没有检查结果是否包含有效实体/面
6.适合简单、可控场景。
*/
bool executeOccBoolean(const TopoDS_Shape& targetShape,
                       const TopoDS_Shape& toolShape,
                       int operationType,
                       TopoDS_Shape& resultShape)
{
    switch (operationType) {
    case 0: {
        BRepAlgoAPI_Fuse op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    case 1: {
        BRepAlgoAPI_Common op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    case 2: {
        BRepAlgoAPI_Cut op(targetShape, toolShape);
        if (!op.IsDone()) return false;
        resultShape = op.Shape();
        return true;
    }
    default:
        return false;
    }
}
//增强版布尔运算，是实际使用中更推荐的函数
/*
1.清空 resultShape。
2.检查 targetShape 和 toolShape 是否为空。
3.复制目标形状：BRepBuilderAPI_Copy targetCopy(targetShape);
4.对工具形状先 coalesceOperandShape，再复制：BRepBuilderAPI_Copy toolCopy(coalesceOperandShape(toolShape));
5.根据操作类型执行布尔：
并集：BRepAlgoAPI_Fuse
交集：BRepAlgoAPI_Common
差集：BRepAlgoAPI_Cut
6.设置：op.SetFuzzyValue(fuzzyValue);op.SetRunParallel(Standard_True);op.Build();模糊容差用于处理微小间隙/重合；并行构建提高性能。
7.差集额外检查：
计算目标体积 vT 和结果体积 vR。
如果目标体积大于混淆值，但体积减少量非常小，则认为差集没有真正切除材料，返回失败。
8.用 hasUsefulBooleanShape 检查结果是否包含实体或面
9.修复结果：ShapeFix_Shape fixer(result);fixer.Perform();
10.同域合并：ShapeUpgrade_UnifySameDomain unify(result, true, true, true);unify.Build();用于合并相同边/面，减少碎片。
11.输出结果，捕获 Standard_Failure 和未知异常，失败返回 false。
*/
bool executeOccBooleanChecked(const TopoDS_Shape& targetShape,
                              const TopoDS_Shape& toolShape,
                              int operationType,
                              TopoDS_Shape& resultShape,
                              double fuzzyValue)
{
    resultShape = TopoDS_Shape();
    if (targetShape.IsNull() || toolShape.IsNull()) {
        return false;
    }

    BRepBuilderAPI_Copy targetCopy(targetShape);
    BRepBuilderAPI_Copy toolCopy(coalesceOperandShape(toolShape));
    if (!targetCopy.IsDone() || !toolCopy.IsDone()) {
        return false;
    }

    const TopoDS_Shape target = targetCopy.Shape();
    const TopoDS_Shape tool = toolCopy.Shape();
    if (target.IsNull() || tool.IsNull()) {
        return false;
    }

    try {
        TopoDS_Shape result;
        switch (operationType) {
        case 0: {
            BRepAlgoAPI_Fuse op(target, tool);
            op.SetFuzzyValue(fuzzyValue);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) {
                return false;
            }
            result = op.Shape();
            break;
        }
        case 1: {
            BRepAlgoAPI_Common op(target, tool);
            op.SetFuzzyValue(fuzzyValue);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) {
                return false;
            }
            result = op.Shape();
            break;
        }
        case 2: {
            BRepAlgoAPI_Cut op(target, tool);
            op.SetFuzzyValue(fuzzyValue);
            op.SetRunParallel(Standard_True);
            op.Build();
            if (!op.IsDone() || op.Shape().IsNull()) {
                return false;
            }
            result = op.Shape();
            {
                GProp_GProps propsTarget, propsResult;
                BRepGProp::VolumeProperties(target, propsTarget);
                BRepGProp::VolumeProperties(result, propsResult);
                const double vT = propsTarget.Mass();
                const double vR = propsResult.Mass();
                if (vT > Precision::Confusion()
                    && (vT - vR) < std::max(1.0e-6 * vT, Precision::Confusion() * 10.0)) {
                    return false;
                }
            }
            break;
        }
        default:
            return false;
        }

        if (!hasUsefulBooleanShape(result)) {
            return false;
        }

        ShapeFix_Shape fixer(result);
        fixer.Perform();
        if (!fixer.Shape().IsNull()) {
            result = fixer.Shape();
        }
        ShapeUpgrade_UnifySameDomain unify(result, true, true, true);
        unify.Build();
        if (!unify.Shape().IsNull()) {
            result = unify.Shape();
        }

        resultShape = result;
        return !resultShape.IsNull();
    } catch (Standard_Failure&) {
        return false;
    } catch (...) {
        return false;
    }
}
//多工具体布尔运算
/*
0并，逐个把工具与当前结果融合
1交，逐个求交
2差，先把所有工具融合成一个整体，再从目标中减去
特点：
1.支持多工具。
2.不像 executeOccBooleanChecked 那样有异常捕获、模糊容差、修复和同域合并。
3.差集分支对空工具的处理不如并集/交集严格。
*/
bool executeOccBooleanMulti(const TopoDS_Shape& targetShape,
                            const QList<TopoDS_Shape>& toolShapes,
                            int operationType,
                            TopoDS_Shape& resultShape)
{
    if (targetShape.IsNull() || toolShapes.isEmpty()) {
        return false;
    }

    switch (operationType) {
    case 0: {
        TopoDS_Shape fused = targetShape;
        for (const TopoDS_Shape& toolShape : toolShapes) {
            if (toolShape.IsNull()) {
                return false;
            }
            fused = fuseShapes(fused, toolShape);
            if (fused.IsNull()) {
                return false;
            }
        }
        resultShape = fused;
        return true;
    }
    case 1: {
        TopoDS_Shape common = targetShape;
        for (const TopoDS_Shape& toolShape : toolShapes) {
            if (toolShape.IsNull()) {
                return false;
            }
            BRepAlgoAPI_Common op(common, toolShape);
            if (!op.IsDone()) {
                return false;
            }
            common = op.Shape();
            if (common.IsNull()) {
                return false;
            }
        }
        resultShape = common;
        return true;
    }
    case 2: {
        TopoDS_Shape combinedTools = toolShapes.first();
        for (int i = 1; i < toolShapes.size(); ++i) {
            combinedTools = fuseShapes(combinedTools, toolShapes.at(i));
            if (combinedTools.IsNull()) {
                return false;
            }
        }
        BRepAlgoAPI_Cut op(targetShape, combinedTools);
        if (!op.IsDone()) {
            return false;
        }
        resultShape = op.Shape();
        return true;
    }
    default:
        return false;
    }
}
//布尔运算前的预检查
/*
1.遍历每个工具形状。
2.并集 0：要求目标与工具的包围盒重叠。
3.交集 1、差集 2：要求目标与工具的交集体积为正。
4.任一工具不满足，返回 false。
5.全部满足，返回 true。
用途：在执行布尔前快速判断是否值得做，避免无效计算。
注意：
并集其实不要求形状重叠，但这里要求包围盒重叠，语义偏严格。
交集体积为正只适合实体，不适合面/壳运算。
*/
bool shapesSatisfyBooleanOverlap(const TopoDS_Shape& targetShape,
                                 const QList<TopoDS_Shape>& toolShapes,
                                 int operationType)
{
    if (targetShape.IsNull() || toolShapes.isEmpty()) {
        return false;
    }

    for (const TopoDS_Shape& toolShape : toolShapes) {
        if (toolShape.IsNull()) {
            return false;
        }

        switch (operationType) {
        case 0:
            if (!boundingBoxesOverlap(targetShape, toolShape)) {
                return false;
            }
            break;
        case 1:
        case 2:
            if (!commonSolidVolumePositive(targetShape, toolShape)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }

    return true;
}

QString booleanOperationName(int operationType)
{
    switch (operationType) {
    case 0: return QStringLiteral("并集");
    case 1: return QStringLiteral("交集");
    case 2: return QStringLiteral("差集");
    default: return QStringLiteral("未知");
    }
}

} // namespace BooleanOps
