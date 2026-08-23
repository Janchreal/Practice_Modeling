#include "extrusioncommand.h"
#include "widget.h"
#include "modeltype.h"
#include "feature_topology.h"
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepTools.hxx>
#include <cmath>
#include <list>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

TopoDS_Shape extrudeResolvedProfile(const TopoDS_Shape& profile,
                                    const ExtrusionRecipeData& recipe,
                                    bool makeSheetBody)
{
    if (profile.IsNull()) {
        return TopoDS_Shape();
    }

    gp_Dir direction = recipe.direction;
    if (recipe.reversed) {
        direction.Reverse();
    }

    gp_Vec extrudeVec(direction);
    extrudeVec.Scale(recipe.lengthFwd);

    TopoDS_Shape shapeToExtrude = profile;
    if (profile.ShapeType() == TopAbs_FACE && makeSheetBody) {
        const TopoDS_Wire outer = BRepTools::OuterWire(TopoDS::Face(profile));
        if (!outer.IsNull()) {
            shapeToExtrude = outer;
        }
    } else if (profile.ShapeType() == TopAbs_WIRE && !makeSheetBody) {
        ShapeFix_Wire fixer;
        fixer.Load(TopoDS::Wire(profile));
        fixer.FixReorder();
        fixer.FixConnected();
        fixer.FixClosed();
        BRepBuilderAPI_MakeFace faceMaker(fixer.Wire());
        if (faceMaker.IsDone()) {
            shapeToExtrude = faceMaker.Face();
        }
    }

    if (std::abs(recipe.startOffset) > Precision::Confusion()) {
        gp_Trsf startShift;
        startShift.SetTranslation(gp_Vec(direction) * recipe.startOffset);
        BRepBuilderAPI_Transform mover(shapeToExtrude, startShift, true);
        shapeToExtrude = mover.Shape();
    }

    BRepPrimAPI_MakePrism prism(shapeToExtrude, extrudeVec);
    if (!prism.IsDone()) {
        return TopoDS_Shape();
    }
    return prism.Shape();
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

bool ExtrusionCommand::extrudeFromRecipe(Widget* widget, const ExtrusionRecipeData& recipe, TopoDS_Shape& resultShape)
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
                const TopoDS_Shape extruded = extrudeResolvedProfile(profile, recipe, recipe.makeSheetBody);
                if (extruded.IsNull()) {
                    return false;
                }
                BRepAlgoAPI_Fuse fuseOp(baseShape, extruded);
                if (!fuseOp.IsDone()) {
                    return false;
                }
                resultShape = fuseOp.Shape();
                ShapeFix_Shape shapeFix(resultShape);
                shapeFix.Perform();
                if (!shapeFix.Shape().IsNull()) {
                    resultShape = shapeFix.Shape();
                }
                ShapeUpgrade_UnifySameDomain unify(resultShape, true, true, true);
                unify.Build();
                if (!unify.Shape().IsNull()) {
                    resultShape = unify.Shape();
                }
            } else {
                resultShape = baseShape;
            }
        } else if (!recipe.profiles.isEmpty()) {
            const TopoDS_Shape profile = resolveSubShapeRef(widget, recipe.profiles.first());
            const TopoDS_Shape extruded = extrudeResolvedProfile(profile, recipe, recipe.makeSheetBody);
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
                    BRepAlgoAPI_Fuse fuseOp(parentShape, extruded);
                    if (!fuseOp.IsDone()) {
                        return false;
                    }
                    resultShape = fuseOp.Shape();
                }
            } else {
                // 布尔=无：独立新建体，不自动与父体 Fuse
                resultShape = extruded;
            }

            if (!resultShape.IsNull()) {
                ShapeFix_Shape shapeFix(resultShape);
                shapeFix.Perform();
                if (!shapeFix.Shape().IsNull()) {
                    resultShape = shapeFix.Shape();
                }
                ShapeUpgrade_UnifySameDomain unify(resultShape, true, true, true);
                unify.Build();
                if (!unify.Shape().IsNull()) {
                    resultShape = unify.Shape();
                }
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

ExtrusionCommand::ExtrusionCommand(Widget* widget,
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
    gp_Vec vec = gp_Vec(params.dir) * (params.lengthFwd + params.lengthRev);
    
    // 复制源形状（避免修改原始形状）
    BRepBuilderAPI_Copy copyMaker(source);
    if (!copyMaker.IsDone()) {
        throw std::runtime_error("Cannot copy shape");
    }
    TopoDS_Shape myShape = copyMaker.Shape();
    
    // 检查是否有锥度角
    if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
        std::fabs(params.taperAngleRev) >= Precision::Angular()) {
        // 带锥度的拉伸
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
        if (source.IsNull()) {
            throw std::runtime_error("Cannot extrude empty shape");
        }
        
        // 应用反向拉伸的偏移
        if (fabs(params.lengthRev) > Precision::Confusion()) {
            gp_Trsf mov;
            mov.SetTranslation(gp_Vec(params.dir) * (-params.lengthRev));
            myShape.Move(mov);
        }
        
        // 如果是实体模式且源形状是线框，需要先创建面
        if (params.solid) {
            TopExp_Explorer faceExp(myShape, TopAbs_FACE);
            if (!faceExp.More()) {
                // 没有面，尝试从线框创建面
                TopExp_Explorer wireExp(myShape, TopAbs_WIRE);
                if (wireExp.More()) {
                    try {
                        BRepBuilderAPI_MakeFace faceMaker(TopoDS::Wire(wireExp.Current()));
                        if (faceMaker.IsDone()) {
                            myShape = faceMaker.Face();
                        }
                    } catch (...) {
                        // 创建面失败，继续使用原始形状
                    }
                }
            }
        }
        
        // 执行拉伸
        BRepPrimAPI_MakePrism prismMaker(myShape, vec);
        if (!prismMaker.IsDone()) {
            throw std::runtime_error("Extrusion failed");
        }
        
        result = prismMaker.Shape();
    }
}

