#ifndef PRESENTATION_MAIN_WINDOW_FEATURE_INTERACTION_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_FEATURE_INTERACTION_WINDOW_STATE_H

#include <IVtk_Types.hxx>

#include <QColor>
#include <QList>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <vtkSmartPointer.h>

class vtkActor;
class vtkFollower;

/**
 * Runtime state shared by extrusion/revolve and fillet/chamfer sessions.
 *
 * The state is intentionally free of action methods. Widget still provides
 * the transitional interaction API, while this class owns the session data
 * and VTK handles used by those APIs.
 */
class FeatureInteractionWindowState {
protected:
    enum class ExtrusionHandlePart { None, StartSphere, EndArrow };
    enum class RevolveHandlePart { None, StartSphere, EndArrow };

    ExtrusionHandlePart extrusionHandleHover_ = ExtrusionHandlePart::None;
    ExtrusionHandlePart extrusionHandleSelected_ = ExtrusionHandlePart::None;
    ExtrusionHandlePart extrusionHandleDrag_ = ExtrusionHandlePart::None;
    RevolveHandlePart revolveHandleHover_ = RevolveHandlePart::None;
    RevolveHandlePart revolveHandleSelected_ = RevolveHandlePart::None;
    RevolveHandlePart revolveHandleDrag_ = RevolveHandlePart::None;
    bool extrusionHandleDragging_ = false;
    bool revolveHandleDragging_ = false;
    bool featureOperationGhostMode_ = false;
    bool featureResultPreviewActive_ = false;
    QList<int> featureResultPreviewHiddenModelIndices_;
    double extrusionHandleDragGrab_ = 0.0;
    double revolveHandleDragGrabDeg_ = 0.0;
    int extrusionHandleSelectDownX_ = -1;
    int extrusionHandleSelectDownY_ = -1;
    int revolveHandleSelectDownX_ = -1;
    int revolveHandleSelectDownY_ = -1;
    vtkSmartPointer<vtkActor> extrusionHandleLineActor_;
    vtkSmartPointer<vtkActor> extrusionHandleSphereActor_;
    vtkSmartPointer<vtkActor> extrusionHandleArrowActor_;
    vtkSmartPointer<vtkActor> revolveHandleArcActor_;
    vtkSmartPointer<vtkActor> revolveHandleStartLineActor_;
    vtkSmartPointer<vtkActor> revolveHandleEndLineActor_;
    vtkSmartPointer<vtkActor> revolveHandleSphereActor_;
    vtkSmartPointer<vtkActor> revolveHandleArrowActor_;
    vtkSmartPointer<vtkActor> revolveHandleCenterActor_;
    QList<int> extrusionSelectedIndices;

    struct ExtrusionFaceSelection {
        int modelIndex = -1;
        IVtk_IdType subShapeId = static_cast<IVtk_IdType>(-1);
        TopoDS_Shape shape;
        TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
        bool isSketchContour = false;
        int sketchContourIndex = -1;

        TopoDS_Face getFace() const
        {
            if (shapeType == TopAbs_FACE && !shape.IsNull()) {
                return TopoDS::Face(shape);
            }
            return TopoDS_Face();
        }

        TopoDS_Wire getWire() const
        {
            if (shapeType == TopAbs_WIRE && !shape.IsNull()) {
                return TopoDS::Wire(shape);
            }
            return TopoDS_Wire();
        }

        TopoDS_Edge getEdge() const
        {
            if (shapeType == TopAbs_EDGE && !shape.IsNull()) {
                return TopoDS::Edge(shape);
            }
            return TopoDS_Edge();
        }

        bool operator==(const ExtrusionFaceSelection& other) const
        {
            return modelIndex == other.modelIndex
                && subShapeId == other.subShapeId
                && isSketchContour == other.isSketchContour
                && sketchContourIndex == other.sketchContourIndex;
        }
    };

