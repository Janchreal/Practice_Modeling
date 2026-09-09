#ifndef PRESENTATION_MAIN_WINDOW_PRIMITIVE_INTERACTION_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_PRIMITIVE_INTERACTION_WINDOW_STATE_H

#include <gp_Pnt.hxx>

#include <vtkSmartPointer.h>

class QLineEdit;
class QWidget;
class vtkActor;
class vtkFollower;

/** Runtime state for the interactive cuboid preview and its Gizmo. */
class PrimitiveInteractionWindowState {
protected:
    enum class CuboidGizmoPart { None, Origin, AxisLength, AxisWidth, AxisHeight };

    bool cuboidInteractiveActive_ = false;
    CuboidGizmoPart cuboidHoverPart_ = CuboidGizmoPart::None;
    CuboidGizmoPart cuboidDragPart_ = CuboidGizmoPart::None;
    bool cuboidDragActive_ = false;
    gp_Pnt cuboidInteractiveOrigin_;
    double cuboidInteractiveLength_ = 2.0;
    double cuboidInteractiveWidth_ = 2.0;
    double cuboidInteractiveHeight_ = 2.0;
    double cuboidDragStartParam_ = 0.0;
    double cuboidDragStartDim_ = 0.0;
    double cuboidDragAxisScrOx_ = 0.0;
    double cuboidDragAxisScrOy_ = 0.0;
    double cuboidDragAxisScrDx_ = 0.0;
    double cuboidDragAxisScrDy_ = 0.0;
    vtkSmartPointer<vtkActor> cuboidPreviewBoxActor_;
    vtkSmartPointer<vtkActor> cuboidGizmoOriginActor_;
    vtkSmartPointer<vtkActor> cuboidGizmoAxisLineActors_[3];
    vtkSmartPointer<vtkActor> cuboidGizmoAxisArrowActors_[3];
    vtkSmartPointer<vtkFollower> cuboidGizmoAxisLabelActors_[3];
    QWidget* cuboidDimOverlayWidgets_[3] = {nullptr, nullptr, nullptr};
    QLineEdit* cuboidDimEdits_[3] = {nullptr, nullptr, nullptr};
    int cuboidActiveDimAxis_ = -1;
};

#endif // PRESENTATION_MAIN_WINDOW_PRIMITIVE_INTERACTION_WINDOW_STATE_H
