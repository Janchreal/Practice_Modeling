#include "sketch.h"
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <gp_Ax3.hxx>

namespace {
static TopoDS_Shape buildCompoundFromGeometries(const QList<TopoDS_Shape>& geometries)
{
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    for (const TopoDS_Shape& geom : geometries) {
        if (!geom.IsNull()) {
            builder.Add(compound, geom);
        }
    }
    return compound;
}
}

Sketch::Sketch()
    : sketchPlane(gp_Pln(gp_Ax3())), valid(false)
{
}

Sketch::~Sketch()
{
}

void Sketch::setPlane(const gp_Pln& plane)
{
    sketchPlane = plane;
}

gp_Pln Sketch::getPlane() const
{
    return sketchPlane;
}

void Sketch::addGeometry(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return;
    }
    geometries.append(shape);
    sketchShape = buildCompoundFromGeometries(geometries);
    valid = !geometries.isEmpty() && !sketchShape.IsNull();
}

QList<TopoDS_Shape> Sketch::getGeometries() const
{
    return geometries;
}

void Sketch::setGeometries(const QList<TopoDS_Shape>& shapes)
{
    geometries = shapes;
    sketchShape = buildCompoundFromGeometries(geometries);
    valid = !geometries.isEmpty() && !sketchShape.IsNull();
}

TopoDS_Shape Sketch::getShape() const
{
    return sketchShape;
}

bool Sketch::isValid() const
{
    return valid && !sketchShape.IsNull();
}

void Sketch::setName(const QString& name)
{
    sketchName = name;
}

QString Sketch::getName() const
{
    return sketchName;
}

void Sketch::clear()
{
    geometries.clear();
    sketchShape = TopoDS_Shape();
    valid = false;
}