    QList<ExtrusionFaceSelection> extrusionSelectedFaces;
    int extrudeRevolveSelectionEpoch_ = 0;
    ExtrusionFaceSelection hoveredFace;
    bool hasHoveredFace = false;
    QList<vtkSmartPointer<vtkActor>> extrusionFaceHighlightActors;
    vtkSmartPointer<vtkActor> extrusionHoverHighlightActor;
    vtkSmartPointer<vtkActor> extrusionHoverOutlineActor;
    int extrusionHoverDimModelIndex_ = -1;
    QColor extrusionHoverDimOriginalColor_;
    double extrusionHoverDimOriginalOpacity_ = 1.0;

    int filletTargetModelIndex = -1;
    QList<IVtk_IdType> filletSelectedEdgeSubIds_;
    QList<TopoDS_Edge> filletSelectedEdges_;
    QList<vtkSmartPointer<vtkActor>> filletEdgeHighlightActors_;
    TopoDS_Edge filletHoverEdge_;
    bool hasFilletHoverEdge_ = false;
    vtkSmartPointer<vtkActor> filletHoverEdgeActor_;
    IVtk_IdType filletHoverEdgeSubId_ = static_cast<IVtk_IdType>(-1);
    int filletHoverModelIndex_ = -1;
    vtkSmartPointer<vtkActor> filletRadiusHandleSphereActor_;
    vtkSmartPointer<vtkActor> filletRadiusHandleSide1Actor_;
    vtkSmartPointer<vtkActor> filletRadiusHandleSide2Actor_;
    vtkSmartPointer<vtkActor> filletRadiusHandleLine1Actor_;
    vtkSmartPointer<vtkActor> filletRadiusHandleLine2Actor_;
    bool filletRadiusHandleDragging_ = false;
    int filletRadiusActiveSide_ = -1;
    int filletRadiusHoverSide_ = -1;
    gp_Pnt filletRadiusEdgeMid_ = gp_Pnt(0, 0, 0);
    gp_Dir filletRadiusDir1_ = gp_Dir(0, 0, 1);
    gp_Dir filletRadiusDir2_ = gp_Dir(0, 0, -1);

    int chamferTargetModelIndex_ = -1;
    QList<IVtk_IdType> chamferSelectedEdgeSubIds_;
    QList<TopoDS_Edge> chamferSelectedEdges_;
    QList<vtkSmartPointer<vtkActor>> chamferEdgeHighlightActors_;
    TopoDS_Edge chamferHoverEdge_;
    bool hasChamferHoverEdge_ = false;
    vtkSmartPointer<vtkActor> chamferHoverEdgeActor_;
    IVtk_IdType chamferHoverEdgeSubId_ = static_cast<IVtk_IdType>(-1);
    int chamferHoverModelIndex_ = -1;
    vtkSmartPointer<vtkActor> chamferAsymHandleSphereActor_;
    vtkSmartPointer<vtkActor> chamferAsymHandleSide1Actor_;
    vtkSmartPointer<vtkActor> chamferAsymHandleSide2Actor_;
    vtkSmartPointer<vtkActor> chamferAsymHandleLine1Actor_;
    vtkSmartPointer<vtkActor> chamferAsymHandleLine2Actor_;
    bool chamferAsymHandleDragging_ = false;
    int chamferAsymActiveSide_ = -1;
    int chamferAsymHoverSide_ = -1;
    int chamferAsymDragStartX_ = -1;
    int chamferAsymDragStartY_ = -1;
    double chamferAsymStartD1_ = 0.0;
    double chamferAsymStartD2_ = 0.0;
    gp_Pnt chamferAsymEdgeMid_ = gp_Pnt(0, 0, 0);
    gp_Dir chamferAsymDir1_ = gp_Dir(0, 0, 1);
    gp_Dir chamferAsymDir2_ = gp_Dir(0, 0, -1);
};

#endif // PRESENTATION_MAIN_WINDOW_FEATURE_INTERACTION_WINDOW_STATE_H
