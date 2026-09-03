#include "extrusioncommand.h"
#include "modeltype.h"
#include "feature_topology.h"
#include "extrusion_geometry.h"
#include "boolean_ops.h"
#include <BRep_Builder.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <cmath>
#include <list>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FeatureRecipe ExtrusionCommand::buildProfileExtrusionRecipe(int profileIndex, const ExtrusionParameters& params)
{
    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    recipe.parentIndices = { profileIndex };
    recipe.extrusion.profileModelIndices = { profileIndex };
    recipe.extrusion.direction = params.dir;
    recipe.extrusion.lengthFwd = params.lengthFwd;
    recipe.extrusion.lengthRev = params.lengthRev;
    recipe.extrusion.solid = params.solid;
    recipe.extrusion.taperAngleFwd = params.taperAngleFwd;
    recipe.extrusion.taperAngleRev = params.taperAngleRev;
    return recipe;
}

bool ExtrusionCommand::extrudeFromRecipe(CommandContext* widget, const ExtrusionRecipeData& recipe, TopoDS_Shape& resultShape)
{
    resultShape = TopoDS_Shape();
    if (!widget) {
        return false;
    }

    try {
        if (recipe.mergedInPlace) {
            TopoDS_Shape baseShape = widget->rebuildBaseShapeFromExtrusionRecipe(recipe);
            if (baseShape.IsNull()) {
                return false;
            }

            if (!recipe.profiles.isEmpty()) {
                const TopoDS_Shape profile = resolveSubShapeRef(widget, recipe.profiles.first());
                gp_Dir direction = recipe.direction;
                if (recipe.reversed) {
                    direction.Reverse();
                }
                const TopoDS_Shape extruded = ExtrusionGeometry::extrudeResolvedProfile(
                    profile, direction, recipe.lengthFwd, recipe.startOffset, recipe.makeSheetBody);
                if (extruded.IsNull()) {
                    return false;
                }
                if (!BooleanOps::executeOccBooleanChecked(baseShape, extruded, 0, resultShape)) {
                    return false;
                }
            } else {
                resultShape = baseShape;
            }
        } else if (!recipe.profiles.isEmpty()) {
            const TopoDS_Shape profile = resolveSubShapeRef(widget, recipe.profiles.first());
            gp_Dir direction = recipe.direction;
            if (recipe.reversed) {
                direction.Reverse();
            }
            const TopoDS_Shape extruded = ExtrusionGeometry::extrudeResolvedProfile(
                profile, direction, recipe.lengthFwd, recipe.startOffset, recipe.makeSheetBody);
            if (extruded.IsNull()) {
                return false;
            }

            if (recipe.boolOpType >= 0) {
                TopoDS_Shape booleaned;
                if (!widget->applyDialogBooleanToShape(recipe.boolOpType, recipe.boolTargetIndex,
                                                      extruded, booleaned)
                    || booleaned.IsNull()) {
                    return false;
                }
                resultShape = booleaned;
            } else if (recipe.mergedInPlace) {
                // 兼容旧配方：原位与并到基体
                int parentIndex = recipe.mergeTargetIndex;
                if (parentIndex < 0 && !recipe.profiles.isEmpty()) {
                    parentIndex = recipe.profiles.first().parentIndex;
                }
                if (parentIndex < 0) {
                    resultShape = extruded;
                } else {
                    const TopoDS_Shape parentShape = widget->getShapeFromHistory(parentIndex);
                    if (parentShape.IsNull()) {
                        return false;
                    }
                    if (!BooleanOps::executeOccBooleanChecked(parentShape, extruded, 0, resultShape)) {
                        return false;
                    }
                }
            } else {
                // 布尔=无：独立新建体，不自动与父体 Fuse
                resultShape = extruded;
            }

        } else if (!recipe.profileModelIndices.isEmpty()) {
            ExtrusionParameters params;
            params.dir = recipe.direction;
            params.lengthFwd = recipe.lengthFwd;
            params.lengthRev = recipe.lengthRev;
            params.solid = recipe.solid;
            params.taperAngleFwd = recipe.taperAngleFwd;
            params.taperAngleRev = recipe.taperAngleRev;
            const TopoDS_Shape source = widget->getShapeFromHistory(recipe.profileModelIndices.first());
            extrudeShape(resultShape, source, params);
        } else {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    } catch (...) {
        return false;
    }

    return !resultShape.IsNull();
}

ExtrusionCommand::ExtrusionCommand(CommandContext* widget,
                                   const QList<int>& profileIndices,
                                   const gp_Dir& direction,
                                   double lengthFwd,
                                   double lengthRev,
                                   const QString& resultName,
                                   const QColor& resultColor,
                                   bool solid,
                                   bool reversed,
                                   bool symmetric,
                                   double taperAngle,
                                   double taperAngleRev)
    : profileIndices(profileIndices),
      direction(direction),
      lengthFwd(lengthFwd),
      lengthRev(lengthRev),
      resultName(resultName),
      resultColor(resultColor),
      solid(solid),
      reversed(reversed),
      symmetric(symmetric),
      taperAngle(taperAngle),
      taperAngleRev(taperAngleRev),
      executed(false)
{
    this->widget = widget;
}

ExtrusionCommand::~ExtrusionCommand()
{
}

void ExtrusionCommand::execute()
{
    if (executed) {
        return;
    }

    if (!widget) {
        return;
    }

    performExtrusion();
    executed = true;
}

void ExtrusionCommand::undo()
{
    if (!executed || !widget) {
        return;
    }

    for (int index : resultIndices) {
        widget->removeModel(index);
    }
    resultIndices.clear();
    executed = false;
}

QString ExtrusionCommand::getDescription() const
{
    return QString("拉伸: %1").arg(resultName);
}

ExtrusionParameters ExtrusionCommand::computeFinalParameters()
{
    // 完全按照FeatureExtrusion::computeFinalParameters的实现
    ExtrusionParameters result;
    
    // 处理方向
    gp_Dir dir = direction;
    if (reversed) {
        dir.Reverse();
    }
    
    // 检查方向长度
    gp_Vec dirVec(dir);
    if (dirVec.Magnitude() < Precision::Confusion()) {
        throw std::runtime_error("Direction is zero-length");
    }
    result.dir = dir;
    
    // 处理长度
    result.lengthFwd = lengthFwd;
    result.lengthRev = lengthRev;
    
    // 如果两个长度都为0，使用方向的模长
    if (fabs(result.lengthFwd) < Precision::Confusion() &&
        fabs(result.lengthRev) < Precision::Confusion()) {
        result.lengthFwd = dirVec.Magnitude();
    }
    
    // 处理对称拉伸
    if (symmetric) {
        result.lengthRev = result.lengthFwd * 0.5;
        result.lengthFwd = result.lengthFwd * 0.5;
    }
    
    // 检查总长度
    if (fabs(result.lengthFwd + result.lengthRev) < Precision::Confusion()) {
        throw std::runtime_error("Total length of extrusion is zero.");
    }
    
    result.solid = solid;
    
    // 转换锥度角为弧度
    result.taperAngleFwd = taperAngle * M_PI / 180.0;
    if (fabs(result.taperAngleFwd) > M_PI * 0.5 - Precision::Angular()) {
        throw std::runtime_error("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");
    }
    
    result.taperAngleRev = taperAngleRev * M_PI / 180.0;
    if (fabs(result.taperAngleRev) > M_PI * 0.5 - Precision::Angular()) {
        throw std::runtime_error("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");
    }
    
    // 默认使用简单的面创建器
    result.faceMakerClass = "Simple";
    
    return result;
}

void ExtrusionCommand::extrudeShape(TopoDS_Shape& result, const TopoDS_Shape& source, const ExtrusionParameters& params)
{
    // 完全按照FeatureExtrusion::extrudeShape的实现
    // 检查是否有锥度角
    if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
        std::fabs(params.taperAngleRev) >= Precision::Angular()) {
        // 带锥度的拉伸
        TopoDS_Shape myShape = source;
        if (myShape.IsNull()) {
            throw std::runtime_error("Cannot extrude empty shape");
        }
        std::list<TopoDS_Shape> drafts;
        makeDraft(myShape, params, drafts);
        if (drafts.empty()) {
            throw std::runtime_error("Drafting shape failed");
        }
        else {
            // 组合结果
            if (drafts.size() == 1) {
                result = drafts.front();
            } else {
                TopoDS_Compound compound;
                BRep_Builder builder;
                builder.MakeCompound(compound);
                for (const auto& draft : drafts) {
                    builder.Add(compound, draft);
                }
                result = compound;
            }
        }
    }
    else {
        // 常规拉伸（无锥度）
        result = ExtrusionGeometry::extrudeResolvedProfile(
            source,
            params.dir,
            params.lengthFwd + params.lengthRev,
            -params.lengthRev,
            params.solid);
        if (result.IsNull()) {
            throw std::runtime_error("Extrusion failed");
        }
    }
}

