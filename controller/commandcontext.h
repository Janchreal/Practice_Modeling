#ifndef COMMANDCONTEXT_H
#define COMMANDCONTEXT_H

#include "axisdirection.h"
#include "featurerecipe.h"
#include "modelhistorysnapshot.h"
#include "modelinghistory.h"
#include "modeltype.h"

#include <QColor>
#include <QList>
#include <QString>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

class CommandContext {
public:
    virtual ~CommandContext() = default;

    virtual const QList<ModelingHistory>& getHistoryList() const = 0;
    virtual int getHistorySize() const = 0;

    virtual void assignFeatureRecipe(int index, const FeatureRecipe& recipe) = 0;

    virtual TopoDS_Shape rebuildBaseShapeFromExtrusionRecipe(const ExtrusionRecipeData& recipe) = 0;
    virtual bool applyDialogBooleanToShape(int boolMode, int boolTargetIndex,
                                           const TopoDS_Shape& featureShape,
                                           TopoDS_Shape& outShape) const = 0;

    virtual void performBooleanOperation(int targetIndex,
                                         const QList<int>& toolIndices,
                                         int operationType,
                                         bool keepTarget,
                                         bool keepTool) = 0;

    virtual void showModel(int index) = 0;
    virtual int createGeometryDirectly(ModelType type, const QString& name, const QColor& color,
                                       double param1, double param2 = 0, double param3 = 0,
                                       bool hasOrigin = false,
                                       double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                                       AxisDirection axisDirection = AxisDirection::Z,
                                       bool axisReversed = false,
                                       bool hasCustomVectorDir = false,
                                       const gp_Dir& customVectorDir = gp_Dir(0, 0, 1)) = 0;
    virtual void removeModel(int index) = 0;
    virtual void removeModelByIndex(int index) = 0;
    virtual void setActiveSketchGeometriesForCommand(const QList<TopoDS_Shape>& geometries) = 0;

    virtual TopoDS_Shape getShapeFromHistory(int index) = 0;
    virtual void displayOccShape(const TopoDS_Shape& shape, const QString& name,
                                 ModelType type, const QColor& color,
                                 double param1 = 0, double param2 = 0, double param3 = 0,
                                 bool hasOrigin = false,
                                 double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                                 AxisDirection axisDirection = AxisDirection::Z,
                                 bool axisReversed = false,
                                 bool hasCustomVectorDir = false,
                                 const gp_Dir& customVectorDir = gp_Dir(0, 0, 1)) = 0;

    virtual void setModelVisibleForCommand(int index, bool visible) = 0;
    virtual bool isModelVisibleForCommand(int index) const = 0;

    virtual void applyModelStateForCommand(int index, const TopoDS_Shape& shape, ModelType type,
                                           const QString& name, bool triggerCascade = true) = 0;
    virtual void setModelParametersForCommand(int index, double param1, double param2, double param3) = 0;
    virtual void regenerateModelForCommand(int index, bool triggerCascade = true) = 0;

    virtual ModelHistorySnapshot captureModelSnapshot(int index) const = 0;
    virtual CascadeUndoRecord beginCascadeUndoCapture(int rootIndex) const = 0;
    virtual void finishCascadeUndoCapture(CascadeUndoRecord& record) const = 0;
    virtual void restoreModelFromSnapshot(int index, const ModelHistorySnapshot& snapshot) = 0;
    virtual void restoreCascadeUndoStates(const CascadeUndoRecord& record, bool useBeforeStates) = 0;
};

#endif // COMMANDCONTEXT_H

