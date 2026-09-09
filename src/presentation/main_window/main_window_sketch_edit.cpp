#include "main_window.h"
#include "ui_main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "application/commands/sketcheditcommand.h"
#include "geometry/sketch/sketch_geometry.h"

#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <limits>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <gp_Circ.hxx>
#include <gp_Lin.hxx>
#include <vtkRenderWindow.h>

bool Widget::isSketchEditMode(SelectionMode mode) const
{
    return mode == SketchQuickTrim || mode == SketchQuickExtend;
}

void Widget::startSketchQuickTrim()
{
    exitSketchCreationMode();
    ensureDefaultSketchForTools();
    if (hasActiveSketch_ && activeSketch_.isValid()) {
        ensureSketchHistoryRecord();
        if (!activeSketch_.getGeometries().isEmpty()) {
            updateSketchHistoryShape();
        }
    }
    sketchClickCount_ = 0;
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewArc();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchEditHover();
    currentSelectionMode = SketchQuickTrim;
    statusBar()->showMessage(tr("快速修剪：单击修剪，按住左键拖动可画笔批量修剪（快捷键 T）。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::startSketchQuickExtend()
{
    exitSketchCreationMode();
    ensureDefaultSketchForTools();
    if (hasActiveSketch_ && activeSketch_.isValid()) {
        ensureSketchHistoryRecord();
        if (!activeSketch_.getGeometries().isEmpty()) {
            updateSketchHistoryShape();
        }
    }
    sketchClickCount_ = 0;
    clearSketchPreviewLine();
    clearSketchPreviewRectangle();
    clearSketchPreviewArc();
    clearSketchPreviewCircle();
    clearSketchPreviewConic();
    closeSketchToolInput();
    closeSketchRectangleModeDialog();
    closeSketchCircleModeDialog();
    clearSketchEditHover();
    currentSelectionMode = SketchQuickExtend;
    statusBar()->showMessage(tr("快速延伸：单击靠近端点延伸，按住左键拖动可画笔批量延伸（快捷键 E）。"), 4000);
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::beginSketchBrushStroke()
{
    if (!isSketchEditMode(currentSelectionMode)) return;
    sketchBrushActive_ = true;
    sketchLastBrushShape_.Nullify();
}

void Widget::endSketchBrushStroke()
{
    sketchBrushActive_ = false;
    sketchLastBrushShape_.Nullify();
}

void Widget::clearSketchEditHover()
{
    if (sketchEditHoverActor_ && renderer) {
        removeSceneActor(sketchEditHoverActor_);
        sketchEditHoverActor_ = nullptr;
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

bool Widget::tryPickSketchEdgeAt(int x, int y, TopoDS_Edge& outEdge, gp_Pnt* outWorldPoint, double* outCurveParam)
{
    if (!shapePicker || !renderer) return false;
    if (activeSketchHistoryIndex_ < 0 || activeSketchHistoryIndex_ >= historyList.size()) return false;
    ModelingHistory& sketchRec = historyList[activeSketchHistoryIndex_];
    if (!renderStateFor(sketchRec).actor || renderStateFor(sketchRec).actor->GetVisibility() == 0 || !renderStateFor(sketchRec).shapeDataSource) return false;

    IVtkTools_ShapeObject::SetShapeSource(renderStateFor(sketchRec).shapeDataSource, renderStateFor(sketchRec).actor);
    renderStateFor(sketchRec).shapeDataSource->Modified();
    renderStateFor(sketchRec).shapeDataSource->Update();

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);

    bool hasWorldPoint = false;
    if (outWorldPoint) {
        gp_Pnt p;
        if (tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            *outWorldPoint = p;
            hasWorldPoint = true;
        }
    }

    if (renderStateFor(sketchRec).shapeWrapper.IsNull()) return false;
    const IVtk_IdType shapeId = renderStateFor(sketchRec).shapeWrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeId);
    if (subIds.IsEmpty()) return false;

    const QList<TopoDS_Shape> sketchGeoms = activeSketch_.getGeometries();
    TopoDS_Edge bestEdge;
    double bestDist = std::numeric_limits<double>::max();
    double bestParam = 0.0;

    for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
        const TopoDS_Shape sh = renderStateFor(sketchRec).shapeWrapper->GetSubShape(it.Value());
        if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
        const TopoDS_Edge candidate = TopoDS::Edge(sh);
        const gp_Pnt refPoint = hasWorldPoint ? *outWorldPoint : gp_Pnt();
        const double dist = hasWorldPoint
            ? SketchGeometry::distancePointToEdge(refPoint, candidate)
            : 0.0;
        if (!hasWorldPoint || dist < bestDist) {
            bestDist = dist;
            bestEdge = candidate;
            if (hasWorldPoint) {
                gp_Pnt projected;
                if (SketchGeometry::projectPointOntoEdge(refPoint, candidate, projected, &bestParam)) {
                    (void)projected;
                }
            }
        }
    }

    if (bestEdge.IsNull()) return false;

    const int idx = SketchGeometry::findSketchEdgeIndex(sketchGeoms, bestEdge);
    outEdge = (idx >= 0) ? TopoDS::Edge(sketchGeoms[idx]) : bestEdge;

    if (outWorldPoint && !hasWorldPoint) {
        SketchGeometry::edgeMidPoint(outEdge, *outWorldPoint);
    }
    if (outCurveParam) {
        *outCurveParam = bestParam;
    }
    return true;
}

void Widget::handleSketchEditHover(int x, int y)
{
    TopoDS_Edge picked;
    if (!tryPickSketchEdgeAt(x, y, picked)) {
        clearSketchEditHover();
        return;
    }
    if (sketchEditHoverActor_ && renderer) {
        removeSceneActor(sketchEditHoverActor_);
        sketchEditHoverActor_ = nullptr;
    }
    sketchEditHoverActor_ = buildSnapShapeHighlightActor(
        picked, 1.0, 0.75, 0.0, 0.95, 4.0
    );
    if (sketchEditHoverActor_ && renderer) {
        addAppearanceActor(sketchEditHoverActor_);
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::handleSketchEditClick(int x, int y, bool brushMode)
{
    TopoDS_Edge picked;
    gp_Pnt clickPoint;
    double clickParam = 0.0;
    if (!tryPickSketchEdgeAt(x, y, picked, &clickPoint, &clickParam)) return;
    if (brushMode && !sketchLastBrushShape_.IsNull() && sketchLastBrushShape_.IsSame(picked)) {
        return;
    }

    bool changed = false;
    if (currentSelectionMode == SketchQuickTrim) {
        changed = applyQuickTrimAt(picked, clickPoint);
    } else if (currentSelectionMode == SketchQuickExtend) {
        changed = applyQuickExtendAt(picked, clickPoint);
    }

    if (changed) {
        sketchLastBrushShape_ = picked;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    } else if (statusBar()) {
        if (currentSelectionMode == SketchQuickTrim) {
            statusBar()->showMessage(tr("修剪失败：未找到相交边界，或点击段过短。"), 2500);
        } else {
            statusBar()->showMessage(tr("延伸失败：未找到可延伸到的相交曲线。"), 2500);
        }
    }
}

bool Widget::replaceSketchEdgeWith(const TopoDS_Edge& targetEdge, const QList<TopoDS_Edge>& replacements, const QString& actionName)
{
    if (!hasActiveSketch_) return false;
    // 修改几何前先移除悬停高亮，避免引用旧 polydata/旧 edge 造成渲染崩溃
    clearSketchEditHover();
    const QList<TopoDS_Shape> before = activeSketch_.getGeometries();
    if (before.isEmpty()) return false;

    const int removeIdx = SketchGeometry::findSketchEdgeIndex(before, targetEdge);
    if (removeIdx < 0) return false;

    QList<TopoDS_Shape> after;
    after.reserve(before.size() + replacements.size());
    for (int i = 0; i < before.size(); ++i) {
        if (i == removeIdx) continue;
        after.append(before[i]);
    }

    for (const TopoDS_Edge& e : replacements) {
        if (!e.IsNull()) after.append(e);
    }

    if (after.isEmpty()) return false;
    executeCommand(new SketchEditCommand(this, before, after, actionName));
    return true;
}

bool Widget::applyQuickTrimAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint)
{
    try {
        if (targetEdge.IsNull()) return false;
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) targetCurve = BRep_Tool::Curve(targetEdge, f, l);
        if (targetCurve.IsNull()) return false;
        const double lo = qMin(f, l);
        const double hi = qMax(f, l);

        gp_Pnt projectedClick;
        double tClick = 0.0;
        if (!SketchGeometry::projectPointOntoEdge(clickPoint, targetEdge, projectedClick, &tClick)) return false;
        (void)projectedClick;

        auto isSameSupportCurve = [](const Handle(Geom_Curve)& a, const Handle(Geom_Curve)& b) -> bool {
            if (a.IsNull() || b.IsNull()) return false;
            if (a == b) return true;

            if (a->IsKind(STANDARD_TYPE(Geom_Line)) && b->IsKind(STANDARD_TYPE(Geom_Line))) {
                const gp_Lin la = Handle(Geom_Line)::DownCast(a)->Lin();
                const gp_Lin lb = Handle(Geom_Line)::DownCast(b)->Lin();
                const double dot = std::abs(la.Direction().Dot(lb.Direction()));
                if (dot < 1.0 - 1.0e-9) return false;
                // 两条线方向平行且彼此距离接近 0，则视作同一支撑直线（重合/共线）
                return la.Distance(lb.Location()) <= 1.0e-7;
            }

            if (a->IsKind(STANDARD_TYPE(Geom_Circle)) && b->IsKind(STANDARD_TYPE(Geom_Circle))) {
                const gp_Circ ca = Handle(Geom_Circle)::DownCast(a)->Circ();
                const gp_Circ cb = Handle(Geom_Circle)::DownCast(b)->Circ();
                const double rDiff = std::abs(ca.Radius() - cb.Radius());
                if (rDiff > 1.0e-7) return false;
                const double centerDist = ca.Location().Distance(cb.Location());
                if (centerDist > 1.0e-7) return false;
                const double axisDot = std::abs(ca.Axis().Direction().Dot(cb.Axis().Direction()));
                return axisDot >= 1.0 - 1.0e-9;
            }

            return false;
        };
        const Handle(Geom_Curve) targetBasis = SketchGeometry::sketchBasisCurve(targetCurve);

        QList<double> hits;
        for (const TopoDS_Shape& sh : activeSketch_.getGeometries()) {
            if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
            if (SketchGeometry::sketchEdgesEquivalent(TopoDS::Edge(sh), targetEdge)) continue;
            Standard_Real bf = 0.0, bl = 0.0;
            Handle(Geom_Curve) bCurve = BRep_Tool::Curve(TopoDS::Edge(sh), bf, bl);
            if (bCurve.IsNull()) continue;
            const Handle(Geom_Curve) bBasis = SketchGeometry::sketchBasisCurve(bCurve);
            // 跳过“同一支撑曲线”的边（例如第一次修剪后产生的共线兄弟线段），
            // 可避免 GeomAPI_ExtremaCurveCurve 在重合/退化情形下触发底层崩溃。
            if (isSameSupportCurve(targetBasis, bBasis)) continue;

            try {
                GeomAPI_ExtremaCurveCurve inter(targetCurve, bCurve);
                if (inter.NbExtrema() < 1) continue;
                for (int i = 1; i <= inter.NbExtrema(); ++i) {
                    gp_Pnt p1, p2;
                    inter.Points(i, p1, p2);
                    if (p1.Distance(p2) > 1.0e-6) continue;
                    gp_Pnt projected;
                    double t = 0.0;
                    if (!SketchGeometry::projectPointOntoEdge(p1, targetEdge, projected, &t)) continue;
                    if (t >= lo + 1.0e-8 && t <= hi - 1.0e-8) {
                        hits.append(t);
                    }
                }
            } catch (...) {
                continue;
            }
        }

        if (hits.isEmpty()) return false;

        double bestT = hits.first();
        double bestDist = std::abs(bestT - tClick);
        for (double t : hits) {
            const double d = std::abs(t - tClick);
            if (d < bestDist) {
                bestDist = d;
                bestT = t;
            }
        }
        if (bestDist <= 1.0e-7) return false;

        const double cutA = qMin(bestT, tClick);
        const double cutB = qMax(bestT, tClick);
        QList<TopoDS_Edge> kept;
        if (cutA - lo > 1.0e-7) {
            BRepBuilderAPI_MakeEdge mkA(targetCurve, lo, cutA);
            if (mkA.IsDone() && !mkA.Edge().IsNull()) kept.append(mkA.Edge());
        }
        if (hi - cutB > 1.0e-7) {
            BRepBuilderAPI_MakeEdge mkB(targetCurve, cutB, hi);
            if (mkB.IsDone() && !mkB.Edge().IsNull()) kept.append(mkB.Edge());
        }
        if (kept.isEmpty()) return false;

        return replaceSketchEdgeWith(targetEdge, kept, tr("快速修剪"));
    } catch (...) {
        return false;
    }
}

bool Widget::applyQuickExtendAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint)
{
    try {
        if (targetEdge.IsNull()) return false;
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) targetCurve = BRep_Tool::Curve(targetEdge, f, l);
        if (targetCurve.IsNull()) return false;

        const double lo = qMin(f, l);
        const double hi = qMax(f, l);
        const gp_Pnt pStart = targetCurve->Value(lo);
        const gp_Pnt pEnd = targetCurve->Value(hi);
        const bool extendStart = (pStart.Distance(clickPoint) <= pEnd.Distance(clickPoint));
        const double refParam = extendStart ? lo : hi;

        gp_Pnt pRef;
        gp_Vec d1;
        targetCurve->D1(refParam, pRef, d1);
        if (d1.Magnitude() <= Precision::Confusion()) return false;
        const gp_Dir tangent = extendStart ? gp_Dir(-d1.X(), -d1.Y(), -d1.Z()) : gp_Dir(d1);

        bool found = false;
        double bestParam = refParam;
        double bestDist = std::numeric_limits<double>::max();

        for (const TopoDS_Shape& sh : activeSketch_.getGeometries()) {
            if (sh.IsNull() || sh.ShapeType() != TopAbs_EDGE) continue;
            if (SketchGeometry::sketchEdgesEquivalent(TopoDS::Edge(sh), targetEdge)) continue;
            Standard_Real bf = 0.0, bl = 0.0;
            Handle(Geom_Curve) bCurve = BRep_Tool::Curve(TopoDS::Edge(sh), bf, bl);
            if (bCurve.IsNull()) continue;

            try {
                GeomAPI_ExtremaCurveCurve inter(targetCurve, bCurve);
                if (inter.NbExtrema() < 1) continue;
                for (int i = 1; i <= inter.NbExtrema(); ++i) {
                    gp_Pnt p1, p2;
                    inter.Points(i, p1, p2);
                    if (p1.Distance(p2) > 1.0e-6) continue;
                    const gp_Pnt ip((p1.X() + p2.X()) * 0.5, (p1.Y() + p2.Y()) * 0.5, (p1.Z() + p2.Z()) * 0.5);

                    gp_Pnt projectedB;
                    double tb = 0.0;
                    if (!SketchGeometry::projectPointOntoEdge(ip, TopoDS::Edge(sh), projectedB, &tb)) continue;
                    const double bLo = qMin(bf, bl);
                    const double bHi = qMax(bf, bl);
                    if (tb < bLo - 1.0e-7 || tb > bHi + 1.0e-7) continue;

                    gp_Pnt projectedT;
                    double tt = 0.0;
                    if (!SketchGeometry::projectPointOntoEdge(ip, targetEdge, projectedT, &tt)) continue;
                    if ((!extendStart && tt <= hi + 1.0e-7) || (extendStart && tt >= lo - 1.0e-7)) continue;

                    gp_Vec dirToHit(pRef, ip);
                    if (dirToHit.Magnitude() <= Precision::Confusion()) continue;
                    if (dirToHit.Dot(gp_Vec(tangent)) <= 0.0) continue;

                    const double d = pRef.Distance(ip);
                    if (d < bestDist) {
                        bestDist = d;
                        bestParam = tt;
                        found = true;
                    }
                }
            } catch (...) {
                continue;
            }
        }
        if (!found) return false;

        const double newLo = extendStart ? bestParam : lo;
        const double newHi = extendStart ? hi : bestParam;
        if (newHi - newLo <= 1.0e-7) return false;

        QList<TopoDS_Edge> repl;
        BRepBuilderAPI_MakeEdge mk(targetCurve, newLo, newHi);
        if (!mk.IsDone() || mk.Edge().IsNull()) return false;
        repl.append(mk.Edge());
        return replaceSketchEdgeWith(targetEdge, repl, tr("快速延伸"));
    } catch (...) {
        return false;
    }
}