void ExtrusionCommand::makeDraft(const TopoDS_Shape& shape,
                                 const ExtrusionParameters& params,
                                 std::list<TopoDS_Shape>& drafts)
{
    std::string error;
    if (!ExtrusionGeometry::buildTaperedExtrusionDrafts(
            shape, params.dir, params.lengthFwd, params.lengthRev, params.solid,
            params.taperAngleFwd, params.taperAngleRev, drafts, &error)) {
        throw std::runtime_error(error.empty() ? "Extrusion: A fatal error occurred when making the loft"
                                               : error);
    }
}

void ExtrusionCommand::performExtrusion()
{
    if (!widget) {
        return;
    }
    
    try {
        ExtrusionParameters params = computeFinalParameters();
        
        for (int profileIndex : profileIndices) {
            TopoDS_Shape profileShape = widget->getShapeFromHistory(profileIndex);
            if (profileShape.IsNull()) {
                continue;
            }
            
            TopoDS_Shape resultShape;
            extrudeShape(resultShape, profileShape, params);
            
            if (resultShape.IsNull()) {
                continue;
            }
            
            QString originalName = widget->getHistoryList()[profileIndex].name;
            double totalLength = params.lengthFwd + params.lengthRev;
            QString finalResultName = resultName.isEmpty() ? 
                QString("拉伸(正向=%1,反向=%2)_%3")
                    .arg(params.lengthFwd)
                    .arg(params.lengthRev)
                    .arg(originalName) : 
                resultName;
            
            widget->displayOccShape(resultShape, finalResultName, EXTRUSION, resultColor, totalLength);
            
            int resultIndex = widget->getHistorySize() - 1;
            widget->assignFeatureRecipe(resultIndex, buildProfileExtrusionRecipe(profileIndex, params));
            widget->setModelVisibleForCommand(profileIndex, false);
            resultIndices.append(resultIndex);
        }
    }
    catch (const std::exception&) {
        // 上层负责提示/处理
    }
    catch (...) {
        // 上层负责提示/处理
    }
}


