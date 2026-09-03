// 两点定矢量：起/终点球形手柄 + 方向箭头（双击反转）
#include "main_window.h"
#include "ui_main_window.h"
#include "handle_geometry.h"
#include "vector_dialog.h"

#include <cmath>

#include <QSignalBlocker>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>

#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

namespace {

constexpr double kVectorTwoPointSphereR = 0.12;
constexpr double kVectorArrowScreenLen = 0.90;
constexpr int kHandleSelectToDragThresholdPx2 = 16;

const char* kSpecId = "vector_two_point";
const char* kCtrlStart = "start_sphere";
const char* kCtrlEnd = "end_sphere";
const char* kCtrlArrow = "direction_arrow";

HandleStateStyle vectorTwoPointCtrlStyle(const char* controlId, ControlState state)
{
    return HandleGeom::styleForControl(
        QString::fromLatin1(kSpecId),
        QString::fromLatin1(controlId),
        state);
}

bool rayPlaneHit(vtkRenderer* renderer, int x, int y, const gp_Pnt& planeOrigin, const gp_Dir& planeNormal,
                 gp_Pnt& outHit)
{
    if (!renderer) return false;
    renderer->SetDisplayPoint(x, y, 0.0);
    renderer->DisplayToWorld();
    double nearPt[4];
    renderer->GetWorldPoint(nearPt);
    renderer->SetDisplayPoint(x, y, 1.0);
    renderer->DisplayToWorld();
    double farPt[4];
    renderer->GetWorldPoint(farPt);
    if (std::abs(nearPt[3]) < 1e-12 || std::abs(farPt[3]) < 1e-12) return false;
    gp_Pnt p0(nearPt[0] / nearPt[3], nearPt[1] / nearPt[3], nearPt[2] / nearPt[3]);
    gp_Pnt p1(farPt[0] / farPt[3], farPt[1] / farPt[3], farPt[2] / farPt[3]);
    gp_Vec ray(p0, p1);
    if (ray.Magnitude() < 1e-12) return false;
    gp_Dir rayDir(ray);
    const double denom = planeNormal.Dot(rayDir);
    if (std::abs(denom) < 1e-12) return false;
    const double t = gp_Vec(p0, planeOrigin).Dot(planeNormal) / denom;
    outHit = p0.Translated(gp_Vec(rayDir) * t);
    return true;
}

} // namespace

Widget::VectorTwoPointHandlePart Widget::pickVectorTwoPointHandlePart(int x, int y)
{
    if (!renderer) return VectorTwoPointHandlePart::None;

    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->PickFromListOn();
    picker->InitializePickList();
    if (vectorTwoPointStartSphereActor_) picker->AddPickList(vectorTwoPointStartSphereActor_);
    if (vectorTwoPointEndSphereActor_) picker->AddPickList(vectorTwoPointEndSphereActor_);
    if (vectorDialogArrowActor_) picker->AddPickList(vectorDialogArrowActor_);

    vtkRenderer* overlay = referenceOverlay();
    picker->Pick(x, y, 0, overlay ? overlay : renderer.GetPointer());
    vtkActor* hit = picker->GetActor();
    if (!hit && overlay) {
        picker->Pick(x, y, 0, renderer.GetPointer());
        hit = picker->GetActor();
    }

    if (!hit) return VectorTwoPointHandlePart::None;
    if (hit == vectorTwoPointStartSphereActor_.GetPointer()) return VectorTwoPointHandlePart::StartSphere;
    if (hit == vectorTwoPointEndSphereActor_.GetPointer()) return VectorTwoPointHandlePart::EndSphere;
    if (hit == vectorDialogArrowActor_.GetPointer()) return VectorTwoPointHandlePart::DirectionArrow;
    return VectorTwoPointHandlePart::None;
}

bool Widget::isVectorTwoPointDialogActive() const
{
    return vectorDialog_ && vectorDialogModeIndex_ == 1;
}

