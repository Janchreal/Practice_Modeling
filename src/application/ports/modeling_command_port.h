#ifndef MODELING_COMMAND_PORT_H
#define MODELING_COMMAND_PORT_H

#include "geometry/placement/axis_placement.h"
#include "common/axisdirection.h"
#include "domain/features/featurerecipe.h"
#include "application/history/modeling_history_record.h"
#include "application/history/model_history_snapshot.h"
#include "common/modeltype.h"
#include "geometry/primitives/primitive_build_request.h"

#include <QColor>
#include <QList>
#include <QString>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

// Application-facing port used by modeling commands. The current main window
// implements it as an adapter; commands do not depend on QMainWindow.
class ModelingCommandPort {
public:
    virtual ~ModelingCommandPort() = default;

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
    virtual int createGeometryDirectly(const QString& name, const QColor& color,
                                       const PrimitiveGeometry::PrimitiveBuildRequest& request) = 0;
    virtual void removeModel(int index) = 0;
    virtual void removeModelByIndex(int index) = 0;
    virtual void setActiveSketchGeometriesForCommand(const QList<TopoDS_Shape>& geometries) = 0;

    virtual TopoDS_Shape getShapeFromHistory(int index) = 0;
    virtual void displayOccShape(const TopoDS_Shape& shape, const QString& name,
                                 ModelType type, const QColor& color,
                                 double param1 = 0, double param2 = 0, double param3 = 0,
                                 const GeometryPlacement::AxisPlacement& placement =
                                     GeometryPlacement::AxisPlacement()) = 0;

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

#endif // MODELING_COMMAND_PORT_H

