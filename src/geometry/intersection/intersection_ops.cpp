#include "intersection_ops.h"

#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>

#include <algorithm>

namespace {

bool boundingBoxesOverlap(const TopoDS_Shape& first,
                          const TopoDS_Shape& second,
                          double fuzzyValue)
{
    if (first.IsNull() || second.IsNull()) {
        return false;
    }

    try {
        Bnd_Box firstBox;
        Bnd_Box secondBox;
        BRepBndLib::Add(first, firstBox);
        BRepBndLib::Add(second, secondBox);
        if (firstBox.IsVoid() || secondBox.IsVoid()) {
            return false;
        }

        const double tolerance = std::max(
            Precision::Confusion() * 10.0,
            std::max(0.0, fuzzyValue));
        firstBox.Enlarge(tolerance);
        secondBox.Enlarge(tolerance);
        return !firstBox.IsOut(secondBox);
    } catch (...) {
        return false;
    }
}

} // namespace

namespace IntersectionOps {

TopoDS_Shape computeSection(const TopoDS_Shape& first,
                            const TopoDS_Shape& second,
                            double fuzzyValue)
{
    if (!boundingBoxesOverlap(first, second, fuzzyValue)) {
        return TopoDS_Shape();
    }

    try {
        BRepAlgoAPI_Section section;
        section.Init1(first);
        section.Init2(second);
        if (fuzzyValue > 0.0) {
            section.SetFuzzyValue(fuzzyValue);
        }
        section.Approximation(Standard_False);
        section.Build();
        if (!section.IsDone() || section.HasErrors()) {
            return TopoDS_Shape();
        }
        return section.Shape();
    } catch (...) {
        return TopoDS_Shape();
    }
}

bool hasIntersection(const TopoDS_Shape& first,
                     const TopoDS_Shape& second,
                     double fuzzyValue)
{
    return !computeSection(first, second, fuzzyValue).IsNull();
}

} // namespace IntersectionOps
