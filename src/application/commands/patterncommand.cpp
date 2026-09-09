#include "patterncommand.h"
#include "application/commands/addmodelcommand.h"
#include "modeling_command_port.h"
#include "pattern_geometry.h"

PatternCommand::PatternCommand(ModelingCommandPort* context,
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
    : Command(context)
    , layoutType_(layoutType)
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
}

void PatternCommand::execute()
{
    if (!context_ || sourceIndices_.isEmpty()) {
        return;
    }
    QList<TopoDS_Shape> sourceShapes;
    for (int index : sourceIndices_) {
        if (index < 0) {
            continue;
        }
        const TopoDS_Shape shape = context_->getShapeFromHistory(index);
        if (!shape.IsNull()) {
            sourceShapes.append(shape);
        }
    }
    const TopoDS_Shape result = PatternGeometry::buildPatternShape(
        sourceShapes, layoutType_, patternOrigin_, direction1_, pitch1_, count1_,
        useRadialReplication_, direction2_, pitch2_, count2_, polygonSpanDegrees_,
        polygonSpacing_, polygonAlongEdgeCount_, polygonAlongEdgePitch_);
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

    AddModelCommand addCmd(context_, result, name_, PATTERN, color_, pitch1_, pitch2_,
                           static_cast<double>(count1_), {}, false, &recipe);
    addCmd.execute();
    createdIndex_ = context_->getHistorySize() - 1;
}

void PatternCommand::undo()
{
    if (!context_ || createdIndex_ < 0) {
        return;
    }
    context_->removeModelByIndex(createdIndex_);
    createdIndex_ = -1;
}

QString PatternCommand::getDescription() const
{
    return QStringLiteral("阵列特征");
}


