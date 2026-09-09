#ifndef PRESENTATION_MAIN_WINDOW_DIALOG_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_DIALOG_WINDOW_STATE_H

#include <vtkSmartPointer.h>

class BoolOperationDialog;
class ExtrusionDialog;
class revolvedialog;
class filletdialog;
class chamferdialog;
class datum_plane;
class DatumAxisDialog;
class CuboidParamsDialog;
class CylinderDialog;
class ConeParamsDialog;
class SphereParamsDialog;
class vtkActor;

/** Dialog instances and dialog-owned preview resources used by the main window. */
class DialogWindowState {
protected:
    BoolOperationDialog* boolDialog = nullptr;
    ExtrusionDialog* extrusionDialog = nullptr;
    revolvedialog* revolveDialog = nullptr;
    filletdialog* filletDialog = nullptr;
    chamferdialog* chamferDialog = nullptr;

    datum_plane* datumPlaneDialog_ = nullptr;
    DatumAxisDialog* datumAxisDialog_ = nullptr;
    vtkSmartPointer<vtkActor> datumPlanePreviewActor_;
    vtkSmartPointer<vtkActor> datumAxisPreviewActor_;

    CuboidParamsDialog* cuboidDialog = nullptr;
    CylinderDialog* cylinderDialog = nullptr;
    ConeParamsDialog* coneDialog = nullptr;
    SphereParamsDialog* sphereDialog = nullptr;
};

#endif // PRESENTATION_MAIN_WINDOW_DIALOG_WINDOW_STATE_H
