#ifndef PRESENTATION_MAIN_WINDOW_COORDINATE_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_COORDINATE_WINDOW_STATE_H

#include "common/axisdirection.h"

#include <vtkSmartPointer.h>

class vtkActor;
class vtkTransform;

/**
 * Runtime state for the work/reference coordinate systems and view triad.
 *
 * The rendering and picking methods remain on Widget during the migration,
 * but ownership of the coordinate-system data lives in this state boundary.
 */
class CoordinateWindowState {
protected:
    vtkSmartPointer<vtkActor> centerAxisXActor;
    vtkSmartPointer<vtkActor> centerAxisYActor;
    vtkSmartPointer<vtkActor> centerAxisZActor;
    vtkSmartPointer<vtkActor> centerTriadFaceActors_[6];
    vtkSmartPointer<vtkActor> centerTriadEdgeActors_[12];
    int centerTriadHoveredFace_ = -1;
    int centerTriadHoveredEdge_ = -1;
    int centerTriadCurrentFace_ = -1;

    AxisDirection currentAxisDirection = AxisDirection::Z;
    double centerAxesBaseScale = 1.2;
    double centerAxesCurrentScale = 1.0;
    double centerAxesCameraDistance = 5.0;

    vtkSmartPointer<vtkActor> workCsysActor;
    vtkSmartPointer<vtkTransform> workCsysTransform;
    bool hasWorkCsys = false;
    bool workCsysDragActive = false;
    int workCsysHistoryIndex_ = -1;
    double workCsysLastPickWorld[3] = {0.0, 0.0, 0.0};

    vtkSmartPointer<vtkActor> workCsysAxisXActor_;
    vtkSmartPointer<vtkActor> workCsysAxisYActor_;
    vtkSmartPointer<vtkActor> workCsysAxisZActor_;
    vtkSmartPointer<vtkActor> workCsysLabelXActor_;
    vtkSmartPointer<vtkActor> workCsysLabelYActor_;
    vtkSmartPointer<vtkActor> workCsysLabelZActor_;

    vtkSmartPointer<vtkActor> refCsysActor_;
    vtkSmartPointer<vtkTransform> refCsysTransform_;
    bool hasReferenceCsys_ = false;
    int referenceCsysHistoryIndex_ = -1;
    vtkSmartPointer<vtkActor> refCsysAxisXActor_;
    vtkSmartPointer<vtkActor> refCsysAxisYActor_;
    vtkSmartPointer<vtkActor> refCsysAxisZActor_;
    vtkSmartPointer<vtkActor> refCsysLabelXActor_;
    vtkSmartPointer<vtkActor> refCsysLabelYActor_;
    vtkSmartPointer<vtkActor> refCsysLabelZActor_;
    vtkSmartPointer<vtkActor> refCsysPlaneXYActor_;
    vtkSmartPointer<vtkActor> refCsysPlaneYZActor_;
    vtkSmartPointer<vtkActor> refCsysPlaneXZActor_;
    vtkSmartPointer<vtkActor> refCsysPlaneFrameActor_;
};

#endif // PRESENTATION_MAIN_WINDOW_COORDINATE_WINDOW_STATE_H
