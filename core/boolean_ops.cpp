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
