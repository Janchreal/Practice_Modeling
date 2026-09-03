// VTK 视图左键点击路由：草图/矢量/捕捉/工作坐标系/点选/倒角边/拉伸子形状/布尔/模型选中（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "sketch_create_dialog.h"
#include "sketch_mode_dialogs.h"
#include "vector_dialog.h"

#include <cmath>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <gce_MakeCirc.hxx>
#include <gp_Ax2.hxx>
#include <Precision.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>

#include <QSignalBlocker>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

void Widget::handleVtkMouseClick(int x, int y)
{
    // 每次拾取前都按当前窗口上下文强制刷新绑定，防止主/子窗口频繁切换后的绑定漂移。
    if (renderer && shapePicker) {
        shapePicker->SetRenderer(renderer);
    }
    prepareShapePickerBindingsForCurrentContext();

    if (handleMirrorTriadClick(x, y)) {
        return;
    }

    // 三重轴视口优先：不要用“稳健邻近拾取”挡掉立方体点击（否则视角切换会失效）
    if (handleCenterAxisPick(x, y)) {
        return;
    }

    // 后续模式用严格可见命中判断“是否点在模型上”（禁止邻近外壳）
    const bool preferModelBeforeTriad =
        (currentSelectionMode == None
         || currentSelectionMode == FeatureBooleanTargetSelect
         || currentSelectionMode == SelectTarget
         || currentSelectionMode == SelectTool
         || currentSelectionMode == PatternBodySelection);
    const int earlyModelIdx = preferModelBeforeTriad ? pickHistoryModelStrict(x, y) : -1;

    // 矢量拾取：点击基准坐标系 X/Y/Z 轴确认方向（优先于平面，避免误切视图）
    if (isVectorAxisPickContext() && handleReferenceCsysAxisPick(x, y)) {
        return;
    }

    // 基准坐标系三平面：点击后带动画切换到对应正交视图
    if (handleReferenceCsysPlanePick(x, y)) {
        return;
    }

    // 工作坐标系三轴：仅在矢量拾取上下文优先拦截；普通单击让模型优先，避免轴容差挡住模型高亮
    if (isVectorAxisPickContext() && handleWorkCsysAxisPick(x, y)) {
        return;
    }

    if (currentSelectionMode == SketchConicDragControl) {
        handleSketchConicDragMouseDown(x, y);
        return;
    }

    if (currentSelectionMode == SketchEllipseAdjust) {
        return;
    }

    // 草图：拾取参考平面
    if (currentSelectionMode == SketchPlaneSelection) {
        gp_Pln pln;
        TopoDS_Face face;
        if (!tryPickPlanarFaceUnderCursor(x, y, pln, face)) {
            statusBar()->showMessage(tr("创建草图：未拾取到平面，请点击一个平面面片。"), 3000);
            return;
        }
        if (activeSketchCreateDialog_) {
            activeSketchCreateDialog_->setPickedPlane(pln);
        }
        // 选中后：创建/更新一个半透明基准平面（略大于所拾取平面），并带不透明实线轮廓
        createOrUpdateSketchSelectedDatumPlane(pln, face);
        clearSketchPlaneHover();
        currentSelectionMode = None;
        statusBar()->showMessage(tr("已拾取参考平面。"), 2000);
        return;
    }

    if (currentSelectionMode == SketchPolygonPick) {
        if (!hasActiveSketch_ || !sketchPolygonDialog_) {
            currentSelectionMode = None;
            return;
        }
        gp_Pnt p;
        if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            statusBar()->showMessage(tr("草图：无法在平面上取点（可能视线与平面近平行）。"), 2500);
            return;
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        if (sketchCommittedPointValid_) {
            p = sketchCommittedPoint_;
            sketchCommittedPointValid_ = false;
        } else if (snap_.armed) {
            pickSnapAt(x, y, SnapPickContext::SketchTool);
            if (hasSnapSelectedPoint_) {
                p = snapSelectedPoint_;
                clearSnapSelected();
            }
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        completeSketchPolygonPick(p);
        return;
    }

    if (currentSelectionMode == SketchEllipsePick) {
        if (!hasActiveSketch_ || !sketchEllipseDialog_) {
            currentSelectionMode = None;
            return;
        }
        gp_Pnt p;
        if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            statusBar()->showMessage(tr("草图：无法在平面上取点（可能视线与平面近平行）。"), 2500);
            return;
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        if (sketchCommittedPointValid_) {
            p = sketchCommittedPoint_;
            sketchCommittedPointValid_ = false;
        } else if (snap_.armed) {
            pickSnapAt(x, y, SnapPickContext::SketchTool);
            if (hasSnapSelectedPoint_) {
                p = snapSelectedPoint_;
                clearSnapSelected();
            }
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        completeSketchEllipsePick(p);
        return;
    }

    if (currentSelectionMode == SketchConicPick) {
        if (!hasActiveSketch_ || !sketchConicDialog_) {
            currentSelectionMode = None;
            return;
        }
        gp_Pnt p;
        if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            statusBar()->showMessage(tr("草图：无法在平面上取点（可能视线与平面近平行）。"), 2500);
            return;
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        if (sketchCommittedPointValid_) {
            p = sketchCommittedPoint_;
            sketchCommittedPointValid_ = false;
        } else if (snap_.armed) {
            pickSnapAt(x, y, SnapPickContext::SketchTool);
            if (hasSnapSelectedPoint_) {
                p = snapSelectedPoint_;
                clearSnapSelected();
            }
        }
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }
        completeSketchConicPick(p);
        return;
    }

    // 草图：在当前草图平面内绘制
    if (currentSelectionMode == SketchDrawLine || currentSelectionMode == SketchDrawArc
        || currentSelectionMode == SketchDrawRectangle || currentSelectionMode == SketchDrawCircle
        || currentSelectionMode == SketchDrawPoint || currentSelectionMode == SketchDrawPolygon) {
        if (!hasActiveSketch_) {
            currentSelectionMode = None;
            return;
        }

        gp_Pnt p;
        if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) {
            statusBar()->showMessage(tr("草图：无法在平面上取点（可能视线与平面近平行）。"), 2500);
            return;
        }

        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();

            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }

        if (sketchCommittedPointValid_) {
            p = sketchCommittedPoint_;
            sketchCommittedPointValid_ = false;
        } else if (snap_.armed) {
            pickSnapAt(x, y, SnapPickContext::SketchTool);
            if (hasSnapSelectedPoint_) {
                p = snapSelectedPoint_;
                clearSnapSelected();
            }
        }

        // 捕捉点可能落在模型非草图平面处，再次投影到草图平面
        {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Pnt o = ax.Location();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            gp_Vec op(o, p);
            const double u = op.Dot(gp_Vec(xd));
            const double v = op.Dot(gp_Vec(yd));
            p = gp_Pnt(o.X() + xd.X() * u + yd.X() * v,
                      o.Y() + xd.Y() * u + yd.Y() * v,
                      o.Z() + xd.Z() * u + yd.Z() * v);
        }

        if (currentSelectionMode == SketchDrawPoint) {
            const gp_Ax3 ax = activeSketchPlane_.Position();
            const gp_Dir xd = ax.XDirection();
            const gp_Dir yd = ax.YDirection();
            // 十字半臂长（mm），与草图/模型长度单位一致（通常为 mm）
            constexpr double kSketchPointCrossHalfMm = 1.0;
            const double L = kSketchPointCrossHalfMm;
            gp_Vec ux(xd);
            gp_Vec uy(yd);
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p.Translated(-L * ux), p.Translated(L * ux)).Edge());
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p.Translated(-L * uy), p.Translated(L * uy)).Edge());
            ensureSketchHistoryRecord();
            updateSketchHistoryShape();
            markDocumentModified(true);
            sketchClickCount_ = 0;
            statusBar()->showMessage(tr("草图点已创建。"), 2000);
            return;
        }

        if (currentSelectionMode == SketchDrawPolygon) {
            if (!sketchPolygonHasCenter_) {
                statusBar()->showMessage(tr("多边形：请先指定中心点。"), 2500);
                return;
            }
            if (commitSketchPolygonFromParams(sketchPolygonCenter_, p)) {
                statusBar()->showMessage(tr("多边形已创建。"), 2500);
            } else {
                statusBar()->showMessage(tr("多边形：尺寸无效，无法创建。"), 3000);
            }
            if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            return;
        }

        if (currentSelectionMode == SketchDrawCircle) {
            const SketchCircleModeDialog::Method cm =
                sketchCircleModeDialog_ ? sketchCircleModeDialog_->method() : SketchCircleModeDialog::CenterRadius;
            if (cm == SketchCircleModeDialog::CenterRadius) {
                if (sketchClickCount_ == 0) {
                    sketchP1_ = p;
                    sketchClickCount_ = 1;
                    statusBar()->showMessage(tr("圆：请在平面内点击确定半径（圆上一点）。"), 2500);
                    return;
                }
                const gp_Pnt C = sketchP1_;
                const double R = gp_Vec(C, p).Magnitude();
                if (R <= Precision::Confusion()) {
                    statusBar()->showMessage(tr("圆：半径过小。"), 2500);
                    sketchClickCount_ = 0;
                    clearSketchPreviewCircle();
                    return;
                }
                try {
                    const gp_Ax2 ax2(C, activeSketchPlane_.Axis().Direction());
                    Handle(Geom_Circle) gc = new Geom_Circle(ax2, R);
                    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(gc).Edge();
                    activeSketch_.addGeometry(edge);
                    ensureSketchHistoryRecord();
                    updateSketchHistoryShape();
                    markDocumentModified(true);
                } catch (...) {
                    statusBar()->showMessage(tr("圆创建失败。"), 3000);
                }
                clearSketchPreviewCircle();
                sketchClickCount_ = 0;
                statusBar()->showMessage(tr("圆已创建。下一次请重新选择圆心。"), 2500);
                return;
            }
            // 三点定圆（仅三点模式；避免与其它工具共用 clickCount 时误走圆逻辑）
            if (cm != SketchCircleModeDialog::ThreePoint) {
                sketchClickCount_ = 0;
                statusBar()->showMessage(tr("圆：请在模式面板中选择“圆心”或“三点”。"), 3000);
                return;
            }
            if (sketchClickCount_ == 0) {
                sketchP1_ = p;
                sketchClickCount_ = 1;
                statusBar()->showMessage(tr("圆：请选择第二点。"), 2000);
                return;
            }
            if (sketchClickCount_ == 1) {
                if (gp_Vec(sketchP1_, p).Magnitude() <= Precision::Confusion()) {
                    statusBar()->showMessage(tr("圆：第二点与第一点过近，请重新选择。"), 2500);
                    return;
                }
                sketchP2_ = p;
                sketchClickCount_ = 2;
                statusBar()->showMessage(tr("圆：请选择第三点。"), 2000);
                return;
            }
            const gp_Pnt p3 = p;
            try {
                gce_MakeCirc mk(sketchP1_, sketchP2_, p3);
                if (!mk.IsDone()) {
                    statusBar()->showMessage(tr("三点定圆失败（可能共线）。"), 3000);
                    sketchClickCount_ = 0;
                    clearSketchPreviewCircle();
                    clearSketchPreviewLine();
                    clearSketchPreviewArc();
                    return;
                }
                const gp_Circ circ = mk.Value();
                Handle(Geom_Circle) gc = new Geom_Circle(circ);
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(gc).Edge();
                activeSketch_.addGeometry(edge);
                ensureSketchHistoryRecord();
                updateSketchHistoryShape();
                markDocumentModified(true);
            } catch (...) {
                statusBar()->showMessage(tr("三点定圆失败。"), 3000);
            }
            clearSketchPreviewCircle();
            clearSketchPreviewLine();
            clearSketchPreviewArc();
            sketchClickCount_ = 0;
            return;
        }

        if (currentSelectionMode == SketchDrawLine) {
            if (sketchClickCount_ == 0) {
                sketchP1_ = p;
                sketchClickCount_ = 1;
                statusBar()->showMessage(tr("直线：请选择终点。"), 2000);
                return;
            }

            const gp_Pnt a = sketchP1_;
            gp_Pnt p2 = p;
            if (gp_Vec(a, p2).Magnitude() <= Precision::Confusion()) {
                statusBar()->showMessage(tr("直线：起点与终点过近，请重新选择。"), 2500);
                sketchClickCount_ = 0;
                return;
            }

            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(a, p2).Edge();
            activeSketch_.addGeometry(edge);
            ensureSketchHistoryRecord();
            updateSketchHistoryShape();
            clearSketchPreviewLine();
            markDocumentModified(true);

            gp_Vec seg(a, p2);
            if (seg.Magnitude() > Precision::Confusion()) {
                sketchChainTangentDir_ = gp_Dir(seg);
                sketchChainTangentValid_ = true;
            }
            if (sketchContourChaining_) {
                sketchP1_ = p2;
                sketchClickCount_ = 1;
                statusBar()->showMessage(tr("直线已创建。下一段起点为上一终点。"), 2000);
            } else {
                sketchClickCount_ = 0;
                sketchChainTangentValid_ = false;
                statusBar()->showMessage(tr("直线已创建。请点击下一条线段的起点。"), 2000);
            }
            return;
        }

        if (currentSelectionMode == SketchDrawRectangle) {
            const SketchRectangleModeDialog::Method rm =
                sketchRectangleModeDialog_ ? sketchRectangleModeDialog_->method()
                                           : SketchRectangleModeDialog::TwoDiagonal;

            if (rm == SketchRectangleModeDialog::TwoDiagonal) {
                if (sketchClickCount_ == 0) {
                    sketchP1_ = p;
                    sketchClickCount_ = 1;
                    statusBar()->showMessage(tr("长方体草图：请选择对角第二点。"), 2500);
                    return;
                }

                const gp_Ax3 ax = activeSketchPlane_.Position();
                const gp_Dir xd = ax.XDirection();
                const gp_Dir yd = ax.YDirection();

                const gp_Pnt p1 = sketchP1_;
                const gp_Pnt p3 = p;
                const gp_Vec diag(p1, p3);
                const double du = diag.Dot(gp_Vec(xd));
                const double dv = diag.Dot(gp_Vec(yd));
                if (std::abs(du) <= Precision::Confusion() || std::abs(dv) <= Precision::Confusion()) {
                    statusBar()->showMessage(tr("长方体草图：对角线退化，无法形成有效矩形，请重新选择。"), 3000);
                    sketchClickCount_ = 0;
                    clearSketchPreviewLine();
                    clearSketchPreviewRectangle();
                    return;
                }

                const gp_Pnt p2 = p1.Translated(du * gp_Vec(xd));
                const gp_Pnt p4 = p1.Translated(dv * gp_Vec(yd));

                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
                ensureSketchHistoryRecord();
                updateSketchHistoryShape();
                clearSketchPreviewLine();
                clearSketchPreviewRectangle();
                markDocumentModified(true);

                sketchClickCount_ = 0;
                statusBar()->showMessage(tr("长方体草图矩形已创建。下一次请重新点击起点。"), 2500);
                return;
            }

            if (rm == SketchRectangleModeDialog::ThreePoint) {
                if (sketchClickCount_ == 0) {
                    sketchP1_ = p;
                    sketchClickCount_ = 1;
                    statusBar()->showMessage(tr("矩形(三点)：请选择相邻角点（定义一边）。"), 2500);
                    return;
                }
                if (sketchClickCount_ == 1) {
                    sketchP2_ = p;
                    sketchClickCount_ = 2;
                    statusBar()->showMessage(tr("矩形(三点)：请选择第三点完成矩形。"), 2500);
                    return;
                }
                const gp_Pnt p1 = sketchP1_;
                const gp_Pnt p2 = sketchP2_;
                gp_Vec u(p1, p2);
                if (u.SquareMagnitude() <= Precision::Confusion()) {
                    statusBar()->showMessage(tr("矩形：边长过短。"), 2500);
                    sketchClickCount_ = 0;
                    clearSketchPreviewRectangle();
                    return;
                }
                gp_Vec wraw(p2, p);
                gp_Vec v = wraw - u * (wraw.Dot(u) / u.Dot(u));
                if (v.SquareMagnitude() <= Precision::Confusion()) {
                    statusBar()->showMessage(tr("矩形：第三点退化。"), 2500);
                    sketchClickCount_ = 0;
                    clearSketchPreviewRectangle();
                    return;
                }
                const gp_Pnt p3 = p2.Translated(v);
                const gp_Pnt p4 = p1.Translated(v);
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
                activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
                ensureSketchHistoryRecord();
                updateSketchHistoryShape();
                clearSketchPreviewRectangle();
                markDocumentModified(true);
                sketchClickCount_ = 0;
                statusBar()->showMessage(tr("矩形(三点)已创建。"), 2500);
                return;
            }

            // FromCenter
            if (sketchClickCount_ == 0) {
                sketchP1_ = p;
                sketchClickCount_ = 1;
                statusBar()->showMessage(tr("矩形(中心)：请选择一侧边中点。"), 2500);
                return;
            }
            if (sketchClickCount_ == 1) {
                sketchP2_ = p;
                sketchClickCount_ = 2;
                statusBar()->showMessage(tr("矩形(中心)：请选择角点大致位置。"), 2500);
                return;
            }
            const gp_Pnt C = sketchP1_;
            const gp_Pnt pMid = sketchP2_;
            gp_Vec ua(C, pMid);
            const double hx = ua.Magnitude();
            if (hx <= Precision::Confusion()) {
                statusBar()->showMessage(tr("矩形：中心到边中点过近。"), 2500);
                sketchClickCount_ = 0;
                clearSketchPreviewRectangle();
                return;
            }
            gp_Dir e1(ua);
            gp_Dir n = activeSketchPlane_.Axis().Direction();
            gp_Dir e2 = n.Crossed(e1);
            const double hy = gp_Vec(C, p).Dot(gp_Vec(e2));
            if (std::abs(hy) <= Precision::Confusion()) {
                statusBar()->showMessage(tr("矩形：高度退化。"), 2500);
                sketchClickCount_ = 0;
                clearSketchPreviewRectangle();
                return;
            }
            const gp_Pnt p1 = C.Translated(-hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
            const gp_Pnt p2 = C.Translated(-hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
            const gp_Pnt p3 = C.Translated(hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
            const gp_Pnt p4 = C.Translated(hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
            activeSketch_.addGeometry(BRepBuilderAPI_MakeEdge(p4, p1).Edge());
            ensureSketchHistoryRecord();
            updateSketchHistoryShape();
            clearSketchPreviewRectangle();
            markDocumentModified(true);
            sketchClickCount_ = 0;
            statusBar()->showMessage(tr("矩形(中心)已创建。"), 2500);
            return;
        }

        // 圆弧：必须判断模式。否则三点定圆在 sketchClickCount_==2 时会误入此处并触发
        // GC_MakeArcOfCircle 的 StdFail_NotDone，表现为第二次点击崩溃。
        if (currentSelectionMode == SketchDrawArc) {
            if (sketchClickCount_ == 0) {
                sketchP1_ = p;
                sketchClickCount_ = 1;
                statusBar()->showMessage(tr("圆弧：请选择终点。"), 2000);
                return;
            }
            if (sketchClickCount_ == 1) {
                gp_Pnt pm;
                if (sketchContourChaining_ && sketchChainTangentValid_
                    && sketchArcMidFromTangentAndEnd(activeSketchPlane_, sketchP1_, sketchChainTangentDir_, p, pm)) {
                    try {
                        GC_MakeArcOfCircle mkArc(sketchP1_, pm, p);
                        if (!mkArc.IsDone()) {
                            statusBar()->showMessage(tr("相切圆弧创建失败。"), 3000);
                        } else {
                            Handle(Geom_TrimmedCurve) arc = mkArc.Value();
                            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc).Edge();
                            activeSketch_.addGeometry(edge);
                            ensureSketchHistoryRecord();
                            updateSketchHistoryShape();
                            markDocumentModified(true);
                            try {
                                gp_Vec dv = arc->DN(arc->LastParameter(), 1);
                                if (dv.Magnitude() > Precision::Confusion()) {
                                    sketchChainTangentDir_ = gp_Dir(dv);
                                    sketchChainTangentValid_ = true;
                                }
                            } catch (...) {
                            }
                            sketchP1_ = p;
                            sketchClickCount_ = 1;
                            clearSketchPreviewArc();
                            clearSketchPreviewLine();
                            statusBar()->showMessage(tr("相切圆弧已创建。下一段起点为终点。"), 2500);
                            return;
                        }
                    } catch (...) {
                        statusBar()->showMessage(tr("相切圆弧创建失败。"), 3000);
                    }
                }
                sketchP2_ = p;
                sketchClickCount_ = 2;
                statusBar()->showMessage(tr("圆弧：请移动鼠标调整半径，点击左键确认圆弧。"), 3000);
                clearSketchPreviewLine();
                return;
            }

            gp_Pnt radiusCtrl = p;
            try {
                GC_MakeArcOfCircle mkArc(sketchP1_, radiusCtrl, sketchP2_);
                if (!mkArc.IsDone()) {
                    statusBar()->showMessage(tr("圆弧创建失败：请确保三点不共线且不重合。"), 3000);
                } else {
                    Handle(Geom_TrimmedCurve) arc = mkArc.Value();
                    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc).Edge();
                    activeSketch_.addGeometry(edge);
                    ensureSketchHistoryRecord();
                    updateSketchHistoryShape();
                    markDocumentModified(true);
                    try {
                        gp_Vec dv = arc->DN(arc->LastParameter(), 1);
                        if (dv.Magnitude() > Precision::Confusion()) {
                            sketchChainTangentDir_ = gp_Dir(dv);
                            sketchChainTangentValid_ = true;
                        }
                    } catch (...) {
                    }
                    if (sketchContourChaining_) {
                        sketchP1_ = sketchP2_;
                        sketchClickCount_ = 1;
                        statusBar()->showMessage(tr("圆弧已创建。下一段起点为上一终点。"), 2000);
                    } else {
                        sketchClickCount_ = 0;
                        sketchChainTangentValid_ = false;
                        statusBar()->showMessage(tr("圆弧已创建。请重新点击选择新圆弧的起点。"), 2500);
                    }
                }
            } catch (...) {
                statusBar()->showMessage(tr("圆弧创建失败：请确保三点不共线且不重合。"), 3000);
            }
            clearSketchPreviewArc();

            return;
        }

        return;
    }

    if (isSketchEditMode(currentSelectionMode)) {
        handleSketchEditClick(x, y, false);
        return;
    }

    // 捕捉点：开启时只做点捕捉，绝不整模高亮；模型改用幽灵外观
    if (snap_.armed && (currentSelectionMode == None || currentSelectionMode == PointSelection)) {
        pickSnapAt(x, y);
        if (currentSelectionMode == None) {
            // 无论是否捕到点，都不要走后面的整模选中/取消逻辑
            return;
        }
        // PointSelection：捕到点后仍由后续指定点流程处理
        if (currentSelectionMode == PointSelection && hasSnapSelectedPoint_) {
            // 继续往下走 PointSelection 分支
        }
    }

    // 普通左键：严格拾取。点在模型上→高亮；点空白→取消。不用邻近吸附（那是右键稳健拾取用的）。
    // 捕捉开启时上面已 return，不会进入这里。
    if (currentSelectionMode == None) {
        const int strictIdx = pickHistoryModelStrict(x, y);
        if (strictIdx >= 0 && strictIdx < historyList.size() && historyList[strictIdx].actor) {
            currentSelectedIndex = strictIdx;
            highlightModel(strictIdx);
            updateHistoryListSelection();
            clearSubShapeHighlight();
        } else {
            currentSelectedIndex = -1;
            highlightModel(-1);
            updateHistoryListSelection();
            clearSubShapeHighlight();
        }
        return;
    }

    if (!shapePicker) {
        if (earlyModelIdx >= 0 && earlyModelIdx < historyList.size() && historyList[earlyModelIdx].actor) {
            currentSelectedIndex = earlyModelIdx;
            highlightModel(earlyModelIdx);
            updateHistoryListSelection();
        }
        return;
    }

    // 将屏幕坐标转换为世界坐标
    // GetEventPosition 已经是 VTK 显示坐标（原点左下，含 DPR），禁止再做 height-y 翻转，
    // 否则会出现“点上方空白却命中模型下部 / 点下部不中”的纵向偏移。
    if (renderer) {
        renderer->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 0.0);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(lastWorldPoint);
    }

    // 如果处于“工作坐标系放置”模式，直接在点击位置创建/移动工作坐标系
    if (currentSelectionMode == WorkCsysPlacement) {
        // 使用与屏幕点击更一致的方式计算放置点：
        // 将当前 VTK 屏幕坐标射线投影到全局 Z=0 平面上
        gp_Pnt p(0.0, 0.0, 0.0);

        if (renderer && vtkWidget) {
            double displayX = static_cast<double>(x);
            double displayY = static_cast<double>(y);

            double worldNear[4] = {0, 0, 0, 1};
            double worldFar[4]  = {0, 0, 1, 1};

            renderer->SetDisplayPoint(displayX, displayY, 0.0);
            renderer->DisplayToWorld();
            renderer->GetWorldPoint(worldNear);

            renderer->SetDisplayPoint(displayX, displayY, 1.0);
            renderer->DisplayToWorld();
            renderer->GetWorldPoint(worldFar);

            if (std::abs(worldNear[3]) > 1e-10) {
                worldNear[0] /= worldNear[3];
                worldNear[1] /= worldNear[3];
                worldNear[2] /= worldNear[3];
            }
            if (std::abs(worldFar[3]) > 1e-10) {
                worldFar[0] /= worldFar[3];
                worldFar[1] /= worldFar[3];
                worldFar[2] /= worldFar[3];
            }

            double dir[3] = {
                worldFar[0] - worldNear[0],
                worldFar[1] - worldNear[1],
                worldFar[2] - worldNear[2]
            };

            double len = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
            if (len > 1e-10) {
                dir[0] /= len;
                dir[1] /= len;
                dir[2] /= len;
            }

            // 与 Z=0 平面求交：worldNear.z + t * dir.z = 0
            if (std::abs(dir[2]) > 1e-10) {
                double t = -worldNear[2] / dir[2];
                double px = worldNear[0] + t * dir[0];
                double py = worldNear[1] + t * dir[1];
                double pz = 0.0;
                p = gp_Pnt(px, py, pz);
            } else {
                // 射线几乎平行于 Z=0，退化情况下直接使用近裁剪点
                p = gp_Pnt(worldNear[0], worldNear[1], worldNear[2]);
            }
        } else {
            // 兜底：退回到上一次点击的世界坐标
            p = gp_Pnt(lastWorldPoint[0], lastWorldPoint[1], lastWorldPoint[2]);
        }

        if (!ensureWorkCsysActorsCreated()) {
            currentSelectionMode = None;
            return;
        }
        const bool needCreate = (workCsysHistoryIndex_ < 0);

        workCsysTransform->Identity();
        workCsysTransform->Translate(p.X(), p.Y(), p.Z());

        hasWorkCsys = true;
        workCsysDragActive = false;
        setWorkCsysVisible(true);
        applyAxisDirectionHighlight(currentAxisDirection);
        // 放置完成后恢复正常视图交互，暂不启用拖拽模式
        currentSelectionMode = None;

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }

        // 写入/更新建模历史（用于特征树显示、可隐藏、可删除）
        if (needCreate) {
            addToHistory(WORK_CSYS, tr("工作坐标系"),
                         workCsysActor, QColor(255, 230, 80),
                         p.X(), p.Y(), p.Z(),
                         nullptr, TopoDS_Shape(), nullptr, nullptr);
            workCsysHistoryIndex_ = historyList.size() - 1;
        } else if (workCsysHistoryIndex_ >= 0 && workCsysHistoryIndex_ < historyList.size()
                   && historyList[workCsysHistoryIndex_].type == WORK_CSYS) {
            historyList[workCsysHistoryIndex_].param1 = p.X();
            historyList[workCsysHistoryIndex_].param2 = p.Y();
            historyList[workCsysHistoryIndex_].param3 = p.Z();
        }
        return;
    }

    // 拾取前绑定当前上下文（主窗口或子窗口）的 actor。
    prepareShapePickerBindingsForCurrentContext();
    
    // 拾取前更新所有 ShapeDataSource，确保 shapePicker 能识别所有模型
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    // 更新 shapePicker 的 renderer 和容差
    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    
    // 根据选择模式设置拾取模式
    if (currentSelectionMode == PointSelection) {
        // 点选择模式：先尝试拾取面（用于识别圆形面的圆心），如果失败再尝试顶点，最后尝试边
        shapePicker->SetSelectionMode(SM_Face);
    } else {
        // 默认模式：拾取面
        shapePicker->SetSelectionMode(SM_Face);
    }

    // 执行拾取：默认先面，失败再边/顶点（与右键 pickModelAtPosition 一致，避免擦边/仰视底面点不中）
    shapePicker->Pick(x, y, 0);

    auto pickFirstVisibleActor = [&]() -> vtkActor* {
        vtkSmartPointer<vtkActorCollection> actors = shapePicker->GetPickedActors(true);
        if (!actors || actors->GetNumberOfItems() == 0) {
            return nullptr;
        }
        actors->InitTraversal();
        while (vtkActor* actor = actors->GetNextActor()) {
            if (actor->GetVisibility() != 0 && actor->GetPickable() != 0) {
                return actor;
            }
        }
        return nullptr;
    };

    if (!pickFirstVisibleActor()) {
        shapePicker->SetSelectionMode(SM_Edge);
        shapePicker->Pick(x, y, 0);
    }
    if (!pickFirstVisibleActor()) {
        shapePicker->SetSelectionMode(SM_Vertex);
        shapePicker->Pick(x, y, 0);
    }
    // 点选择模式额外再确保走过面→顶点→边（上面已覆盖边/顶点）
    if (currentSelectionMode == PointSelection && !pickFirstVisibleActor()) {
        shapePicker->SetSelectionMode(SM_Face);
        shapePicker->Pick(x, y, 0);
    }

    // 获取拾取到的Actors（取鼠标下所有命中，再过滤隐藏的，避免隐藏布尔结果“挡死”后面的模型）
    vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);

    vtkActor* selectedActor = nullptr;
    int selectedIndex = -1;

    if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
        pickedActors->InitTraversal();
        // 只接受 history 中的实体 actor（忽略轮廓/坐标系等可拾取叠加物）
        while (vtkActor* actor = pickedActors->GetNextActor()) {
            if (actor->GetVisibility() == 0 || actor->GetPickable() == 0) continue;
            const int idx = resolveHistoryIndexByActor(actor);
            if (idx >= 0) {
                selectedActor = actor;
                selectedIndex = idx;
                break;
            }
        }
    }

    // ShapePicker 在底角/擦边常失效；点拾取器能中是因为走屏幕邻近拓扑。
    // 优先使用开头已算好的稳健结果，再补一次回退。
    if (selectedIndex < 0
        && currentSelectionMode != VectorDialogPickDirection
        && currentSelectionMode != VectorDialogPickStartPoint
        && currentSelectionMode != VectorDialogPickEndPoint) {
        int fb = earlyModelIdx;
        if (fb < 0) fb = pickHistoryModelFallback(x, y);
        if (fb >= 0 && fb < historyList.size() && historyList[fb].actor) {
            selectedIndex = fb;
            selectedActor = historyList[fb].actor;
        }
    }

    // 优先处理矢量对话框拾取（必须在点选择之前）
    if (currentSelectionMode == VectorDialogPickDirection) {
        gp_Dir baseDir(0, 0, 1);
        if (tryComputeVectorDirUnderCursor(x, y, baseDir, nullptr, nullptr)) {
            setCustomVectorDirFromDialog(baseDir);
            if (patternVectorPick_ != PatternVectorPick::None) {
                applyPatternVectorPick(customVectorDir_);
                clearVectorDialogHoverShape();
                return;
            }
            if (hasVectorDialogArrowOrigin_) {
                updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
            }
            if (vectorDialog_) {
                vectorDialog_->setVectorDirDisplay(customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
                if (vectorDialogModeIndex_ == 4 && hasVectorDialogCurveEdge_) {
                    vectorDialog_->setCurvePicked(true);
                    vectorDialog_->setCurveTotalLength(vectorDialogCurveTotalLen_);
                }
            }
            clearVectorDialogHoverShape();
            currentSelectionMode = None;
        }
        return;
    }

    if (currentSelectionMode == PatternBodySelection && selectedActor) {
        handlePatternBodySelection(selectedActor);
        return;
    }

    if (currentSelectionMode == VectorDialogPickStartPoint) {
        if (vectorTwoPointStartSnapKind_ == -1) {
            gp_Pnt p;
            if (!tryPickPointOnModelForVector(x, y, p)) return;
            onVectorTwoPointStartPicked(p);
            return;
        }

        if (!snap_.armed) return;
        if (!tryPickVectorTwoPointSnapAt(x, y)) return;
        onVectorTwoPointStartPicked(snapSelectedPoint_);
        return;
    }

    if (currentSelectionMode == VectorDialogPickEndPoint) {
        if (!hasVectorStartPoint_) return;
        if (vectorTwoPointEndSnapKind_ == -1) {
            gp_Pnt p;
            if (!tryPickPointOnModelForVector(x, y, p)) return;
            onVectorTwoPointEndPicked(p);
            return;
        }

        if (!snap_.armed) return;
        if (!tryPickVectorTwoPointSnapAt(x, y)) return;
        onVectorTwoPointEndPicked(snapSelectedPoint_);
        return;
    }

    // 优先检查点选择模式（必须在其他选择模式之前）
    if (currentSelectionMode == PointSelection && patternDialog_) {
        if (selectedActor) {
            handlePointSelection(selectedActor, x, y);
            return;
        }
        gp_Pnt picked;
        if (tryPickPointOnModelForVector(x, y, picked)) {
            patternDialog_->setRotationCenter(picked, true);
            selectedOriginPoint = picked;
            hasSelectedOriginPoint = true;
            const QString label = tr("面点\n(%1, %2, %3)")
                                      .arg(picked.X(), 0, 'f', 2)
                                      .arg(picked.Y(), 0, 'f', 2)
                                      .arg(picked.Z(), 0, 'f', 2);
            showSelectedPoint(picked, label);
            clearPointSelectionHover();
            if (patternDialog_->hasDirection1()) {
                updatePatternRotationAxisArrow();
            }
            restorePatternPitchInteractiveAfterOriginPick();
            return;
        }
    } else if (currentSelectionMode == PointSelection && selectedActor) {
        handlePointSelection(selectedActor, x, y);
        return;
    }

    // 结果预览锁定期间：禁止继续选截面/布尔目标
    if (featureResultPreviewActive_
        && (currentSelectionMode == FeatureBooleanTargetSelect
            || currentSelectionMode == ExtrusionSelection
            || currentSelectionMode == EdgeSelection
            || currentSelectionMode == FaceSelection)) {
        return;
    }

    // 拉伸/旋转：选择布尔目标体
    if (currentSelectionMode == FeatureBooleanTargetSelect && selectedIndex >= 0) {
        if (extrusionDialog) {
            extrusionDialog->setBooleanTargetIndex(selectedIndex, historyList[selectedIndex].name);
        }
        if (revolveDialog) {
            revolveDialog->setBooleanTargetIndex(selectedIndex, historyList[selectedIndex].name);
        }
        // 选完目标体后恢复剖面选择模式，避免后续无法继续选边/面
        if (extrusionDialog) {
            const QString mode = extrusionDialog->getSelectionMode();
            if (mode == QLatin1String("edge")) {
                currentSelectionMode = EdgeSelection;
            } else if (mode == QLatin1String("face")) {
                currentSelectionMode = FaceSelection;
            } else {
                currentSelectionMode = ExtrusionSelection;
            }
        } else if (revolveDialog) {
            const QString mode = revolveDialog->getSelectionMode();
            if (mode == QLatin1String("edge")) {
                currentSelectionMode = EdgeSelection;
            } else if (mode == QLatin1String("face")) {
                currentSelectionMode = FaceSelection;
            } else {
                currentSelectionMode = ExtrusionSelection;
            }
        } else {
            currentSelectionMode = None;
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("已选择布尔目标体: %1").arg(historyList[selectedIndex].name), 2500);
        }
        if (extrusionDialog) {
            updateExtrusionHandles();
            refreshExtrusionLivePreview();
        }
        if (revolveDialog) {
            updateRevolveHandles();
            refreshRevolveLivePreview();
        }
        return;
    }

    // 倒圆角：边选择
    if (currentSelectionMode == FilletEdgeSelection) {
        handleFilletEdgeClick(x, y);
        return;
    }
    // 倒角：边选择
    if (currentSelectionMode == ChamferEdgeSelection) {
        handleChamferEdgeClick(x, y);
        return;
    }

    // 拉伸/旋转：子几何选择（边/面）
    if (currentSelectionMode == ExtrusionSelection ||
        currentSelectionMode == EdgeSelection ||
        currentSelectionMode == FaceSelection) {
        handleExtrusionFaceClick(x, y);
        return;
    }

    // 检查是否处于布尔运算选择模式（注意：这个检查必须在点选择和拉伸选择之后）
    if (currentSelectionMode != None && selectedActor) {
        handleBooleanSelection(selectedActor);
        return;
    }

    // 正常模式：先记录拾取到的子形状 ID（与 vis_picker_example：后续对 SubPolyDataFilter::SetData）
    IVtk_ShapeIdList pickedSubShapeIds;
    if (selectedIndex >= 0 && selectedActor) {
        if (IVtkTools_ShapeDataSource* dataSource =
                IVtkTools_ShapeObject::GetShapeSource(selectedActor)) {
            Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
            if (!shapeWrapper.IsNull()) {
                pickedSubShapeIds = shapePicker->GetPickedSubShapesIds(shapeWrapper->GetId());
            }
        }
    }

    // 先整体高亮选中模型（会暂时隐藏所有子形状高亮 Actor），再叠加 VIS 子形状高亮
    bool found = false;
    if (selectedActor) {
        const int idx = resolveHistoryIndexByActor(selectedActor);
        if (idx >= 0 && idx < historyList.size()) {
            currentSelectedIndex = idx;
            highlightModel(idx);
            updateHistoryListSelection();
            found = true;
        }
    }

    if (!found) {
        // 仅矢量拾取上下文才让工作坐标系轴吞点击；普通模式下点空白必须能取消高亮
        if (isVectorAxisPickContext()
            && pickHistoryModelFallback(x, y) < 0
            && handleWorkCsysAxisPick(x, y)) {
            return;
        }
        currentSelectedIndex = -1;
        highlightModel(-1);
        updateHistoryListSelection();
    }

    if (selectedIndex >= 0) {
        highlightSubShapes(selectedIndex, pickedSubShapeIds);
    } else {
        clearSubShapeHighlight();
    }
}