void ExtrusionCommand::makeDraft(const TopoDS_Shape& shape,
                                 const ExtrusionParameters& params,
                                 std::list<TopoDS_Shape>& drafts)
{
    // 完全按照ExtrusionHelper::makeDraft的实现
    std::vector<std::vector<TopoDS_Shape>> wiresections;
    
    auto addWiresToWireSections =
        [&shape](std::vector<std::vector<TopoDS_Shape>>& wiresections) -> size_t {
        TopExp_Explorer ex;
        size_t i = 0;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
            wiresections.emplace_back();
            wiresections[i].push_back(TopoDS::Wire(ex.Current()));
        }
        return i;
    };
    
    double distanceFwd = tan(params.taperAngleFwd) * params.lengthFwd;
    double distanceRev = tan(params.taperAngleRev) * params.lengthRev;
    gp_Vec vecFwd = gp_Vec(params.dir) * params.lengthFwd;
    gp_Vec vecRev = gp_Vec(params.dir.Reversed()) * params.lengthRev;
    
    bool bFwd = fabs(params.lengthFwd) > Precision::Confusion();
    bool bRev = fabs(params.lengthRev) > Precision::Confusion();
    bool bMid = !bFwd || !bRev || -1.0 * params.taperAngleFwd != params.taperAngleRev;
    
    if (shape.IsNull())
        throw std::runtime_error("Not a valid shape");
    
    size_t numWires = addWiresToWireSections(wiresections);
    if (numWires == 0)
        throw std::runtime_error("Extrusion: Input must not only consist if a vertex");
    
    TopoDS_Wire offsetWire;
    std::vector<std::vector<TopoDS_Shape>> extrusionSections(wiresections.size(), std::vector<TopoDS_Shape>());
    size_t rows = 0;
    int numEdges = 0;
    
    // 检查内外线框
    std::vector<TopoDS_Shape> resultPrisms;
    TopoDS_Shape singlePrism;
    for (auto& wireVector : wiresections) {
        for (auto& singleWire : wireVector) {
            BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(singleWire));
            auto tempFace = mkFace.Shape();
            BRepPrimAPI_MakePrism mkPrism(tempFace, vecFwd);
            if (!mkPrism.IsDone())
                throw std::runtime_error("Extrusion: Generating prism failed");
            singlePrism = mkPrism.Shape();
            resultPrisms.push_back(singlePrism);
        }
    }
    std::vector<bool> isInnerWire(resultPrisms.size(), false);
    std::vector<bool> checklist(resultPrisms.size(), true);
    checkInnerWires(isInnerWire, params.dir, checklist, false, resultPrisms);
    
    int numInnerWires = 0;
    for (auto isInner : isInnerWire) {
        if (isInner)
            ++numInnerWires;
    }
    
    // 创建反向截面
    if (bRev) {
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                numEdges = 0;
                TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                while (xp.More()) {
                    numEdges++;
                    xp.Next();
                }
                if (!isInnerWire[rows]) {
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                }
                else {
                    if (numEdges > 1) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, -distanceRev, true, offsetWire);
                    }
                    else {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                    }
                }
                if (offsetWire.IsNull())
                    return;
                extrusionSections[rows].push_back(offsetWire);
            }
            ++rows;
        }
    }
    
    // 添加中间截面
    if (bMid) {
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                extrusionSections[rows].push_back(singleWire);
            }
            rows++;
        }
    }
    
    // 创建正向截面
    if (bFwd) {
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                numEdges = 0;
                TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                while (xp.More()) {
                    numEdges++;
                    xp.Next();
                }
                if (!isInnerWire[rows]) {
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
                }
                else {
                    if (numEdges > 1) {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, -distanceFwd, false, offsetWire);
                    }
                    else {
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
                    }
                }
                if (offsetWire.IsNull())
                    return;
                extrusionSections[rows].push_back(offsetWire);
            }
            ++rows;
        }
    }
    
    try {
        std::vector<TopoDS_Shape> shells;
        
        for (auto& wires : extrusionSections) {
            BRepOffsetAPI_ThruSections mkTS(params.solid, Standard_True, Precision::Confusion());
            
            for (auto& singleWire : wires) {
                if (singleWire.ShapeType() == TopAbs_VERTEX)
                    mkTS.AddVertex(TopoDS::Vertex(singleWire));
                else
                    mkTS.AddWire(TopoDS::Wire(singleWire));
            }
            mkTS.Build();
            if (!mkTS.IsDone())
                throw std::runtime_error("Extrusion: Loft could not be built");
            
            shells.push_back(mkTS.Shape());
        }
        
        if (params.solid) {
            if (numInnerWires > 0) {
                GProp_GProps tempProperties;
                Standard_Real momentOfInertiaInitial;
                Standard_Real momentOfInertiaFinal;
                std::vector<bool>::iterator isInnerWireIterator = isInnerWire.begin();
                std::vector<bool>::iterator isInnerWireIteratorLoop;
                for (auto itOuter = shells.begin(); itOuter != shells.end(); ++itOuter) {
                    if (*isInnerWireIterator) {
                        ++isInnerWireIterator;
                        continue;
                    }
                    isInnerWireIteratorLoop = isInnerWire.begin();
                    for (auto itInner = shells.begin(); itInner != shells.end(); ++itInner) {
                        if (itOuter == itInner || !*isInnerWireIteratorLoop) {
                            ++isInnerWireIteratorLoop;
                            continue;
                        }
                        BRepGProp::VolumeProperties(*itOuter, tempProperties);
                        momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), params.dir));
                        BRepAlgoAPI_Cut mkCut(*itOuter, *itInner);
                        if (!mkCut.IsDone())
                            throw std::runtime_error("Extrusion: Final cut out failed");
                        BRepGProp::VolumeProperties(mkCut.Shape(), tempProperties);
                        momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), params.dir));
                        if ((momentOfInertiaInitial != momentOfInertiaFinal)
                            && (momentOfInertiaFinal > Precision::Confusion())) {
                            *itOuter = mkCut.Shape();
                        }
                        ++isInnerWireIteratorLoop;
                    }
                    drafts.push_back(*itOuter);
                    ++isInnerWireIterator;
                }
            }
            else {
                for (const auto & shell : shells)
                    drafts.push_back(shell);
            }
        }
        else {
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            for (TopoDS_Shape& s : shells)
                sewer.Add(s);
            sewer.Perform();
            drafts.push_back(sewer.SewedShape());
        }
    }
    catch (Standard_Failure& e) {
        throw std::runtime_error(e.GetMessageString());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(e.what());
    }
    catch (...) {
        throw std::runtime_error("Extrusion: A fatal error occurred when making the loft");
    }
}

