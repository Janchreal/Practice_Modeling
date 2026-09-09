#ifndef INTERACTION_SELECTION_SELECTION_WINDOW_STATE_H
#define INTERACTION_SELECTION_SELECTION_WINDOW_STATE_H

#include <IVtk_Types.hxx>

#include <QList>

#include <vtkSmartPointer.h>

class vtkActor;

/** Shared selection mode and transient selection state for a modeling view. */
class SelectionWindowState {
public:
    enum SelectionMode {
        None,
        SelectTarget,
        SelectTool,
        ExtrusionSelection,
        FaceSelection,
        EdgeSelection,
        FilletEdgeSelection,
        FilletRadiusHandleDrag,
        ChamferEdgeSelection,
        ChamferAsymHandleDrag,
        PointSelection,
        VectorDialogPickDirection,
        VectorDialogPickStartPoint,
        VectorDialogPickEndPoint,
        VectorTwoPointInteractive,
        VectorTwoPointHandleDrag,
        WorkCsysPlacement,
        WorkCsysDrag,
        SketchPlaneSelection,
        SketchDrawLine,
        SketchDrawArc,
        SketchDrawRectangle,
        SketchDrawCircle,
        SketchDrawPoint,
        SketchDrawPolygon,
        SketchPolygonPick,
        SketchEllipsePick,
        SketchEllipseAdjust,
        SketchConicPick,
        SketchConicDragControl,
        SketchQuickTrim,
        SketchQuickExtend,
        CuboidInteractive,
        PatternBodySelection,
        PatternPitchInteractive,
        ExtrusionHandleDrag,
        RevolveHandleDrag,
        FeatureBooleanTargetSelect
    };

protected:
    SelectionMode currentSelectionMode = None;
    int selectedTargetIndex = -1;
    QList<int> selectedToolIndices;
    QList<IVtk_IdType> selectedSubShapeIds;
    vtkSmartPointer<vtkActor> faceHighlightActor;
    vtkSmartPointer<vtkActor> edgeHighlightActor;
    int currentPickedModelIndex = -1;

    int currentSelectedIndex = -1;
    int hoveredModelIndex_ = -1;
    int hoverBaseSelectedIndex_ = -2;

    SelectionMode extrusionHandleSavedMode_ = None;
    SelectionMode revolveHandleSavedMode_ = None;
    SelectionMode vectorTwoPointHandleSavedMode_ = None;
};

#endif // INTERACTION_SELECTION_SELECTION_WINDOW_STATE_H
