#ifndef PRESENTATION_MAIN_WINDOW_PATTERN_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_PATTERN_WINDOW_STATE_H

#include <gp_Pnt.hxx>

#include <QList>

#include <vtkSmartPointer.h>

class QLineEdit;
class QWidget;
class PatternFeatureDialog;
class vtkActor;

/** Runtime state for pattern body selection and pitch interaction. */
class PatternWindowState {
protected:
    PatternFeatureDialog* patternDialog_ = nullptr;
    QList<int> patternSelectedIndices_;
    enum class PatternVectorPick { None, Direction1, Direction2 };
    PatternVectorPick patternVectorPick_ = PatternVectorPick::None;
    bool patternPitchInteractiveActive_ = false;
    int patternActivePitchAxis_ = 0;
    bool patternPitchDragActive_ = false;
    double patternDragStartPitch_ = 0.0;
    double patternDragStartParam_ = 0.0;
    double patternDragStartDim_ = 0.0;
    double patternDragAxisScrOx_ = 0.0;
    double patternDragAxisScrOy_ = 0.0;
    double patternDragAxisScrDx_ = 0.0;
    double patternDragAxisScrDy_ = 0.0;
    gp_Pnt patternArrayOrigin_;
    vtkSmartPointer<vtkActor> patternPreviewActor_;
    vtkSmartPointer<vtkActor> patternGizmoLineActor_;
    vtkSmartPointer<vtkActor> patternGizmoArrowActor_;
    QWidget* patternPitchOverlay_ = nullptr;
    QLineEdit* patternPitchEdit_ = nullptr;
};

#endif // PRESENTATION_MAIN_WINDOW_PATTERN_WINDOW_STATE_H
