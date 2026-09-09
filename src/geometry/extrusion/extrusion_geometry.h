#ifndef EXTRUSION_GEOMETRY_H
#define EXTRUSION_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <list>
#include <string>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>

namespace ExtrusionGeometry {

TopoDS_Shape extrudeResolvedProfile(const TopoDS_Shape& profile,
                                    const gp_Dir& direction,
                                    double length,
                                    double startOffset,
                                    bool makeSheetBody);
bool buildExtrusionCompound(const QList<TopoDS_Shape>& profiles,
                            const gp_Dir& direction,
                            double startDistance,
                            double endDistance,
                            bool makeSheetBody,
                            TopoDS_Compound& outCompound);

TopoDS_Shape prepareSolidExtrusionProfile(const TopoDS_Shape& sourceShape);
TopoDS_Shape prepareSolidExtrusionProfile(const QList<TopoDS_Edge>& edges);

bool buildTaperedExtrusionDrafts(const TopoDS_Shape& sourceShape,
                                 const gp_Dir& direction,
                                 double lengthFwd,
                                 double lengthRev,
                                 bool solid,
                                 double taperAngleFwd,
                                 double taperAngleRev,
                                 std::list<TopoDS_Shape>& drafts,
                                 std::string* errorMessage = nullptr);

} // namespace ExtrusionGeometry

#endif // EXTRUSION_GEOMETRY_H
