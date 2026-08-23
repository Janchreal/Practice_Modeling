// Qt eventFilter + VTK 鼠标移动悬停路由（从 widget.cpp 拆出）
#include "widget.h"
#include "widget_mirror_globals.h"
#include "sketchmodedialogs.h"
#include "sketchpolygondialog.h"
#include "sketchellipsedialog.h"

#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <QVTKOpenGLNativeWidget.h>

#include <limits>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

bool Widget::eventFilter(QObject *obj, QEvent *event)
{
    const auto activateContextByObject = [this, obj]() -> bool {
        if (obj == g_mainVtkWidgetMap.value(this, nullptr)) {
            activateMainRenderContext();
            return true;
        }
        if (g_mirrorRenderContextMap.contains(this)) {
            auto& map = g_mirrorRenderContextMap[this];
            for (auto it = map.begin(); it != map.end(); ++it) {
                if (it.value().vtkWidget == obj) {
                    activateMirrorRenderContext(it.key());
                    return true;
                }
            }
        }
        return false;
    };

    if (event->type() == QEvent::FocusIn ||
        event->type() == QEvent::Enter ||
        event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::Wheel) {
        activateContextByObject();
    }

    if (event->type() == QEvent::Leave) {
        if (obj == g_mainVtkWidgetMap.value(this, nullptr)) {
            clearModelHoverHighlight();
        } else if (g_mirrorRenderContextMap.contains(this)) {
            auto& map = g_mirrorRenderContextMap[this];
            for (auto it = map.begin(); it != map.end(); ++it) {
                if (it.value().vtkWidget == obj) {
                    clearModelHoverHighlight();
                    break;
                }
            }
        }
    }

    if (event->type() == QEvent::KeyPress && vtkWidget && obj == vtkWidget) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (currentSelectionMode == SketchDrawPolygon && sketchPolygonHasCenter_ && sketchPolygonValueDialog_) {
            if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down
                || ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
                sketchPolygonValueDialog_->focusFirstFieldFromArrowKey();
                return true;
            }
        }
        if (currentSelectionMode == SketchEllipseAdjust && sketchEllipseHasCenter_ && sketchEllipseAngleDialog_) {
            if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down
                || ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
                sketchEllipseAngleDialog_->focusAngleField();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}



