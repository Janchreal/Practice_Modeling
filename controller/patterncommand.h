#ifndef PATTERNCOMMAND_H
#define PATTERNCOMMAND_H

#include "command.h"
#include "modeltype.h"
#include "patternfeaturetypes.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

#include <QColor>
#include <QList>
#include <QString>

class CommandContext;

class PatternCommand : public Command
{
public:
    PatternCommand(CommandContext* widget,
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
                   double polygonSpanDegrees = 360.0,
                   PolygonSpacingMode polygonSpacing = PolygonSpacingMode::CountPerSide,
                   int polygonAlongEdgeCount = 2,
                   double polygonAlongEdgePitch = 10.0,
                   const QString& name = QString(),
                   const QColor& color = QColor(100, 180, 255));

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    PatternLayoutType layoutType_ = PatternLayoutType::Linear;
    gp_Pnt patternOrigin_;
    QList<int> sourceIndices_;
    gp_Dir direction1_;
    gp_Dir direction2_;
    double pitch1_ = 10.0;
    double pitch2_ = 10.0;
    int count1_ = 2;
    int count2_ = 2;
    double polygonSpanDegrees_ = 360.0;
    PolygonSpacingMode polygonSpacing_ = PolygonSpacingMode::CountPerSide;
    int polygonAlongEdgeCount_ = 2;
    double polygonAlongEdgePitch_ = 10.0;
    bool useRadialReplication_ = false;
    QString name_;
    QColor color_;
    int createdIndex_ = -1;
};

#endif // PATTERNCOMMAND_H

