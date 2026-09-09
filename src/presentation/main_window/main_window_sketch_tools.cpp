#include "main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "presentation/dialogs/sketch/sketch_tool_input_dialog.h"

#include <QDialog>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <Qt>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <gp_Ax3.hxx>
#include <gp_Vec.hxx>
#include <vtkRenderWindow.h>

void Widget::openOrRaiseSketchToolInput(SketchToolInputDialog::ObjectKind initialObject)
{
    if (!sketchToolInputDialog_) {
        sketchToolInputDialog_ = new SketchToolInputDialog(initialObject, this);
        sketchToolInputDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchToolInputDialog_, &SketchToolInputDialog::closedByUser, this, [this]() {
            sketchToolInputDialog_ = nullptr;
            sketchCommittedPointValid_ = false;
            clearSketchPreviewArc();
            clearSketchPreviewLine();
            if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        });
        connect(sketchToolInputDialog_, &QObject::destroyed, this, [this]() {
            sketchToolInputDialog_ = nullptr;
        });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::valuesCommitted, this, &Widget::sketchApplyManualInputFromDialog);
        connect(sketchToolInputDialog_, &SketchToolInputDialog::inputKindChanged, this, [this](SketchToolInputDialog::InputKind) {
            if (hasActiveSketch_) {
                sketchCommittedPointValid_ = false;
                clearSketchPreviewArc();
                clearSketchPreviewLine();
                if (sketchLastHoverValid_) updateSketchToolInputDialogFields(sketchLastHoverPoint_);
                if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
            }
        });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::objectKindChanged, this,
                [this](SketchToolInputDialog::ObjectKind k) {
                    // 直线/圆弧切换时始终清除预览，避免相切圆弧(count==1)等情况下残留高亮
                    clearSketchPreviewArc();
                    clearSketchPreviewLine();
                    if (k == SketchToolInputDialog::ObjLine) {
                        if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 2) {
                            sketchClickCount_ = 0;
                        }
                        currentSelectionMode = SketchDrawLine;
                    } else {
                        currentSelectionMode = SketchDrawArc;
                    }
                    sketchCommittedPointValid_ = false;
                    if (hasActiveSketch_ && sketchLastHoverValid_) updateSketchToolInputDialogFields(sketchLastHoverPoint_);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                });
        connect(sketchToolInputDialog_, &SketchToolInputDialog::sketchButtonClicked, this, &Widget::openSketchPlaneDialogFromTool);
    } else {
        sketchToolInputDialog_->setObjectKind(initialObject);
    }
    sketchCommittedPointValid_ = false;
    sketchToolInputDialog_->show();
    sketchToolInputDialog_->raise();
    positionSketchToolInputDialog();
    {
        gp_Pnt ref = activeSketchPlane_.Location();
        if (sketchLastHoverValid_) ref = sketchLastHoverPoint_;
        updateSketchToolInputDialogFields(ref);
    }
}

void Widget::ensureDefaultSketchForTools()
{
    if (hasActiveSketch_) return;

    // 默认草图：使用世界 XY 平面
    activeSketch_.clear();
    clearSketchCommittedOverlay();
    activeSketchPlane_ = gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    activeSketch_.setPlane(activeSketchPlane_);
    activeSketch_.setName(tr("草图"));
    hasActiveSketch_ = true;
    sketchClickCount_ = 0;
    sketchChainTangentValid_ = false;
    sketchContourChaining_ = false;
    activeSketchHistoryIndex_ = -1;

    ensureSketchHistoryRecord();
    statusBar()->showMessage(tr("未确定基准平面：已使用默认XY平面开始草图，可点“草图”按钮重新选择平面。"), 4000);
}