void Widget::beginVectorTwoPointPickStart(bool refreshSnapKindsFromDialog)
{
    if (!isVectorTwoPointDialogActive()) return;

    if (refreshSnapKindsFromDialog && vectorDialog_) {
        vectorTwoPointStartSnapKind_ = vectorDialog_->twoPointStartSnapKindForPick();
        vectorTwoPointEndSnapKind_ = vectorDialog_->twoPointEndSnapKindForPick();
    }

    hasVectorStartPoint_ = false;
    hasVectorEndPoint_ = false;
    hasCustomVectorDir_ = false;
    vectorTwoPointAwaitingEndPick_ = true;
    clearVectorTwoPointHandles();

    currentSelectionMode = VectorDialogPickStartPoint;
    if (vectorDialogArrowActor_) vectorDialogArrowActor_->SetVisibility(false);

    applyTwoPointVectorSnapKind(vectorTwoPointStartSnapKind_, true);
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::onVectorTwoPointStartPicked(const gp_Pnt& point)
{
    vectorStartPoint_ = point;
    hasVectorStartPoint_ = true;
    hasVectorDialogArrowOrigin_ = true;
    vectorDialogArrowOrigin_ = point;

    clearSnapHover();
    clearPointSelectionHover();
    updateVectorTwoPointHandles();
    if (vectorDialogArrowActor_) vectorDialogArrowActor_->SetVisibility(false);

    if (vectorTwoPointAwaitingEndPick_) {
        currentSelectionMode = VectorDialogPickEndPoint;
        applyTwoPointVectorSnapKind(vectorTwoPointEndSnapKind_, false);
    } else {
        if (hasVectorEndPoint_) {
            applyVectorTwoPointFromEndpoints();
        }
        currentSelectionMode = VectorTwoPointInteractive;
        disableSnapUiAfterVectorTwoPointComplete();
        updateVectorTwoPointHandles();
    }
}

void Widget::applyVectorTwoPointFromEndpoints()
{
    if (!hasVectorStartPoint_ || !hasVectorEndPoint_) return;

    gp_Vec v(vectorStartPoint_, vectorEndPoint_);
    if (v.Magnitude() <= Precision::Confusion()) return;

    gp_Dir dir(v);
    setCustomVectorDirFromDialog(dir);
    hasVectorDialogArrowOrigin_ = true;
    vectorDialogArrowOrigin_ = vectorStartPoint_;

    if (vectorDialog_) {
        vectorDialog_->setVectorDirDisplay(customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
    }
}

void Widget::onVectorTwoPointEndPicked(const gp_Pnt& point)
{
    vectorEndPoint_ = point;
    hasVectorEndPoint_ = true;
    vectorTwoPointAwaitingEndPick_ = false;
    applyVectorTwoPointFromEndpoints();

    vectorTwoPointHandleHover_ = VectorTwoPointHandlePart::None;
    currentSelectionMode = VectorTwoPointInteractive;
    disableSnapUiAfterVectorTwoPointComplete();
    updateVectorTwoPointHandles();
}

void Widget::disableSnapUiAfterVectorTwoPointComplete()
{
    snap_.enabled = false;
    setSnapArmed(false);
    clearSnapPersistentPoints();
    clearSnapSelected();
    clearSnapHover();
    clearSelectedPoint();
    clearPointSelectionHover();

    if (ui) {
        QSignalBlocker bUse(ui->Use_Capture);
        QSignalBlocker bClosed(ui->Capture_Closed);
        QSignalBlocker bEnd(ui->Capture_Endpoint);
        QSignalBlocker bMid(ui->Capture_Midpoint);
        QSignalBlocker bInt(ui->Capture_Insertsectionpoint);
        QSignalBlocker bArcCen(ui->Capture_Arccenterpoint);
        QSignalBlocker bQuad(ui->Capture_Quadrantpoint);
        ui->Use_Capture->setChecked(false);
        ui->Capture_Closed->setChecked(false);
        ui->Capture_Endpoint->setChecked(false);
        ui->Capture_Midpoint->setChecked(false);
        ui->Capture_Insertsectionpoint->setChecked(false);
        ui->Capture_Arccenterpoint->setChecked(false);
        ui->Capture_Quadrantpoint->setChecked(false);
        clearTabPointSnapToolbarButtons();
        if (ui->pushButton_71) {
            QSignalBlocker b71(ui->pushButton_71);
            ui->pushButton_71->setChecked(false);
        }
        if (ui->pushButton_70) {
            QSignalBlocker b70(ui->pushButton_70);
            ui->pushButton_70->setChecked(false);
        }
    }
    mergeSnapFiltersFromToolbarAndCaptureUi();
    snap_.enabled = false;
    snap_.armed = false;
}

void Widget::updateVectorTwoPointSnapPresentation(int x, int y, int snapKind, bool dragMode)
{
    if (snapKind == -1) {
        clearSnapHover();
        hasSnapHoverBestPoint_ = false;
        return;
    }

    reapplyTwoPointSnapKindFilters(snapKind);

    SnapHoverOptions opts;
    opts.suppressHoverBall = true;
    opts.suppressHoverText = true;
    opts.showCandidateGhosts = dragMode;
    opts.expandScreenPixelRadius = 110.0;
    updateSnapHover(x, y, opts);
}

bool Widget::tryPickVectorTwoPointSnapAt(int x, int y)
{
    if (!snap_.armed) return false;

    pickSnapAt(x, y);
    if (hasSnapSelectedPoint_) return true;

    // 点击边线时：若悬停已算出最近捕捉点，则直接采用（与边高亮/预览一致）
    if (hasSnapHoverBestPoint_) {
        snapSelectedPoint_ = snapHoverBestPoint_;
        hasSnapSelectedPoint_ = true;
        clearSnapHover();
        return true;
    }
    return false;
}

gp_Pnt Widget::resolveVectorTwoPointPreviewPosition(int x, int y, int snapKind)
{
    gp_Pnt hit(0, 0, 0);
    const gp_Pnt planeOrigin(0, 0, 0);
    const gp_Dir planeNormal(0, 0, 1);
    if (renderer && rayPlaneHit(renderer, x, y, planeOrigin, planeNormal, hit)) {
        hit.SetZ(0.0);
    }

    if (snapKind == -1) {
        gp_Pnt modelP;
        if (tryPickPointOnModelForVector(x, y, modelP)) {
            return modelP;
        }
        return hit;
    }

    updateVectorTwoPointSnapPresentation(x, y, snapKind, false);
    if (hasSnapHoverBestPoint_) {
        return snapHoverBestPoint_;
    }
    gp_Pnt modelP;
    if (tryPickPointOnModelForVector(x, y, modelP)) {
        return modelP;
    }
    return hit;
}

gp_Pnt Widget::resolveVectorTwoPointDragPosition(int x, int y, int snapKind) const
{
    gp_Pnt hit(0, 0, 0);
    const gp_Pnt planeOrigin(0, 0, 0);
    const gp_Dir planeNormal(0, 0, 1);
    if (renderer && rayPlaneHit(renderer, x, y, planeOrigin, planeNormal, hit)) {
        hit.SetZ(0.0);
    }

    if (snapKind == -1) {
        gp_Pnt modelP;
        if (const_cast<Widget*>(this)->tryPickPointOnModelForVector(x, y, modelP)) {
            return modelP;
        }
        return hit;
    }

    const_cast<Widget*>(this)->updateVectorTwoPointSnapPresentation(x, y, snapKind, true);
    if (hasSnapHoverBestPoint_) {
        return snapHoverBestPoint_;
    }
    gp_Pnt modelP;
    if (const_cast<Widget*>(this)->tryPickPointOnModelForVector(x, y, modelP)) {
        return modelP;
    }
    return hit;
}

void Widget::reverseVectorTwoPointDirection()
{
    if (!hasVectorStartPoint_ || !hasVectorEndPoint_) return;

    vectorDialogReverse_ = !vectorDialogReverse_;
    if (vectorDialog_) {
        vectorDialog_->setReverseState(vectorDialogReverse_);
    }

    if (hasVectorDialogBaseDir_) {
        gp_Dir base = vectorDialogBaseDir_;
        setCustomVectorDirFromDialog(base);
        if (vectorDialog_) {
            vectorDialog_->setVectorDirDisplay(customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
        }
    }
    updateVectorTwoPointHandles();
}

void Widget::clearVectorTwoPointHandles()
{
    vectorTwoPointHandlesVisible_ = false;
    vectorTwoPointHandleHover_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleDragging_ = false;

    if (vectorTwoPointStartSphereActor_) {
        removeSceneActor(vectorTwoPointStartSphereActor_);
        vectorTwoPointStartSphereActor_ = nullptr;
    }
    if (vectorTwoPointEndSphereActor_) {
        removeSceneActor(vectorTwoPointEndSphereActor_);
        vectorTwoPointEndSphereActor_ = nullptr;
    }
    if (vectorTwoPointLineActor_) {
        removeSceneActor(vectorTwoPointLineActor_);
        vectorTwoPointLineActor_ = nullptr;
    }
    if (vectorDialogArrowActor_) {
        vectorDialogArrowActor_->SetVisibility(false);
        vectorDialogArrowActor_->SetPickable(0);
    }
    clearVectorTwoPointSnapGhosts();
}

void Widget::updateVectorTwoPointHandles(const gp_Pnt* previewEnd, const gp_Pnt* previewStart)
{
    if (!renderer || !vtkWidget) return;
    if (!isVectorTwoPointDialogActive()) {
        clearVectorTwoPointHandles();
        return;
    }

    const bool showStart = hasVectorStartPoint_ || (previewStart != nullptr);
    const bool showEnd = hasVectorEndPoint_ || (previewEnd != nullptr);
    if (!showStart && !showEnd) {
        clearVectorTwoPointHandles();
        return;
    }

    const gp_Pnt pStart = hasVectorStartPoint_ ? vectorStartPoint_
                                              : (previewStart ? *previewStart : gp_Pnt());
    gp_Pnt pEnd = hasVectorEndPoint_ ? vectorEndPoint_ : (previewEnd ? *previewEnd : pStart);
    if (showStart && showEnd) {
        gp_Vec v(pStart, pEnd);
        if (v.Magnitude() <= Precision::Confusion() && previewEnd == nullptr && previewStart == nullptr) {
            if (!hasVectorEndPoint_) {
                clearVectorTwoPointHandles();
                return;
            }
        }
    }

    const bool isPreviewEnd = (previewEnd != nullptr && !hasVectorEndPoint_);
    const bool isPreviewStart = (previewStart != nullptr && !hasVectorStartPoint_);
    const bool startLocked = hasVectorStartPoint_ && isPreviewEnd;

    vectorTwoPointHandlesVisible_ = true;
    ensureVectorDialogArrowActor();

    // 轨迹线
    if (showStart && showEnd) {
        if (!vectorTwoPointLineActor_) {
            vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(line->GetOutputPort());
            vectorTwoPointLineActor_ = vtkSmartPointer<vtkActor>::New();
            vectorTwoPointLineActor_->SetMapper(mapper);
            vectorTwoPointLineActor_->SetPickable(false);
            addReferenceActor(vectorTwoPointLineActor_);
        }
        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(pStart.X(), pStart.Y(), pStart.Z());
        line->SetPoint2(pEnd.X(), pEnd.Y(), pEnd.Z());
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(vectorTwoPointLineActor_->GetMapper())) {
            mapper->SetInputConnection(line->GetOutputPort());
            mapper->Modified();
        }
        const bool lineActive = vectorTwoPointHandleDragging_
            || vectorTwoPointHandleSelected_ != VectorTwoPointHandlePart::None
            || vectorTwoPointHandleHover_ != VectorTwoPointHandlePart::None;
        const ControlState lineSt = HandleGeom::resolveControlState(
            vectorTwoPointHandleDragging_, lineActive, lineActive);
        HandleGeom::applyStateStyle(vectorTwoPointLineActor_->GetProperty(),
                                    vectorTwoPointCtrlStyle(kCtrlArrow, lineSt));
        vectorTwoPointLineActor_->SetVisibility(true);
    } else if (vectorTwoPointLineActor_) {
        vectorTwoPointLineActor_->SetVisibility(false);
    }

    // 起点球
    if (showStart) {
        if (!vectorTwoPointStartSphereActor_) {
            auto sph = HandleGeom::makeSphereSource(
                HandleGeom::defaultHandleSphereParams(kVectorTwoPointSphereR));
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(sph->GetOutputPort());
            vectorTwoPointStartSphereActor_ = vtkSmartPointer<vtkActor>::New();
            vectorTwoPointStartSphereActor_->SetMapper(mapper);
            vectorTwoPointStartSphereActor_->SetPickable(true);
            addReferenceActor(vectorTwoPointStartSphereActor_);
        }
        vectorTwoPointStartSphereActor_->SetPosition(pStart.X(), pStart.Y(), pStart.Z());
        vectorTwoPointStartSphereActor_->SetVisibility(true);

        const bool dragging = vectorTwoPointHandleDragging_
            && vectorTwoPointHandleDrag_ == VectorTwoPointHandlePart::StartSphere;
        const bool selected = !vectorTwoPointHandleDragging_
            && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::StartSphere;
        const bool hover = !vectorTwoPointHandleDragging_
            && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::None
            && vectorTwoPointHandleHover_ == VectorTwoPointHandlePart::StartSphere;
        ControlState st = HandleGeom::resolveControlState(dragging, selected, hover);
        if (startLocked) {
            st = ControlState::Selected;
        } else if (isPreviewStart) {
            st = ControlState::Hover;
        }
        HandleStateStyle sphereStyle = vectorTwoPointCtrlStyle(kCtrlStart, st);
        if (isPreviewStart || startLocked) {
            sphereStyle.scaleFactor = vectorTwoPointCtrlStyle(kCtrlStart, ControlState::Default).scaleFactor;
        }
        HandleGeom::applyStateStyle(vectorTwoPointStartSphereActor_,
                                    sphereStyle,
                                    overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z()),
                                    true);
        vectorTwoPointStartSphereActor_->SetPickable(isPreviewStart ? 0 : 1);
    } else if (vectorTwoPointStartSphereActor_) {
        vectorTwoPointStartSphereActor_->SetVisibility(false);
    }

    // 终点球
    if (showEnd) {
        if (!vectorTwoPointEndSphereActor_) {
            auto sph = HandleGeom::makeSphereSource(
                HandleGeom::defaultHandleSphereParams(kVectorTwoPointSphereR));
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(sph->GetOutputPort());
            vectorTwoPointEndSphereActor_ = vtkSmartPointer<vtkActor>::New();
            vectorTwoPointEndSphereActor_->SetMapper(mapper);
            vectorTwoPointEndSphereActor_->SetPickable(true);
            addReferenceActor(vectorTwoPointEndSphereActor_);
        }
        vectorTwoPointEndSphereActor_->SetPosition(pEnd.X(), pEnd.Y(), pEnd.Z());
        vectorTwoPointEndSphereActor_->SetVisibility(true);
        vectorTwoPointEndSphereActor_->SetPickable(isPreviewEnd ? 0 : 1);

        const bool dragging = vectorTwoPointHandleDragging_
            && vectorTwoPointHandleDrag_ == VectorTwoPointHandlePart::EndSphere;
        const bool selected = !vectorTwoPointHandleDragging_
            && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::EndSphere;
        const bool hover = !vectorTwoPointHandleDragging_
            && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::None
            && vectorTwoPointHandleHover_ == VectorTwoPointHandlePart::EndSphere;
        ControlState st = HandleGeom::resolveControlState(dragging, selected, hover);
        if (isPreviewEnd) {
            st = ControlState::Hover;
        }
        HandleStateStyle sphereStyle = vectorTwoPointCtrlStyle(kCtrlEnd, st);
        if (isPreviewEnd) {
            sphereStyle.scaleFactor = vectorTwoPointCtrlStyle(kCtrlEnd, ControlState::Default).scaleFactor;
        }
        HandleGeom::applyStateStyle(vectorTwoPointEndSphereActor_,
                                    sphereStyle,
                                    overlayWorldScaleAt(pEnd.X(), pEnd.Y(), pEnd.Z()),
                                    true);
    } else if (vectorTwoPointEndSphereActor_) {
        vectorTwoPointEndSphereActor_->SetVisibility(false);
    }

    // 方向箭头
    if (showStart && showEnd) {
        gp_Vec v(pStart, pEnd);
        if (v.Magnitude() > Precision::Confusion()) {
            gp_Dir dir(v);
            if (vectorDialogReverse_) dir.Reverse();

            const double arrowWorldLen =
                kVectorArrowScreenLen * overlayWorldScaleAt(pStart.X(), pStart.Y(), pStart.Z());
            HandleGeom::applyArrowOrientation(vectorDialogArrowTransform_, pStart, dir, arrowWorldLen);

            const bool arrowDragging = vectorTwoPointHandleDragging_
                && vectorTwoPointHandleDrag_ == VectorTwoPointHandlePart::DirectionArrow;
            const bool arrowSelected = !vectorTwoPointHandleDragging_
                && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::DirectionArrow;
            const bool arrowHover = !vectorTwoPointHandleDragging_
                && vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::None
                && vectorTwoPointHandleHover_ == VectorTwoPointHandlePart::DirectionArrow;
            const ControlState arrowSt = HandleGeom::resolveControlState(arrowDragging, arrowSelected, arrowHover);
            HandleGeom::applyStateStyle(vectorDialogArrowActor_->GetProperty(),
                                        vectorTwoPointCtrlStyle(kCtrlArrow, arrowSt));

            vectorDialogArrowActor_->SetVisibility(true);
            vectorDialogArrowActor_->SetPickable(1);
        } else if (vectorDialogArrowActor_) {
            vectorDialogArrowActor_->SetVisibility(false);
        }
    } else if (vectorDialogArrowActor_) {
        vectorDialogArrowActor_->SetVisibility(false);
    }

    if (vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::handleVectorTwoPointHandleMouseDown(int x, int y)
{
    if (!isVectorTwoPointDialogActive()) return;
    if (currentSelectionMode != VectorTwoPointInteractive
        && currentSelectionMode != VectorTwoPointHandleDrag) {
        return;
    }
    if (!hasVectorStartPoint_ && !hasVectorEndPoint_) return;

    const VectorTwoPointHandlePart part = pickVectorTwoPointHandlePart(x, y);

    if (part == VectorTwoPointHandlePart::None) {
        vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
        vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
        vectorTwoPointHandleDragging_ = false;
        return;
    }

    vectorTwoPointHandleSavedMode_ = currentSelectionMode;
    vectorTwoPointHandleSelected_ = part;
    vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleDragging_ = false;
    vectorTwoPointHandleSelectDownX_ = x;
    vectorTwoPointHandleSelectDownY_ = y;
    currentSelectionMode = VectorTwoPointHandleDrag;

  // 拖拽前按当前点类型武装捕捉
    if (part == VectorTwoPointHandlePart::StartSphere) {
        applyTwoPointVectorSnapKind(vectorTwoPointStartSnapKind_, false);
    } else if (part == VectorTwoPointHandlePart::EndSphere) {
        applyTwoPointVectorSnapKind(vectorTwoPointEndSnapKind_, false);
    }

    updateVectorTwoPointHandles();
}

void Widget::handleVectorTwoPointHandleMouseMove(int x, int y)
{
    if (!isVectorTwoPointDialogActive()) return;
    if (currentSelectionMode != VectorTwoPointInteractive
        && currentSelectionMode != VectorTwoPointHandleDrag) {
        return;
    }

    if (!vectorTwoPointHandleDragging_) {
        if (vectorTwoPointHandleSelected_ != VectorTwoPointHandlePart::None) {
            const int dx = x - vectorTwoPointHandleSelectDownX_;
            const int dy = y - vectorTwoPointHandleSelectDownY_;
            if (dx * dx + dy * dy <= kHandleSelectToDragThresholdPx2) {
                updateVectorTwoPointHandles();
                return;
            }
            if (vectorTwoPointHandleSelected_ == VectorTwoPointHandlePart::DirectionArrow) {
                return;
            }
            vectorTwoPointHandleDragging_ = true;
            vectorTwoPointHandleDrag_ = vectorTwoPointHandleSelected_;
            vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
        } else {
            const VectorTwoPointHandlePart hover = pickVectorTwoPointHandlePart(x, y);
            if (hover != vectorTwoPointHandleHover_) {
                vectorTwoPointHandleHover_ = hover;
                updateVectorTwoPointHandles();
            }
            return;
        }
    }

    if (vectorTwoPointHandleDrag_ == VectorTwoPointHandlePart::StartSphere) {
        const gp_Pnt p = resolveVectorTwoPointDragPosition(x, y, vectorTwoPointStartSnapKind_);
        vectorStartPoint_ = p;
        hasVectorStartPoint_ = true;
        hasVectorDialogArrowOrigin_ = true;
        vectorDialogArrowOrigin_ = p;
        if (hasVectorEndPoint_) {
            applyVectorTwoPointFromEndpoints();
        }
        updateVectorTwoPointHandles();
    } else if (vectorTwoPointHandleDrag_ == VectorTwoPointHandlePart::EndSphere) {
        const gp_Pnt p = resolveVectorTwoPointDragPosition(x, y, vectorTwoPointEndSnapKind_);
        vectorEndPoint_ = p;
        hasVectorEndPoint_ = true;
        applyVectorTwoPointFromEndpoints();
        updateVectorTwoPointHandles();
    }
}

void Widget::handleVectorTwoPointHandleMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    const VectorTwoPointHandlePart releasedPart = vectorTwoPointHandleSelected_;
    const bool wasDragging = vectorTwoPointHandleDragging_;

    if (wasDragging) {
        clearVectorTwoPointSnapGhosts();
        clearSnapHover();
    }

    vectorTwoPointHandleDragging_ = false;
    vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
    vectorTwoPointHandleSelectDownX_ = vectorTwoPointHandleSelectDownY_ = -1;

    if (currentSelectionMode == VectorTwoPointHandleDrag) {
        if (!wasDragging && releasedPart == VectorTwoPointHandlePart::StartSphere) {
            vectorTwoPointAwaitingEndPick_ = false;
            currentSelectionMode = VectorDialogPickStartPoint;
            applyTwoPointVectorSnapKind(vectorTwoPointStartSnapKind_, false);
        } else if (!wasDragging && releasedPart == VectorTwoPointHandlePart::EndSphere) {
            vectorTwoPointAwaitingEndPick_ = false;
            if (!hasVectorStartPoint_) {
                currentSelectionMode = vectorTwoPointHandleSavedMode_;
            } else {
                currentSelectionMode = VectorDialogPickEndPoint;
                applyTwoPointVectorSnapKind(vectorTwoPointEndSnapKind_, false);
            }
        } else {
            currentSelectionMode = VectorTwoPointInteractive;
            disableSnapUiAfterVectorTwoPointComplete();
        }
    }

    updateVectorTwoPointHandles();
}

void Widget::handleVectorTwoPointArrowDoubleClick(int x, int y)
{
    if (!isVectorTwoPointDialogActive() || !hasVectorEndPoint_) return;

    const VectorTwoPointHandlePart part = pickVectorTwoPointHandlePart(x, y);
    if (part != VectorTwoPointHandlePart::DirectionArrow) return;

    reverseVectorTwoPointDirection();
    if (statusBar()) {
        statusBar()->showMessage(tr("已反转矢量方向"), 1500);
    }
}