// 处理鼠标移动（用于点选择的悬停提示）
void Widget::handleVtkMouseMove(int x, int y)
{
    // 悬停高亮同样依赖拾取绑定；在多窗口切换后先做一次上下文对齐。
    if (renderer && shapePicker) {
        shapePicker->SetRenderer(renderer);
    }
    prepareShapePickerBindingsForCurrentContext();

    if (currentSelectionMode == SketchPlaneSelection) {
        // 草图拾取平面：悬浮高亮
        updateSketchPlaneHover(x, y);
        return;
    }

    if (currentSelectionMode == SketchConicDragControl) {
        if (!hasActiveSketch_) return;
        handleSketchConicDragMouseMove(x, y);
        return;
    }

    if (currentSelectionMode == SketchEllipseAdjust) {
        if (!hasActiveSketch_) return;
        handleSketchEllipseAdjustMouseMove(x, y);
        refreshSnapHoverAfterSketchMouseMove(x, y);
        return;
    }

    if (currentSelectionMode == CuboidInteractive) {
        handleCuboidInteractiveMouseMove(x, y);
        return;
    }
    if (currentSelectionMode == PointSelection && cuboidDialog && cuboidInteractiveActive_) {
        handleCuboidInteractiveMouseMove(x, y);
        if (cuboidDragActive_) return;
    }

    if (currentSelectionMode == PatternPitchInteractive) {
        handlePatternPitchMouseMove(x, y);
        return;
    }

    if (currentSelectionMode == ExtrusionHandleDrag || extrusionDialog) {
        handleExtrusionHandleMouseMove(x, y);
        if (currentSelectionMode == ExtrusionHandleDrag) return;
    }
    if (currentSelectionMode == RevolveHandleDrag || revolveDialog) {
        handleRevolveHandleMouseMove(x, y);
        if (currentSelectionMode == RevolveHandleDrag) return;
    }

    if (currentSelectionMode == ChamferAsymHandleDrag || chamferDialog) {
        handleChamferAsymHandleMouseMove(x, y);
        if (currentSelectionMode == ChamferAsymHandleDrag) return;
    }
    if (currentSelectionMode == FilletRadiusHandleDrag || filletDialog) {
        handleFilletRadiusHandleMouseMove(x, y);
        if (currentSelectionMode == FilletRadiusHandleDrag) return;
    }

    // 草图绘制：实时预览
    if (currentSelectionMode == SketchDrawLine || currentSelectionMode == SketchDrawArc
        || currentSelectionMode == SketchDrawRectangle || currentSelectionMode == SketchDrawCircle
        || currentSelectionMode == SketchDrawPoint || currentSelectionMode == SketchDrawPolygon) {
        if (!hasActiveSketch_) return;

        struct SketchSnapHoverGuard {
            Widget* w;
            int mx, my;
            ~SketchSnapHoverGuard()
            {
                if (w)
                    w->refreshSnapHoverAfterSketchMouseMove(mx, my);
            }
        } snapHoverGuard{this, x, y};

        gp_Pnt p;
        if (!tryPickPointOnPlane(activeSketchPlane_, x, y, p)) return;

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

        sketchCommittedPointValid_ = false;
        sketchLastHoverPoint_ = p;
        sketchLastHoverValid_ = true;
        if (sketchToolInputDialog_) {
            updateSketchToolInputDialogFields(p);
        }

        if (currentSelectionMode == SketchDrawCircle) {
            const SketchCircleModeDialog::Method cm =
                sketchCircleModeDialog_ ? sketchCircleModeDialog_->method() : SketchCircleModeDialog::CenterRadius;
            if (cm == SketchCircleModeDialog::CenterRadius && sketchClickCount_ == 1) {
                const double R = gp_Vec(sketchP1_, p).Magnitude();
                updateSketchPreviewCircle(sketchP1_, R);
                clearSketchPreviewLine();
            } else if (cm == SketchCircleModeDialog::ThreePoint && sketchClickCount_ == 1) {
                // 与圆弧一致：第一点→第二点之间用直线预览
                updateSketchPreviewLine(sketchP1_, p);
                clearSketchPreviewCircle();
                clearSketchPreviewArc();
            } else if (cm == SketchCircleModeDialog::ThreePoint && sketchClickCount_ == 2) {
                clearSketchPreviewLine();
                if (sketchP1_.Distance(sketchP2_) > Precision::Confusion()) {
                    try {
                        updateSketchPreviewArc(sketchP1_, p, sketchP2_);
                    } catch (...) {
                    }
                } else {
                    clearSketchPreviewArc();
                }
            }
            clearSketchPreviewRectangle();
            if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            return;
        }

        if (currentSelectionMode == SketchDrawLine) {
            if (sketchClickCount_ == 1) {
                updateSketchPreviewLine(sketchP1_, p);
                clearSketchPreviewRectangle();
                clearSketchPreviewCircle();
                clearSketchPreviewArc();
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
            return;
        }

        if (currentSelectionMode == SketchDrawRectangle) {
            const SketchRectangleModeDialog::Method rm =
                sketchRectangleModeDialog_ ? sketchRectangleModeDialog_->method()
                                           : SketchRectangleModeDialog::TwoDiagonal;
            if (rm == SketchRectangleModeDialog::TwoDiagonal && sketchClickCount_ == 1) {
                updateSketchPreviewRectangle(sketchP1_, p);
                clearSketchPreviewLine();
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            } else if (rm == SketchRectangleModeDialog::ThreePoint) {
                if (sketchClickCount_ == 1) {
                    updateSketchPreviewLine(sketchP1_, p);
                    clearSketchPreviewRectangle();
                } else if (sketchClickCount_ == 2) {
                    const gp_Pnt& p1 = sketchP1_;
                    const gp_Pnt& p2 = sketchP2_;
                    gp_Vec u(p1, p2);
                    if (u.SquareMagnitude() > Precision::Confusion()) {
                        gp_Vec wraw(p2, p);
                        gp_Vec v = wraw - u * (wraw.Dot(u) / u.Dot(u));
                        const gp_Pnt p3 = p2.Translated(v);
                        const gp_Pnt p4 = p1.Translated(v);
                        updateSketchPreviewRectangleGeneral(p1, p2, p3, p4);
                    }
                    clearSketchPreviewLine();
                }
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            } else if (rm == SketchRectangleModeDialog::FromCenter) {
                const gp_Pln& pln = activeSketchPlane_;
                const gp_Dir n = pln.Axis().Direction();
                if (sketchClickCount_ == 1) {
                    const gp_Pnt C = sketchP1_;
                    gp_Vec ua(C, p);
                    const double hx = ua.Magnitude();
                    if (hx > Precision::Confusion()) {
                        gp_Dir e1(ua);
                        gp_Dir e2 = n.Crossed(e1);
                        const double hy = hx;
                        const gp_Pnt q1 = C.Translated(-hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
                        const gp_Pnt q2 = C.Translated(-hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
                        const gp_Pnt q3 = C.Translated(hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
                        const gp_Pnt q4 = C.Translated(hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
                        updateSketchPreviewRectangleGeneral(q1, q2, q3, q4);
                    }
                } else if (sketchClickCount_ == 2) {
                    const gp_Pnt C = sketchP1_;
                    const gp_Pnt pMid = sketchP2_;
                    gp_Vec ua(C, pMid);
                    const double hx = ua.Magnitude();
                    if (hx > Precision::Confusion()) {
                        gp_Dir e1(ua);
                        gp_Dir e2 = n.Crossed(e1);
                        const double hy = gp_Vec(C, p).Dot(gp_Vec(e2));
                        const gp_Pnt q1 = C.Translated(-hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
                        const gp_Pnt q2 = C.Translated(-hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
                        const gp_Pnt q3 = C.Translated(hx * gp_Vec(e1)).Translated(hy * gp_Vec(e2));
                        const gp_Pnt q4 = C.Translated(hx * gp_Vec(e1)).Translated(-hy * gp_Vec(e2));
                        updateSketchPreviewRectangleGeneral(q1, q2, q3, q4);
                    }
                }
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
            return;
        }

        if (currentSelectionMode == SketchDrawArc) {
            if (sketchClickCount_ == 1) {
                gp_Pnt pm;
                if (sketchContourChaining_ && sketchChainTangentValid_
                    && sketchArcMidFromTangentAndEnd(activeSketchPlane_, sketchP1_, sketchChainTangentDir_, p, pm)) {
                    try {
                        updateSketchPreviewArc(sketchP1_, pm, p);
                    } catch (...) {
                        clearSketchPreviewArc();
                        updateSketchPreviewLine(sketchP1_, p);
                    }
                } else {
                    clearSketchPreviewArc();
                    updateSketchPreviewLine(sketchP1_, p);
                }
            } else if (sketchClickCount_ == 2) {
                clearSketchPreviewLine();
                updateSketchPreviewArc(sketchP1_, p, sketchP2_);
            }
            clearSketchPreviewRectangle();
            clearSketchPreviewCircle();
            if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            return;
        }

        if (currentSelectionMode == SketchDrawPolygon) {
            if (sketchPolygonHasCenter_) {
                rebuildSketchPolygonPreviewFromHover(p);
                if (vtkWidget) {
                    const QPoint g = vtkWidget->mapToGlobal(QPoint(x, y));
                    positionSketchPolygonValueDialog(g.x(), g.y());
                }
            }
            return;
        }

        return;
    }

    if (isSketchEditMode(currentSelectionMode)) {
        handleSketchEditHover(x, y);
        if (sketchBrushActive_) {
            handleSketchEditClick(x, y, true);
        }
        return;
    }

    if (currentSelectionMode == PointSelection) {
        // 点选择模式下，如果捕捉点已开启，则使用捕捉点规则进行悬停提示
        if (snap_.armed) {
            updateSnapHover(x, y);
        } else {
            updatePointSelectionHover(x, y);
        }
    } else if (currentSelectionMode == VectorDialogPickStartPoint
               || currentSelectionMode == VectorDialogPickEndPoint) {
        // 两点定矢量：恢复点悬浮高亮（勿被 isVectorAxisPickContext 提前 return 吞掉）
        updateReferenceCsysAxisHover(x, y);
        const int kind = (currentSelectionMode == VectorDialogPickStartPoint)
                             ? vectorTwoPointStartSnapKind_
                             : vectorTwoPointEndSnapKind_;
        if (kind == -1) {
            updatePointSelectionHover(x, y);
        } else if (snap_.armed) {
            updateSnapHover(x, y);
        } else {
            clearSnapHover();
            clearPointSelectionHover();
        }
    } else if (isVectorAxisPickContext()) {
        updateReferenceCsysAxisHover(x, y);
        if (currentSelectionMode != VectorDialogPickDirection) {
            return;
        }
        // 矢量对话框拾取：悬停自动识别并预览箭头
        gp_Dir baseDir(0, 0, 1);
        int hoverModelIndex = -1;
        const IVtk_IdType invalidSubShapeId = static_cast<IVtk_IdType>(-1);
        IVtk_IdType hoverSubShapeId = invalidSubShapeId;
        if (tryComputeVectorDirUnderCursor(x, y, baseDir, &hoverModelIndex, &hoverSubShapeId)) {
            setCustomVectorDirFromDialog(baseDir);
            if (hasVectorDialogArrowOrigin_) {
                updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
            }

            // 曲线/轴矢量、面法向相关：悬停对象自动高亮，便于点击确认
            if (vectorDialogModeIndex_ == 3 || vectorDialogModeIndex_ == 4 ||
                vectorDialogModeIndex_ == 5 || vectorDialogModeIndex_ == 6) {
                const bool isCurveMode = (vectorDialogModeIndex_ == 3 || vectorDialogModeIndex_ == 4);
                const long long sid = static_cast<long long>(hoverSubShapeId);
                const bool hitValidSubShape = (hoverModelIndex >= 0 &&
                                               hoverSubShapeId != invalidSubShapeId &&
                                               sid > 0 &&
                                               sid <= static_cast<long long>(std::numeric_limits<int>::max()));
                if (hitValidSubShape) {
                    applyVectorDialogHoverSubShape(hoverModelIndex, hoverSubShapeId, !isCurveMode);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                } else {
                    clearVectorDialogHoverShape();
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                }
            } else {
                clearVectorDialogHoverShape();
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
        } else {
            clearVectorDialogHoverShape();
            if (vectorDialogArrowActor_) {
                vectorDialogArrowActor_->SetVisibility(false);
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
        }
    } else if (currentSelectionMode == ExtrusionSelection || 
               currentSelectionMode == EdgeSelection || 
               currentSelectionMode == FaceSelection) {
        // 拉伸选择模式下，处理面/边悬停高亮
        handleExtrusionFaceHover(x, y);
    } else if (currentSelectionMode == FilletEdgeSelection) {
        // 倒圆角：边悬停高亮
        handleFilletEdgeHover(x, y);
    } else if (currentSelectionMode == ChamferEdgeSelection) {
        // 倒角：边悬停高亮
        handleChamferEdgeHover(x, y);
    } else if (snap_.armed) {
        // 非选择模式：全局捕捉点悬停提示
        updateSnapHover(x, y);
        if (hasReferenceCsys_) updateReferenceCsysAxisHover(x, y);
    } else if (currentSelectionMode == None) {
        if (hasReferenceCsys_) updateReferenceCsysAxisHover(x, y);
        updateModelHoverHighlight(x, y);
    } else {
        clearModelHoverHighlight();
    }
}