void Widget::openSketchPlaneDialogFromTool()
{
    SelectionMode restoreMode = SketchDrawLine;
    SketchToolInputDialog::ObjectKind restoreObj = SketchToolInputDialog::ObjLine;
    if (sketchToolInputDialog_) {
        restoreObj = sketchToolInputDialog_->objectKind();
        restoreMode = (restoreObj == SketchToolInputDialog::ObjArc) ? SketchDrawArc : SketchDrawLine;
    } else {
        restoreMode = (currentSelectionMode == SketchDrawArc) ? SketchDrawArc : SketchDrawLine;
        restoreObj = (restoreMode == SketchDrawArc) ? SketchToolInputDialog::ObjArc : SketchToolInputDialog::ObjLine;
    }

    auto* dlg = new SketchCreateDialog(dialogParentWidget());
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    activeSketchCreateDialog_ = dlg;

    connect(dlg, &SketchCreateDialog::requestPickPlane, this, [this]() {
        currentSelectionMode = SketchPlaneSelection;
        statusBar()->showMessage(tr("草图：请在模型上点击一个平面作为参考平面。"), 4000);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg, restoreMode, restoreObj]() {
        if (!dlg->hasPickedPlane()) {
            statusBar()->showMessage(tr("草图：未拾取参考平面，保持当前平面不变。"), 3000);
            return;
        }

        // 若已有草图几何，为避免跨平面误差，切换平面时清空草图并重新开始
        if (hasActiveSketch_ && activeSketch_.isValid()) {
            activeSketch_.clear();
            clearSketchCommittedOverlay();
            activeSketchHistoryIndex_ = -1;
            clearSketchPreviewLine();
            clearSketchPreviewArc();
            clearSketchPreviewCircle();
            clearSketchPreviewConic();
        }

        activeSketchPlane_ = dlg->pickedPlane();
        activeSketch_.setPlane(activeSketchPlane_);
        activeSketch_.setName(tr("草图"));
        hasActiveSketch_ = true;
        sketchClickCount_ = 0;
        sketchCommittedPointValid_ = false;

        ensureSketchHistoryRecord();

        alignViewToSketchPlane(activeSketchPlane_);

        currentSelectionMode = restoreMode;
        openOrRaiseSketchToolInput(restoreObj);
        statusBar()->showMessage(tr("草图平面已更新，可继续绘制。"), 2500);
        if (vtkWidget) vtkWidget->setFocus();
    });

    connect(dlg, &QDialog::rejected, this, [this, restoreMode]() {
        // 若用户取消拾取平面，回到原绘制模式
        if (currentSelectionMode == SketchPlaneSelection) {
            currentSelectionMode = restoreMode;
        }
        activeSketchCreateDialog_ = nullptr;
    });

    connect(dlg, &QObject::destroyed, this, [this, restoreMode]() {
        if (activeSketchCreateDialog_) activeSketchCreateDialog_ = nullptr;
        if (currentSelectionMode == SketchPlaneSelection) currentSelectionMode = restoreMode;
    });

    dlg->show();
    dlg->raise();
}

void Widget::closeSketchToolInput()
{
    if (sketchToolInputDialog_) {
        sketchToolInputDialog_->close();
        sketchToolInputDialog_ = nullptr;
    }
    sketchCommittedPointValid_ = false;
}

void Widget::updateSketchToolInputDialogFields(const gp_Pnt& hoverWorld)
{
    if (!sketchToolInputDialog_ || !hasActiveSketch_) return;

    SketchToolInputDialog* dlg = sketchToolInputDialog_;
    const gp_Pln& pln = activeSketchPlane_;

    double xc = 0, yc = 0;
    sketchWorldToUV(pln, hoverWorld, xc, yc);

    if (dlg->inputKind() == SketchToolInputDialog::Coordinate) {
        dlg->setCoordinateValues(xc, yc);
        return;
    }

    if (dlg->objectKind() == SketchToolInputDialog::ObjLine) {
        if (currentSelectionMode == SketchDrawLine) {
            if (sketchClickCount_ == 0) {
                const gp_Pnt o = pln.Location();
                double len = 0, ang = 0;
                sketchLineLengthAngle(pln, o, hoverWorld, len, ang);
                dlg->setLineParameters(len, ang);
            } else if (sketchClickCount_ == 1) {
                double len = 0, ang = 0;
                sketchLineLengthAngle(pln, sketchP1_, hoverWorld, len, ang);
                dlg->setLineParameters(len, ang);
            }
        }
    } else if (dlg->objectKind() == SketchToolInputDialog::ObjArc) {
        if (currentSelectionMode == SketchDrawArc) {
            if (sketchClickCount_ == 0) {
                dlg->setArcParameters(0.0, 90.0);
            } else if (sketchClickCount_ == 1) {
                gp_Pnt pm;
                double R = 0.0;
                double sw = 90.0;
                if (sketchContourChaining_ && sketchChainTangentValid_
                    && sketchArcMidFromTangentAndEnd(pln, sketchP1_, sketchChainTangentDir_, hoverWorld, pm)) {
                    R = sketchArcRadiusFromThreePoints(sketchP1_, pm, hoverWorld);
                    try {
                        Handle(Geom_TrimmedCurve) ta = GC_MakeArcOfCircle(sketchP1_, pm, hoverWorld);
                        sw = std::abs(ta->LastParameter() - ta->FirstParameter()) * 180.0 / M_PI;
                    } catch (...) {
                    }
                }
                dlg->setArcParameters(R, sw);
            } else if (sketchClickCount_ == 2) {
                const double r = sketchArcRadiusFromThreePoints(sketchP1_, hoverWorld, sketchP2_);
                double sw = 90.0;
                try {
                    Handle(Geom_TrimmedCurve) ta = GC_MakeArcOfCircle(sketchP1_, hoverWorld, sketchP2_);
                    sw = std::abs(ta->LastParameter() - ta->FirstParameter()) * 180.0 / M_PI;
                } catch (...) {
                }
                dlg->setArcParameters(r, sw);
            }
        }
    }
}

