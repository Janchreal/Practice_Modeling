#include "extrusioncommand.h"
#include "common/modeltype.h"
#include "application/ports/modeling_command_port.h"
#include "geometry/topology/feature_topology.h"
#include "geometry/extrusion/extrusion_geometry.h"
#include "geometry/sketch/sketch_geometry.h"
#include "geometry/boolean/boolean_ops.h"
#include <BRep_Builder.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <cmath>
#include <list>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

TopoDS_Shape resolveExtrusionProfile(ModelingCommandPort* context,
                                     int profileIndex,
                                     bool solid,
                                     std::string* errorMessage = nullptr)
{
    if (!context) {
        if (errorMessage) *errorMessage = "Extrusion context is invalid";
        return TopoDS_Shape();
    }

    const QList<ModelingHistory>& histories = context->getHistoryList();
    if (profileIndex < 0 || profileIndex >= histories.size()) {
        if (errorMessage) *errorMessage = "Extrusion profile does not exist";
        return TopoDS_Shape();
    }

    const ModelingHistory& record = histories[profileIndex];
    const TopoDS_Shape source = context->getShapeFromHistory(profileIndex);
    if (source.IsNull()) {
        if (errorMessage) *errorMessage = "Extrusion profile has no shape";
        return TopoDS_Shape();
    }

    if (record.type == SKETCH) {
        if (!solid) {
            return source;
        }

        gp_Pln plane(record.recipe.sketch.planeOrigin,
                     record.recipe.sketch.planeNormal);
        TopoDS_Shape profile;
        if (!SketchGeometry::buildPlanarProfile(
                source, plane, profile, errorMessage)) {
            return TopoDS_Shape();
        }
        return profile;
    }

    if (!solid) {
        return source;
    }
    return ExtrusionGeometry::prepareSolidExtrusionProfile(source);
}

TopoDS_Shape resolveProfileReference(ModelingCommandPort* context,
                                      const SubShapeRef& profileRef,
                                      const ExtrusionRecipeData& recipe)
{
    if (!context) {
        return TopoDS_Shape();
    }

    const QList<ModelingHistory>& histories = context->getHistoryList();
    if (profileRef.parentIndex >= 0 && profileRef.parentIndex < histories.size()
        && histories[profileRef.parentIndex].type == SKETCH) {
        if (profileRef.semanticKind == static_cast<int>(SubShapeSemanticKind::SketchContour)) {
            const TopoDS_Shape sketchShape = context->getShapeFromHistory(profileRef.parentIndex);
            if (sketchShape.IsNull()) {
                return TopoDS_Shape();
            }

            const ModelingHistory& record = histories[profileRef.parentIndex];
            const gp_Pln plane(record.recipe.sketch.planeOrigin,
                               record.recipe.sketch.planeNormal);
            TopoDS_Shape profile;
            std::string error;
            if (!SketchGeometry::buildPlanarProfileAt(
                    sketchShape, plane, profileRef.sketchContourIndex,
                    profile, &error)) {
                return TopoDS_Shape();
            }

            if (profileRef.signatureEdgeCount > 0) {
                int edgeCount = 0;
                for (TopExp_Explorer ex(profile, TopAbs_EDGE); ex.More(); ex.Next()) {
                    ++edgeCount;
                }
                if (edgeCount != profileRef.signatureEdgeCount) {
                    return TopoDS_Shape();
                }
            }

            return profile;
        }

        std::string error;
        return resolveExtrusionProfile(
            context, profileRef.parentIndex, !recipe.makeSheetBody, &error);
    }

    return resolveSubShapeRef(
        context->getShapeFromHistory(profileRef.parentIndex), profileRef);
}

} // namespace

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