void ExtrusionCommand::checkInnerWires(std::vector<bool>& isInnerWire,
                                       const gp_Dir& direction,
                                       std::vector<bool>& checklist,
                                       bool forInner,
                                       const std::vector<TopoDS_Shape>& prisms)
{
    // 完全按照ExtrusionHelper::checkInnerWires的实现
    size_t numCheckWiresInitial = 0;
    for (auto checks : checklist) {
        if (checks)
            ++numCheckWiresInitial;
    }
    GProp_GProps tempProperties;
    Standard_Real momentOfInertiaInitial;
    Standard_Real momentOfInertiaFinal;
    size_t numCheckWires = 0;
    std::vector<bool>::iterator isInnerWireIterator = isInnerWire.begin();
    std::vector<bool>::iterator toCheckIterator = checklist.begin();
    std::vector<bool> toDisable(checklist.size(), false);
    int outer = -1;
    
    for (auto itOuter = prisms.begin(); itOuter != prisms.end(); ++itOuter) {
        ++outer;
        if (!*toCheckIterator) {
            ++isInnerWireIterator;
            ++toCheckIterator;
            continue;
        }
        auto toCheckIteratorInner = checklist.begin();
        bool saveIsInnerWireIterator = *isInnerWireIterator;
        for (auto itInner = prisms.begin(); itInner != prisms.end(); ++itInner) {
            if (itOuter == itInner || !*toCheckIteratorInner) {
                ++toCheckIteratorInner;
                continue;
            }
            BRepGProp::VolumeProperties(*itInner, tempProperties);
            momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            BRepAlgoAPI_Cut mkCut(*itInner, *itOuter);
            if (!mkCut.IsDone())
                throw std::runtime_error("Extrusion: Cut out failed");
            BRepGProp::VolumeProperties(mkCut.Shape(), tempProperties);
            momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            if ((momentOfInertiaInitial != momentOfInertiaFinal)
                && (momentOfInertiaFinal > Precision::Confusion())) {
                *isInnerWireIterator = !forInner;
                ++numCheckWires;
                *toCheckIterator = true;
                break;
            }
            ++toCheckIteratorInner;
        }
        if (saveIsInnerWireIterator == *isInnerWireIterator)
            toDisable[outer] = true;
        ++isInnerWireIterator;
        ++toCheckIterator;
    }
    
    size_t i = 0;
    for (auto disable : toDisable) {
        if (disable)
            checklist[i] = false;
        ++i;
    }
    
    if (numCheckWires == isInnerWire.size()) {
        isInnerWire[0] = false;
        checklist[0] = false;
        --numCheckWires;
    }
    
    if (numCheckWiresInitial == numCheckWires) {
        i = 0;
        for (auto checks : checklist) {
            if (checks) {
                isInnerWire[i] = false;
                checklist[i] = false;
                --numCheckWires;
            }
            ++i;
        }
    }
    
    if (numCheckWires > 1)
        checkInnerWires(isInnerWire, direction, checklist, !forInner, prisms);
}

