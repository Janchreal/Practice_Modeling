#ifndef REVOLVE_GEOMETRY_H
#define REVOLVE_GEOMETRY_H

#include "platformmath.h"

#include <QList>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>

bool buildRevolutionCompound(const QList<TopoDS_Shape>& profiles,
                             const gp_Ax1& axis,
                             double angleRad,
                             TopoDS_Compound& outCompound,
                             double startAngleRad = 0.0);

#endif // REVOLVE_GEOMETRY_H