bool ExtrusionCommand::extrudeFromRecipe(ModelingCommandPort* context, const ExtrusionRecipeData& recipe, TopoDS_Shape& resultShape)
{
    resultShape = TopoDS_Shape();
    if (!context) {
        return false;
    }

    try {
        if (recipe.mergedInPlace) {
            TopoDS_Shape baseShape = context->rebuildBaseShapeFromExtrusionRecipe(recipe);
            if (baseShape.IsNull()) {
                return false;
            }

            if (!recipe.profiles.isEmpty()) {
                const SubShapeRef& profileRef = recipe.profiles.first();
                const TopoDS_Shape profile = resolveProfileReference(context, profileRef, recipe);
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
            const SubShapeRef& profileRef = recipe.profiles.first();
            const TopoDS_Shape profile = resolveProfileReference(context, profileRef, recipe);
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
                if (!context->applyDialogBooleanToShape(recipe.boolOpType, recipe.boolTargetIndex,
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
                    const TopoDS_Shape parentShape = context->getShapeFromHistory(parentIndex);
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
            const bool makeSheetBody = recipe.makeSheetBody;
            const bool solid = recipe.solid && !makeSheetBody;
            std::string error;
            const TopoDS_Shape source = resolveExtrusionProfile(
                context,
                recipe.profileModelIndices.first(),
                solid,
                &error);
            if (source.IsNull()) {
                return false;
            }

            gp_Dir direction = recipe.direction;
            if (recipe.reversed) {
                direction.Reverse();
            }

            // Older profileModelIndices recipes store forward/reverse lengths,
            // while dialog-created recipes store one length plus a start offset.
            double length = recipe.lengthFwd;
            double startOffset = recipe.startOffset;
            if (std::abs(startOffset) <= Precision::Confusion()
                && std::abs(recipe.lengthRev) > Precision::Confusion()) {
                length = recipe.lengthFwd + recipe.lengthRev;
                startOffset = -recipe.lengthRev;
            }
            if (recipe.symmetric && std::abs(recipe.lengthRev) <= Precision::Confusion()) {
                length = recipe.lengthFwd;
                startOffset = -length * 0.5;
            }
            if (std::abs(length) <= Precision::Confusion()) {
                return false;
            }

            if (std::abs(recipe.taperAngleFwd) >= Precision::Angular()
                || std::abs(recipe.taperAngleRev) >= Precision::Angular()) {
                ExtrusionParameters params;
                params.dir = direction;
                params.lengthFwd = recipe.lengthFwd;
                params.lengthRev = recipe.lengthRev;
                params.solid = solid;
                params.taperAngleFwd = recipe.taperAngleFwd;
                params.taperAngleRev = recipe.taperAngleRev;
                extrudeShape(resultShape, source, params);
            } else {
                resultShape = ExtrusionGeometry::extrudeResolvedProfile(
                    source, direction, length, startOffset, makeSheetBody);
            }
            if (resultShape.IsNull()) {
                return false;
            }

            if (recipe.boolOpType >= 0) {
                TopoDS_Shape booleaned;
                if (!context->applyDialogBooleanToShape(
                        recipe.boolOpType, recipe.boolTargetIndex,
                        resultShape, booleaned)
                    || booleaned.IsNull()) {
                    return false;
                }
                resultShape = booleaned;
            }
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

ExtrusionCommand::ExtrusionCommand(ModelingCommandPort* context,
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
    : Command(context),
      profileIndices(profileIndices),
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
}

void ExtrusionCommand::execute()
{
    if (executed) {
        return;
    }

    if (!context_) {
        return;
    }

    performExtrusion();
    executed = true;
}

void ExtrusionCommand::undo()
{
    if (!executed || !context_) {
        return;
    }

    for (int index : resultIndices) {
        context_->removeModel(index);
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
    if (!context_) {
        return;
    }
    
    try {
        ExtrusionParameters params = computeFinalParameters();
        
        for (int profileIndex : profileIndices) {
            std::string error;
            TopoDS_Shape profileShape = resolveExtrusionProfile(
                context_, profileIndex, params.solid, &error);
            if (profileShape.IsNull()) {
                continue;
            }
            
            TopoDS_Shape resultShape;
            extrudeShape(resultShape, profileShape, params);
            
            if (resultShape.IsNull()) {
                continue;
            }
            
            QString originalName = context_->getHistoryList()[profileIndex].name;
            double totalLength = params.lengthFwd + params.lengthRev;
            QString finalResultName = resultName.isEmpty() ? 
                QString("拉伸(正向=%1,反向=%2)_%3")
                    .arg(params.lengthFwd)
                    .arg(params.lengthRev)
                    .arg(originalName) : 
                resultName;
            
            context_->displayOccShape(resultShape, finalResultName, EXTRUSION, resultColor, totalLength);
            
            int resultIndex = context_->getHistorySize() - 1;
            context_->assignFeatureRecipe(resultIndex, buildProfileExtrusionRecipe(profileIndex, params));
            context_->setModelVisibleForCommand(profileIndex, false);
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