void ExtrusionCommand::createTaperedPrismOffset(const TopoDS_Wire& sourceWire,
                                                const gp_Vec& translation,
                                                double offset,
                                                bool isSecond,
                                                TopoDS_Wire& result)
{
    // 完全按照ExtrusionHelper::createTaperedPrismOffset的实现
    // 注意：使用BRepOffsetAPI_MakeOffset替代BRepOffsetAPI_MakeOffsetFix
    gp_Trsf tempTransform;
    tempTransform.SetTranslation(translation);
    TopLoc_Location loc(tempTransform);
    TopoDS_Wire movedSourceWire = TopoDS::Wire(sourceWire.Moved(loc));
    
    TopoDS_Shape offsetShape;
    if (fabs(offset) > Precision::Confusion()) {
        BRepOffsetAPI_MakeOffset offsetMaker;
        offsetMaker.AddWire(movedSourceWire);
        try {
            offsetMaker.Perform(offset);
            if (offsetMaker.IsDone()) {
                offsetShape = offsetMaker.Shape();
            } else {
                throw std::runtime_error("Extrusion: Offset could not be created");
            }
        }
        catch (const Standard_Failure& e) {
            throw std::runtime_error(e.GetMessageString());
        }
    }
    else {
        offsetShape = movedSourceWire;
    }
    
    if (offsetShape.IsNull()) {
        throw std::runtime_error("Extrusion: end face of tapered extrusion is empty");
    }
    
    TopAbs_ShapeEnum type = offsetShape.ShapeType();
    if (type == TopAbs_WIRE) {
        result = TopoDS::Wire(offsetShape);
    }
    else if (type == TopAbs_EDGE) {
        BRepBuilderAPI_MakeWire mkWire2(TopoDS::Edge(offsetShape));
        result = mkWire2.Wire();
    }
    else {
        result = TopoDS_Wire();
        throw std::runtime_error("Extrusion: type of extrusion end face is not supported");
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