void Widget::sketchApplyManualInputFromDialog()
{
    if (!sketchToolInputDialog_ || !hasActiveSketch_) return;

    SketchToolInputDialog* dlg = sketchToolInputDialog_;
    const gp_Pln& pln = activeSketchPlane_;

    if (dlg->inputKind() == SketchToolInputDialog::Coordinate) {
        const double xc = dlg->xcValue();
        const double yc = dlg->ycValue();
        sketchCommittedPoint_ = sketchUVToWorld(pln, xc, yc);
        sketchCommittedPointValid_ = true;
    } else if (dlg->objectKind() == SketchToolInputDialog::ObjLine) {
        if (sketchClickCount_ == 0) {
            const gp_Pnt o = pln.Location();
            const double len = dlg->lengthValue();
            const double ang = dlg->angleValue();
            sketchCommittedPoint_ = sketchLineEndFromLengthAngle(pln, o, len, ang);
        } else if (sketchClickCount_ == 1) {
            const double len = dlg->lengthValue();
            const double ang = dlg->angleValue();
            sketchCommittedPoint_ = sketchLineEndFromLengthAngle(pln, sketchP1_, len, ang);
        }
        sketchCommittedPointValid_ = true;
    } else {
        if (sketchClickCount_ == 1) {
            const double R = dlg->radiusValue();
            const double sw = dlg->sweepAngleValue();
            gp_Pnt E, pm, C;
            gp_Dir refTan = sketchChainTangentValid_ ? sketchChainTangentDir_ : pln.Position().XDirection();
            if (R > Precision::Confusion()
                && sketchArcFromStartRadiusSweep(pln, sketchP1_, refTan, R, sw, E, pm, C)) {
                try {
                    Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(sketchP1_, pm, E);
                    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc).Edge();
                    activeSketch_.addGeometry(edge);
                    ensureSketchHistoryRecord();
                    updateSketchHistoryShape();
                    if (sketchContourChaining_) {
                        sketchP1_ = E;
                        sketchClickCount_ = 1;
                        gp_Vec re(C, E);
                        gp_Vec ez(pln.Axis().Direction());
                        gp_Vec endTan = ez.Crossed(re);
                        if (endTan.Magnitude() > Precision::Confusion()) {
                            endTan.Normalize();
                            sketchChainTangentDir_ = gp_Dir(endTan);
                            sketchChainTangentValid_ = true;
                        }
                    } else {
                        sketchClickCount_ = 0;
                        sketchChainTangentValid_ = false;
                    }
                    clearSketchPreviewArc();
                    clearSketchPreviewLine();
                    markDocumentModified(true);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                    statusBar()->showMessage(tr("参数圆弧已创建。"), 2500);
                } catch (...) {
                    statusBar()->showMessage(tr("参数圆弧创建失败。"), 3000);
                }
            }
            return;
        }
        if (sketchClickCount_ == 2) {
            const double R = dlg->radiusValue();
            if (R > Precision::Confusion()) {
                gp_Pnt C;
                if (sketchArcCenterFromTwoPointsRadius(pln, sketchP1_, sketchP2_, R, sketchLastHoverPoint_, C)) {
                    const gp_Pnt pm = sketchArcMidPointOnCircle(pln, C, R, sketchP1_, sketchP2_);
                    sketchCommittedPoint_ = pm;
                    sketchCommittedPointValid_ = true;
                    updateSketchPreviewArc(sketchP1_, pm, sketchP2_);
                    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
                }
            }
        }
        return;
    }

    sketchLastHoverPoint_ = sketchCommittedPoint_;
    sketchLastHoverValid_ = true;

    if (currentSelectionMode == SketchDrawLine && sketchClickCount_ == 1) {
        updateSketchPreviewLine(sketchP1_, sketchCommittedPoint_);
    } else if (currentSelectionMode == SketchDrawRectangle && sketchClickCount_ == 1) {
        updateSketchPreviewRectangle(sketchP1_, sketchCommittedPoint_);
        clearSketchPreviewLine();
    } else if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 1) {
        updateSketchPreviewLine(sketchP1_, sketchCommittedPoint_);
    } else if (currentSelectionMode == SketchDrawArc && sketchClickCount_ == 2) {
        const gp_Pnt& pm = sketchCommittedPoint_;
        updateSketchPreviewArc(sketchP1_, pm, sketchP2_);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}
