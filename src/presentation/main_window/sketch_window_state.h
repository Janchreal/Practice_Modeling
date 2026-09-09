#ifndef PRESENTATION_MAIN_WINDOW_SKETCH_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_SKETCH_WINDOW_STATE_H

#include "domain/sketch/sketch.h"

#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <QList>

#include <vtkSmartPointer.h>

class QDialog;
class QPushButton;
class SketchCircleModeDialog;
class SketchConicDialog;
class SketchCreateDialog;
class SketchEllipseAngleDialog;
class SketchEllipseDialog;
class SketchPolygonDialog;
class SketchPolygonValueDialog;
class SketchRectangleModeDialog;
class SketchToolInputDialog;
class vtkActor;
class vtkFollower;
class vtkLineSource;
class vtkPolyData;
class vtkSphereSource;

/**
 * Runtime state for the sketch workspace.
 *
 * This type deliberately contains state only. UI actions and geometry
 * operations remain on Widget for now, while the state boundary prevents
 * the main-window declaration from becoming the owner of every sketch field.
 */
class SketchWindowState {
protected:
    bool hasActiveSketch_ = false;
    Sketch activeSketch_;
    gp_Pln activeSketchPlane_;
    int activeSketchHistoryIndex_ = -1;
    SketchCreateDialog* activeSketchCreateDialog_ = nullptr;
    int sketchClickCount_ = 0;
    gp_Pnt sketchP1_, sketchP2_, sketchP3_;
    bool sketchChainTangentValid_ = false;
    gp_Dir sketchChainTangentDir_;
    bool sketchContourChaining_ = false;

    vtkSmartPointer<vtkActor> sketchPlaneHoverActor_;
    vtkSmartPointer<vtkActor> sketchSelectedPlaneFillActor_;
    vtkSmartPointer<vtkActor> sketchSelectedPlaneOutlineActor_;
    int sketchSelectedPlaneHistoryIndex_ = -1;

    vtkSmartPointer<vtkActor> sketchPreviewLineActor_;
    vtkSmartPointer<vtkLineSource> sketchPreviewLineSource_;
    vtkSmartPointer<vtkActor> sketchPreviewRectangleActor_;
    vtkSmartPointer<vtkPolyData> sketchPreviewRectanglePolyData_;
    vtkSmartPointer<vtkActor> sketchPreviewArcActor_;
    vtkSmartPointer<vtkPolyData> sketchPreviewArcPolyData_;
    vtkSmartPointer<vtkActor> sketchPreviewCircleActor_;
    vtkSmartPointer<vtkPolyData> sketchPreviewCirclePolyData_;
    vtkSmartPointer<vtkActor> sketchPreviewPolygonActor_;
    vtkSmartPointer<vtkPolyData> sketchPreviewPolygonPolyData_;
    bool sketchPreviewPolygonGuideDashed_ = false;
    vtkSmartPointer<vtkActor> sketchPreviewConicActor_;
    vtkSmartPointer<vtkPolyData> sketchPreviewConicPolyData_;
    vtkSmartPointer<vtkActor> sketchConicMarkerActors_[3];
    vtkSmartPointer<vtkSphereSource> sketchConicMarkerSpheres_[3];
    vtkSmartPointer<vtkActor> sketchEditHoverActor_;
    QList<vtkSmartPointer<vtkActor>> sketchCommittedOverlayActors_;
    bool sketchBrushActive_ = false;
    TopoDS_Shape sketchLastBrushShape_;

    SketchToolInputDialog* sketchToolInputDialog_ = nullptr;
    SketchRectangleModeDialog* sketchRectangleModeDialog_ = nullptr;
    SketchCircleModeDialog* sketchCircleModeDialog_ = nullptr;
    SketchConicDialog* sketchConicDialog_ = nullptr;
    SketchPolygonDialog* sketchPolygonDialog_ = nullptr;
    SketchPolygonValueDialog* sketchPolygonValueDialog_ = nullptr;
    int sketchPolygonPendingField_ = -1;
    bool sketchPolygonHasCenter_ = false;
    gp_Pnt sketchPolygonCenter_;
    SketchEllipseDialog* sketchEllipseDialog_ = nullptr;
    SketchEllipseAngleDialog* sketchEllipseAngleDialog_ = nullptr;
    int sketchEllipsePendingField_ = -1;
    bool sketchEllipseHasCenter_ = false;
    gp_Pnt sketchEllipseCenter_;
    int sketchEllipseGeomIndex_ = -1;
    bool sketchEllipseAngleDragActive_ = false;
    int sketchConicPendingField_ = -1;
    bool sketchConicHasP0_ = false;
    bool sketchConicHasP1_ = false;
    bool sketchConicHasPc_ = false;
    gp_Pnt sketchConicP0_, sketchConicP1_, sketchConicPc_;
    int sketchConicCommittedGeomIndex_ = -1;
    bool sketchConicDragActive_ = false;
    QPushButton* sketchCreationExclusiveButton_ = nullptr;
    gp_Pnt sketchLastHoverPoint_;
    bool sketchLastHoverValid_ = false;
    bool sketchCommittedPointValid_ = false;
    gp_Pnt sketchCommittedPoint_;
};

#endif // PRESENTATION_MAIN_WINDOW_SKETCH_WINDOW_STATE_H
