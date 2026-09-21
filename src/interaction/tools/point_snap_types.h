#ifndef INTERACTION_TOOLS_POINT_SNAP_TYPES_H
#define INTERACTION_TOOLS_POINT_SNAP_TYPES_H

#include <QList>
#include <QString>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

enum class PointSnapType {
    None = 0,
    Endpoint,
    Midpoint,
    Center,
    Quadrant,
    Intersection,
    Projection,
    Nearest,
    Grid,
    ArcMidpoint,
    OnCurve,
    OnFace
};

struct PointSnapMode {
    bool nearest = false;
    bool endpoint = false;
    bool midpoint = false;
    bool arcMidpoint = false;
    bool intersection = false;
    bool center = false;
    bool quadrant = false;
    bool projection = false;
    bool onCurve = false;
    bool onFace = false;
    bool anyPoint = false;
    bool grid = false;

    bool hasAnyType() const
    {
        return nearest || endpoint || midpoint || arcMidpoint || intersection
            || center || quadrant || projection || onCurve || onFace || anyPoint || grid;
    }
};

struct PointPickResult {
    bool valid = false;
    gp_Pnt point;
    PointSnapType snapType = PointSnapType::None;
    TopoDS_Shape sourceShape;
    TopoDS_Edge sourceEdge;
    double parameter = 0.0;
    bool hasParameter = false;
    bool isConfirmed = false;
};

struct PointCandidate {
    QString label;
    gp_Pnt point;
    PointSnapType snapType = PointSnapType::None;
    TopoDS_Shape sourceShape;
    TopoDS_Edge sourceEdge;
    double parameter = 0.0;
    bool hasParameter = false;
    double screenDistanceSquared = 0.0;
};

using PointCandidateList = QList<PointCandidate>;

#endif // INTERACTION_TOOLS_POINT_SNAP_TYPES_H
