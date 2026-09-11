#ifndef PRESENTATION_MAIN_WINDOW_VECTOR_SNAP_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_VECTOR_SNAP_WINDOW_STATE_H

#include <IVtk_Types.hxx>

#include <QColor>
#include <QList>
#include <QPointer>

#include <TopoDS_Edge.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <vtkSmartPointer.h>

class QLineEdit;
class QWidget;
class vectordialog;
class vtkActor;
class vtkFollower;
class vtkTransform;

/**
 * Runtime state for vector picking, point snapping, and origin picking.
 *
 * Picking actions remain on Widget while the tool state is kept in one
 * boundary shared by modeling and sketch workflows.
 */
class VectorSnapWindowState {
protected:
    QPointer<vectordialog> vectorDialog_;
    bool hasCustomVectorDir_ = false;
    gp_Dir customVectorDir_ = gp_Dir(0, 0, 1);
    bool hasVectorDialogBaseDir_ = false;
    gp_Dir vectorDialogBaseDir_ = gp_Dir(0, 0, 1);
    bool vectorDialogReverse_ = false;
    int vectorDialogModeIndex_ = 0;

    bool hasVectorDialogCurveEdge_ = false;
    TopoDS_Edge vectorDialogCurveEdge_;
    double vectorDialogCurveTotalLen_ = 0.0;
    int vectorDialogCurvePosMode_ = 0;
    double vectorDialogCurvePosValue_ = 0.0;
    bool hasVectorStartPoint_ = false;
    bool hasVectorEndPoint_ = false;
    gp_Pnt vectorStartPoint_;
    gp_Pnt vectorEndPoint_;

    enum class VectorTwoPointHandlePart { None, StartSphere, EndSphere, DirectionArrow };
    VectorTwoPointHandlePart vectorTwoPointHandleHover_ = VectorTwoPointHandlePart::None;
    VectorTwoPointHandlePart vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
    VectorTwoPointHandlePart vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
    bool vectorTwoPointHandleDragging_ = false;
    bool vectorTwoPointHandlesVisible_ = false;
    bool vectorTwoPointAwaitingEndPick_ = false;
    int vectorTwoPointHandleSelectDownX_ = -1;
    int vectorTwoPointHandleSelectDownY_ = -1;
    vtkSmartPointer<vtkActor> vectorTwoPointStartSphereActor_;
    vtkSmartPointer<vtkActor> vectorTwoPointEndSphereActor_;
    vtkSmartPointer<vtkActor> vectorTwoPointLineActor_;
    QList<vtkSmartPointer<vtkActor>> vectorTwoPointSnapGhostActors_;
    struct VectorSnapPreviewCandidate {
        gp_Pnt point;
        double screenDistanceSquared = 0.0;
        int type = 0; // 1=endpoint, 2=midpoint
        int modelIndex = -1;
        IVtk_IdType subShapeId = static_cast<IVtk_IdType>(-1);
    };
    QList<VectorSnapPreviewCandidate> vectorSnapPreviewCandidates_;
    gp_Pnt snapHoverBestPoint_;
    bool hasSnapHoverBestPoint_ = false;
    bool vectorTwoPointSnapHasHoveredEdge_ = false;

    vtkSmartPointer<vtkActor> vectorDialogArrowActor_;
    vtkSmartPointer<vtkTransform> vectorDialogArrowTransform_;
    vtkSmartPointer<vtkActor> vectorDialogHoverShapeActor_;
    vtkSmartPointer<vtkActor> vectorDialogHoverOutlineActor_;
    bool hasVectorDialogArrowOrigin_ = false;
    gp_Pnt vectorDialogArrowOrigin_;
    int vectorDialogHoverModelIndex_ = -1;
    IVtk_IdType vectorDialogHoverSubShapeId_ = static_cast<IVtk_IdType>(-1);
    int vectorDialogHoverDimModelIndex_ = -1;
    QColor vectorDialogHoverDimOriginalColor_;
    double vectorDialogHoverDimOriginalOpacity_ = 1.0;
    int vectorTwoPointStartSnapKind_ = 1;
    int vectorTwoPointEndSnapKind_ = 1;

    struct SnapSettings {
        bool enabled = false;
        bool armed = false;
        bool nearest = false;
        bool endpoint = false;
        bool midpoint = false;
        bool arcMidpoint = false;
        bool intersection = false;
        bool center = false;
        bool quadrant = false;
        bool onCurve = false;
        bool onFace = false;
    };
    SnapSettings snap_;
    gp_Pnt snapSelectedPoint_;
    bool hasSnapSelectedPoint_ = false;
    QList<gp_Pnt> snapPersistentPoints_;
    QList<vtkSmartPointer<vtkActor>> snapPersistentPointActors_;
    vtkSmartPointer<vtkActor> snapHoverPointActor_;
    vtkSmartPointer<vtkFollower> snapHoverTextActor_;
    vtkSmartPointer<vtkActor> snapHoverShapeActor_;
    vtkSmartPointer<vtkActor> snapSelectedPointActor_;
    vtkSmartPointer<vtkActor> snapSelectedShapeActor_;
    bool snapPickGhostOwned_ = false;

    struct SnapHoverOptions {
        bool suppressHoverBall;
        bool suppressHoverText;
        bool showCandidateGhosts;
        double expandScreenPixelRadius;

        SnapHoverOptions()
            : suppressHoverBall(false)
            , suppressHoverText(false)
            , showCandidateGhosts(false)
            , expandScreenPixelRadius(0.0)
        {
        }
    };

    double tempOriginX = 0.0;
    double tempOriginY = 0.0;
    double tempOriginZ = 0.0;
    bool hasTempOrigin = false;
    vtkSmartPointer<vtkActor> hoverPointActor;
    vtkSmartPointer<vtkActor> selectedPointActor;
    vtkSmartPointer<vtkFollower> hoverTextActor;
    vtkSmartPointer<vtkFollower> selectedTextActor;
    gp_Pnt selectedOriginPoint;
    bool hasSelectedOriginPoint = false;

    enum class OriginDialogKind {
        None = 0,
        Cuboid,
        Cylinder,
        Cone,
        Sphere,
        Revolve,
        Pattern
    };
    OriginDialogKind pendingOriginDialogKind_ = OriginDialogKind::None;
    bool originSnapSelectionActive_ = false;
    int pendingOriginSnapKind_ = -1;
};

#endif // PRESENTATION_MAIN_WINDOW_VECTOR_SNAP_WINDOW_STATE_H
