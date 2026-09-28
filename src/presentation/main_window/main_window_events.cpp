// Qt eventFilter + VTK 鼠标移动悬停路由（从 main_window.cpp 拆出）
#include "main_window.h"
#include "viewport/mirror/mirror_view_state.h"
#include "ui_main_window.h"
#include "presentation/dialogs/sketch/sketch_mode_dialogs.h"
#include "sketch_polygon_dialog.h"
#include "sketch_ellipse_dialog.h"

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QObject>
#include <QStatusBar>
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
            clearReferenceCsysHover();
        } else if (g_mirrorRenderContextMap.contains(this)) {
            auto& map = g_mirrorRenderContextMap[this];
            for (auto it = map.begin(); it != map.end(); ++it) {
                if (it.value().vtkWidget == obj) {
                    clearModelHoverHighlight();
                    clearReferenceCsysHover();
                    break;
                }
            }
        }
    }

    if (event->type() == QEvent::KeyPress && vtkWidget && obj == vtkWidget) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape && sketchContourChaining_
            && (currentSelectionMode == SketchDrawLine || currentSelectionMode == SketchDrawArc)) {
            sketchClickCount_ = 0;
            sketchChainTangentValid_ = false;
            sketchCommittedPointValid_ = false;
            clearSketchPreviewLine();
            clearSketchPreviewArc();
            statusBar()->showMessage(tr("轮廓：已中断本次连锁，请重新点击下一段起点。"), 2500);
            if (vtkWidget && vtkWidget->renderWindow()) {
                vtkWidget->renderWindow()->Render();
            }
            return true;
        }
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
    refreshShapePickerBindingsForCurrentContext(0.05, false);

    if (currentSelectionMode == SketchPlaneSelection) {
        // 草图拾取平面：悬浮高亮
        if (!sketchPlaneHoverArmed_) {
            // 模式切换时可能收到一次旧位置/焦点导致的鼠标移动事件。
            // 只忽略进入模式前记录的那个坐标；坐标一变化就处理第一次真实移动。
            if (x == sketchPlaneHoverActivationX_ &&
                y == sketchPlaneHoverActivationY_) {
                return;
            }
            sketchPlaneHoverArmed_ = true;
        }
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
    if (currentSelectionMode == VectorTwoPointHandleDrag || currentSelectionMode == VectorTwoPointInteractive) {
        handleVectorTwoPointHandleMouseMove(x, y);
        if (currentSelectionMode == VectorTwoPointHandleDrag) return;
    }

    // 草图绘制：实时预览
    if (currentSelectionMode == SketchDrawLine || currentSelectionMode == SketchDrawArc
        || currentSelectionMode == SketchDrawRectangle || currentSelectionMode == SketchDrawCircle
        || currentSelectionMode == SketchDrawPoint || currentSelectionMode == SketchDrawPolygon) {
        if (!hasActiveSketch_) return;

        gp_Pnt p;
        if (!tryResolveSketchHoverPoint(x, y, p)) return;

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
            } else if (cm == SketchCircleModeDialog::TwoPointRadius && sketchClickCount_ == 1) {
                updateSketchPreviewLine(sketchP1_, p);
                clearSketchPreviewCircle();
                clearSketchPreviewArc();
            } else if (cm == SketchCircleModeDialog::TwoPointRadius && sketchClickCount_ == 2) {
                clearSketchPreviewLine();
                const double R = gp_Vec(sketchP1_, p).Magnitude();
                gp_Pnt center;
                if (sketchCircleCenterFromTwoPointsRadius(activeSketchPlane_, sketchP1_, sketchP2_, R, p, center)) {
                    updateSketchPreviewCircle(center, R);
                } else {
                    clearSketchPreviewCircle();
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
                    updateSketchPreviewLine(C, p);
                    clearSketchPreviewRectangle();
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
            const SketchArcModeDialog::Method arcMethod =
                sketchArcModeDialog_ ? sketchArcModeDialog_->method()
                                     : SketchArcModeDialog::ThreePoint;
            if (sketchClickCount_ == 1) {
                gp_Pnt pm;
                gp_Dir arcStartTangent = sketchChainTangentDir_;
                if (sketchContourChaining_ && sketchChainTangentValid_
                    && sketchContourArcStartTangentForPoint(activeSketchPlane_, sketchP1_,
                                                            sketchChainTangentDir_, p,
                                                            arcStartTangent)
                    && sketchArcMidFromTangentAndEnd(activeSketchPlane_, sketchP1_, arcStartTangent, p, pm)) {
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
                if (arcMethod == SketchArcModeDialog::CenterEndpoint && !sketchContourChaining_) {
                    gp_Pnt endPoint;
                    gp_Pnt midPoint;
                    if (sketchArcEndAndMidFromCenterStartHint(activeSketchPlane_, sketchP1_, sketchP2_, p,
                                                               endPoint, midPoint)) {
                        updateSketchPreviewArc(sketchP2_, midPoint, endPoint);
                    } else {
                        clearSketchPreviewArc();
                    }
                } else {
                    updateSketchPreviewArc(sketchP1_, p, sketchP2_);
                }
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

    if (currentSelectionMode == SketchPolygonPick
        || currentSelectionMode == SketchEllipsePick
        || currentSelectionMode == SketchConicPick) {
        gp_Pnt p;
        if (tryResolveSketchHoverPoint(x, y, p)) {
            sketchLastHoverPoint_ = p;
            sketchLastHoverValid_ = true;
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

    if (inSketchEnvironment_ && currentSelectionMode == None) {
        clearModelHoverHighlight();
        clearSubShapeHighlight();
        return;
    }

    if (currentSelectionMode == WorkCsysPlacement) {
        if (snap_.armed) {
            updateSnapHover(x, y);
        } else {
            clearSnapHover();
        }
        return;
    }

    if (currentSelectionMode == PointSelection) {
        if (hasReferenceCsys_) {
            updateReferenceCsysAxisHover(x, y);
            if (referenceCsysOriginHovered_) {
                clearPointSelectionHover();
                return;
            }
        }
        // 点选择模式下，如果捕捉点已开启，则使用捕捉点规则进行悬停提示
        if (snap_.armed) {
            updateSnapHover(x, y);
        } else {
            updatePointSelectionHover(x, y);
        }
    } else if (currentSelectionMode == VectorDialogPickStartPoint
               || currentSelectionMode == VectorDialogPickEndPoint) {
        updateReferenceCsysAxisHover(x, y);
        const int kind = (currentSelectionMode == VectorDialogPickStartPoint)
                             ? vectorTwoPointStartSnapKind_
                             : vectorTwoPointEndSnapKind_;
        clearPointSelectionHover();
        updateVectorTwoPointSnapPresentation(x, y, kind, false);
        if (hasSnapHoverBestPoint_) {
            if (currentSelectionMode == VectorDialogPickEndPoint && hasVectorStartPoint_) {
                updateVectorTwoPointHandles(&snapHoverBestPoint_, nullptr);
            } else if (currentSelectionMode == VectorDialogPickStartPoint) {
                updateVectorTwoPointHandles(nullptr, &snapHoverBestPoint_);
            } else {
                updateVectorTwoPointHandles();
            }
        } else if (vectorTwoPointSnapHasHoveredEdge_) {
            updateVectorTwoPointHandles();
        } else {
            gp_Pnt preview;
            if ((currentSelectionMode == VectorDialogPickStartPoint
                 || hasVectorStartPoint_)
                && tryPickPointOnModelForVector(x, y, preview)) {
                if (currentSelectionMode == VectorDialogPickEndPoint
                    && hasVectorStartPoint_) {
                    updateVectorTwoPointHandles(&preview, nullptr);
                } else if (currentSelectionMode == VectorDialogPickStartPoint) {
                    updateVectorTwoPointHandles(nullptr, &preview);
                } else {
                    updateVectorTwoPointHandles();
                }
            } else {
                updateVectorTwoPointHandles();
            }
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
        bool hoverIsFace = false;
        if (tryComputeVectorDirUnderCursor(
                x, y, baseDir, &hoverModelIndex, &hoverSubShapeId, &hoverIsFace)) {
            setCustomVectorDirFromDialog(baseDir);
            if (hasVectorDialogArrowOrigin_) {
                updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
            }

            // 自动判断和显式曲线/面模式都显示当前命中的面或边。
            if (vectorDialogModeIndex_ == 0 || vectorDialogModeIndex_ == 2 ||
                vectorDialogModeIndex_ == 3 || vectorDialogModeIndex_ == 4 ||
                vectorDialogModeIndex_ == 5 || vectorDialogModeIndex_ == 6) {
                const long long sid = static_cast<long long>(hoverSubShapeId);
                const bool hitValidSubShape = (hoverModelIndex >= 0 &&
                                               hoverSubShapeId != invalidSubShapeId &&
                                               sid > 0 &&
                                               sid <= static_cast<long long>(std::numeric_limits<int>::max()));
                if (hitValidSubShape) {
                    applyVectorDialogHoverSubShape(hoverModelIndex, hoverSubShapeId, hoverIsFace);
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

void Widget::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (ui && ui->checkBox_2) {
        updateCenterTriadViewport(ui->checkBox_2->isChecked());
    } else if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    QMainWindow::mousePressEvent(event);

    if (isInSelectionMode()) {
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!vtkWidget) {
        return;
    }
    const QPoint inVtk = vtkWidget->mapFrom(this, event->pos());
    if (!vtkWidget->rect().contains(inVtk)) {
        currentSelectedIndex = -1;
        highlightModel(-1);
        updateHistoryListSelection();
    }
}
