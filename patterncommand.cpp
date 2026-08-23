#include "patterncommand.h"
#include "widget.h"
#include "addmodelcommand.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopoDS_Compound.hxx>

#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

PatternCommand::PatternCommand(Widget* widget,
                               PatternLayoutType layoutType,
                               const QList<int>& sourceIndices,
                               const gp_Pnt& patternOrigin,
                               const gp_Dir& direction1,
                               double pitch1,
                               int count1,
                               bool useRadialReplication,
                               const gp_Dir& radialDir,
                               double pitch2,
                               int count2,
                               double polygonSpanDegrees,
                               PolygonSpacingMode polygonSpacing,
                               int polygonAlongEdgeCount,
                               double polygonAlongEdgePitch,
                               const QString& name,
                               const QColor& color)
    : layoutType_(layoutType)
    , patternOrigin_(patternOrigin)
    , sourceIndices_(sourceIndices)
    , direction1_(direction1)
    , direction2_(radialDir)
    , pitch1_(pitch1)
    , pitch2_(pitch2)
    , count1_(count1)
    , count2_(count2)
    , polygonSpanDegrees_(polygonSpanDegrees)
    , polygonSpacing_(polygonSpacing)
    , polygonAlongEdgeCount_(polygonAlongEdgeCount)
    , polygonAlongEdgePitch_(polygonAlongEdgePitch)
    , useRadialReplication_(useRadialReplication)
    , name_(name)
    , color_(color)
{
    this->widget = widget;
}

void PatternCommand::execute()
{
    if (!widget || sourceIndices_.isEmpty()) {
        return;
    }
    TopoDS_Shape result;
    if (layoutType_ == PatternLayoutType::Circular) {
        result = widget->buildCircularPatternShape(
            sourceIndices_, patternOrigin_, direction1_, pitch1_, count1_,
            useRadialReplication_, direction2_, pitch2_, count2_);
    } else if (layoutType_ == PatternLayoutType::Polygonal) {
        result = widget->buildPolygonalPatternShape(
            sourceIndices_, patternOrigin_, direction1_, polygonSpanDegrees_, count1_,
            polygonSpacing_, polygonAlongEdgeCount_, polygonAlongEdgePitch_,
            useRadialReplication_, direction2_, pitch2_, count2_);
    } else {
        result = widget->buildLinearPatternShape(
            sourceIndices_, direction1_, pitch1_, count1_, useRadialReplication_, direction2_, pitch2_, count2_);
    }
    if (result.IsNull()) {
        return;
    }

    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    recipe.parentIndices = sourceIndices_;
    recipe.pattern.layoutType = layoutType_;
    recipe.pattern.sourceIndices = sourceIndices_;
    recipe.pattern.origin = patternOrigin_;
    recipe.pattern.direction1 = direction1_;
    recipe.pattern.direction2 = direction2_;
    recipe.pattern.pitch1 = pitch1_;
    recipe.pattern.pitch2 = pitch2_;
    recipe.pattern.count1 = count1_;
    recipe.pattern.count2 = count2_;
    recipe.pattern.useRadialReplication = useRadialReplication_;
    recipe.pattern.polygonSpanDegrees = polygonSpanDegrees_;
    recipe.pattern.polygonSpacing = polygonSpacing_;
    recipe.pattern.polygonAlongEdgeCount = polygonAlongEdgeCount_;
    recipe.pattern.polygonAlongEdgePitch = polygonAlongEdgePitch_;

    AddModelCommand addCmd(widget, result, name_, PATTERN, color_, pitch1_, pitch2_,
                           static_cast<double>(count1_), {}, false, &recipe);
    addCmd.execute();
    createdIndex_ = widget->getHistorySize() - 1;
}

void PatternCommand::undo()
{
    if (!widget || createdIndex_ < 0) {
        return;
    }
    widget->removeModelByIndex(createdIndex_);
    createdIndex_ = -1;
}

QString PatternCommand::getDescription() const
{
    return QStringLiteral("阵列特征");
}
