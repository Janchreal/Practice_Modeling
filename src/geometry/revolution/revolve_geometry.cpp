#include "revolve_geometry.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopoDS_Shape.hxx>

#include <cmath>

bool buildRevolutionCompound(const QList<TopoDS_Shape>& profiles,
                             const gp_Ax1& axis,
                             double angleRad,
                             TopoDS_Compound& outCompound,
                             double startAngleRad)
{
    BRep_Builder builder;
    builder.MakeCompound(outCompound);

    gp_Trsf startRot;
    const bool needStart = std::abs(startAngleRad) > 1e-12;
    if (needStart) {
        startRot.SetRotation(axis, startAngleRad);
    }

    bool hasValid = false;
    for (const TopoDS_Shape& profileShape : profiles) {
        if (profileShape.IsNull()) continue;

        TopoDS_Shape profile = profileShape;
        if (needStart) {
            BRepBuilderAPI_Transform mover(profile, startRot, true);
            profile = mover.Shape();
        }

        BRepPrimAPI_MakeRevol revol(profile, axis, angleRad);
        if (!revol.IsDone()) continue;

        builder.Add(outCompound, revol.Shape());
        hasValid = true;
    }
    return hasValid;
}
