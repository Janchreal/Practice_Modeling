// VTK 视图鼠标交互：MouseInteractorStyle 实现（从 main_window.cpp 拆出，便于维护）
#include "mouse_interactor.h"

#include <QCursor>
#include <QPoint>

#include <cmath>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

vtkStandardNewMacro(MouseInteractorStyle);

namespace {
double vecLen3(const double v[3])
{
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

bool normalize3(double v[3])
{
    const double len = vecLen3(v);
    if (len < 1e-12) return false;
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
    return true;
}

void cross3(const double a[3], const double b[3], double out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

void rotateVecAroundAxis(double v[3], const double axis[3], double angleRad)
{
    const double c = std::cos(angleRad);
    const double s = std::sin(angleRad);
    const double dot = v[0] * axis[0] + v[1] * axis[1] + v[2] * axis[2];
    const double cross[3] = {
        axis[1] * v[2] - axis[2] * v[1],
        axis[2] * v[0] - axis[0] * v[2],
        axis[0] * v[1] - axis[1] * v[0]
    };
    v[0] = v[0] * c + cross[0] * s + axis[0] * dot * (1.0 - c);
    v[1] = v[1] * c + cross[1] * s + axis[1] * dot * (1.0 - c);
    v[2] = v[2] * c + cross[2] * s + axis[2] * dot * (1.0 - c);
}

bool isHoverInteractionMode(SelectionWindowState::SelectionMode mode)
{
    switch (mode) {
    case SelectionWindowState::PointSelection:
    case SelectionWindowState::ExtrusionSelection:
    case SelectionWindowState::EdgeSelection:
    case SelectionWindowState::FaceSelection:
    case SelectionWindowState::VectorDialogPickDirection:
    case SelectionWindowState::VectorDialogPickStartPoint:
    case SelectionWindowState::VectorDialogPickEndPoint:
    case SelectionWindowState::FilletEdgeSelection:
    case SelectionWindowState::FilletRadiusHandleDrag:
    case SelectionWindowState::ChamferEdgeSelection:
    case SelectionWindowState::ChamferAsymHandleDrag:
    case SelectionWindowState::SketchPlaneSelection:
    case SelectionWindowState::SketchDrawLine:
    case SelectionWindowState::SketchDrawArc:
    case SelectionWindowState::SketchDrawRectangle:
    case SelectionWindowState::SketchDrawCircle:
    case SelectionWindowState::SketchDrawPoint:
    case SelectionWindowState::SketchDrawPolygon:
    case SelectionWindowState::SketchPolygonPick:
    case SelectionWindowState::SketchEllipsePick:
    case SelectionWindowState::SketchEllipseAdjust:
    case SelectionWindowState::SketchConicPick:
    case SelectionWindowState::SketchConicDragControl:
    case SelectionWindowState::SketchQuickTrim:
    case SelectionWindowState::SketchQuickExtend:
    case SelectionWindowState::CuboidInteractive:
    case SelectionWindowState::PatternBodySelection:
    case SelectionWindowState::PatternPitchInteractive:
    case SelectionWindowState::ExtrusionHandleDrag:
    case SelectionWindowState::RevolveHandleDrag:
    case SelectionWindowState::VectorTwoPointInteractive:
    case SelectionWindowState::VectorTwoPointHandleDrag:
    case SelectionWindowState::FeatureBooleanTargetSelect:
        return true;
    default:
        return false;
    }
}
} // namespace

// 用于区分 VTK 视图中的“点击”与“拖拽”（仅供本翻译单元内 MouseInteractorStyle 使用）
static int lastMouseX = -1;
static int lastMouseY = -1;
static bool mouseMoved = false;

void resetVtkMouseDragTracking()
{
    lastMouseX = -1;
    lastMouseY = -1;
    mouseMoved = false;
}

void MouseInteractorStyle::rotateCameraAroundWorldCenter(int prevX, int prevY, int x, int y)
{
    if (!interactionContext_.renderer || !this->Interactor) return;

    vtkRenderer* mainRenderer = interactionContext_.renderer();
    if (!mainRenderer) return;

    vtkCamera* cam = mainRenderer->GetActiveCamera();
    if (!cam) return;

    if (prevX < 0 || prevY < 0) {
        navLastX_ = x;
        navLastY_ = y;
        return;
    }

    const int dx = x - prevX;
    const int dy = y - prevY;
    if (dx == 0 && dy == 0) return;

    int* size = this->Interactor->GetSize();
    const double width = (size && size[0] > 0) ? static_cast<double>(size[0]) : 1.0;
    const double height = (size && size[1] > 0) ? static_cast<double>(size[1]) : 1.0;
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
    const double motionFactor = this->GetMotionFactor();
    // 注意：VTK 的屏幕坐标 y 向下递增，因此这里需要保证“向下拖拽 => 向下俯仰旋转”
    const double azimuthRad = (static_cast<double>(dx) * (-20.0 / width) * motionFactor) * kDegToRad;
    const double elevationRad = (static_cast<double>(dy) * (20.0 / height) * motionFactor) * kDegToRad;

    double pos[3] = {0.0, 0.0, 0.0};
    double fp[3] = {0.0, 0.0, 0.0};
    double up[3] = {0.0, 1.0, 0.0};
    cam->GetPosition(pos);
    cam->GetFocalPoint(fp);
    cam->GetViewUp(up);
    if (!normalize3(up)) {
        up[0] = 0.0;
        up[1] = 1.0;
        up[2] = 0.0;
    }

    if (std::abs(azimuthRad) > 1e-12) {
        rotateVecAroundAxis(pos, up, azimuthRad);
        rotateVecAroundAxis(fp, up, azimuthRad);
    }

    if (std::abs(elevationRad) > 1e-12) {
        double viewDir[3] = {fp[0] - pos[0], fp[1] - pos[1], fp[2] - pos[2]};
        if (!normalize3(viewDir)) {
            viewDir[0] = 0.0;
            viewDir[1] = 0.0;
            viewDir[2] = -1.0;
        }

        double right[3] = {1.0, 0.0, 0.0};
        cross3(viewDir, up, right);
        if (!normalize3(right)) {
            right[0] = 1.0;
            right[1] = 0.0;
            right[2] = 0.0;
        }

        rotateVecAroundAxis(pos, right, elevationRad);
        rotateVecAroundAxis(fp, right, elevationRad);
        rotateVecAroundAxis(up, right, elevationRad);
        normalize3(up);
    }

    cam->SetPosition(pos);
    cam->SetFocalPoint(fp);
    cam->SetViewUp(up);
    cam->OrthogonalizeViewUp();
    if (interactionContext_.refreshCameraClippingRange)
        interactionContext_.refreshCameraClippingRange();
}

void MouseInteractorStyle::OnLeftButtonDown()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.selectionMode || !this->Interactor) {
        return;
    }

    // 记录鼠标按下位置
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    lastMouseX = x;
    lastMouseY = y;
    mouseMoved = false;

    // 获取当前选择模式
    const MouseInteractionContext::SelectionMode mode = interactionContext_.selectionMode();

    // 检查是否处于布尔运算选择模式
    const bool isBooleanSelection = (mode == SelectionWindowState::SelectTarget
                                     || mode == SelectionWindowState::SelectTool);

    // 检查是否处于拉伸选择模式
    const bool isExtrusionSelection = (mode == SelectionWindowState::ExtrusionSelection
                                       || mode == SelectionWindowState::FaceSelection
                                       || mode == SelectionWindowState::EdgeSelection);
    // 矢量对话框拾取（点击确认，拖拽旋转）
    const bool isVectorDialogSelection = (mode == SelectionWindowState::VectorDialogPickDirection
                                          || mode == SelectionWindowState::VectorDialogPickStartPoint
                                          || mode == SelectionWindowState::VectorDialogPickEndPoint);
    bool isSketchEditSelection =
        (mode == SelectionWindowState::SketchQuickTrim || mode == SelectionWindowState::SketchQuickExtend);
    const bool isCuboidInteractive = (mode == SelectionWindowState::CuboidInteractive);
    const bool isPatternPitchInteractive = (mode == SelectionWindowState::PatternPitchInteractive);
    const bool isExtrusionHandle = (mode == SelectionWindowState::ExtrusionHandleDrag);
    const bool isRevolveHandle = (mode == SelectionWindowState::RevolveHandleDrag);

    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();

    if (isBooleanSelection) {
        // 在布尔运算选择模式下，只处理选择逻辑，不调用基类方法
        if (interactionContext_.handleVtkMouseClick)
            interactionContext_.handleVtkMouseClick(x, y);
        // 在选择模式下，完全阻止基类处理
        return;
    }

    // 拉伸/旋转手柄优先：即使当前是曲线/面片选择模式，也先尝试抓取手柄
    if (interactionContext_.hasExtrusionHandleContext && interactionContext_.hasExtrusionHandleContext()) {
        if (interactionContext_.handleExtrusionHandleMouseDown)
            interactionContext_.handleExtrusionHandleMouseDown(x, y);
        if (interactionContext_.selectionMode()
            == SelectionWindowState::ExtrusionHandleDrag) {
            return;
        }
    }
    if (interactionContext_.hasRevolveHandleContext && interactionContext_.hasRevolveHandleContext()) {
        if (interactionContext_.handleRevolveHandleMouseDown)
            interactionContext_.handleRevolveHandleMouseDown(x, y);
        if (interactionContext_.selectionMode()
            == SelectionWindowState::RevolveHandleDrag) {
            return;
        }
    }
    if (interactionContext_.hasChamferHandleContext && interactionContext_.hasChamferHandleContext()) {
        if (interactionContext_.handleChamferAsymHandleMouseDown)
            interactionContext_.handleChamferAsymHandleMouseDown(x, y);
        if (interactionContext_.selectionMode()
            == SelectionWindowState::ChamferAsymHandleDrag) {
            return;
        }
    }
    if (interactionContext_.hasFilletHandleContext && interactionContext_.hasFilletHandleContext()) {
        if (interactionContext_.handleFilletRadiusHandleMouseDown)
            interactionContext_.handleFilletRadiusHandleMouseDown(x, y);
        if (interactionContext_.selectionMode()
            == SelectionWindowState::FilletRadiusHandleDrag) {
            return;
        }
    }
    if (interactionContext_.isVectorTwoPointDialogActive
        && interactionContext_.isVectorTwoPointDialogActive()) {
        const MouseInteractionContext::SelectionMode mode = interactionContext_.selectionMode();
        if (mode == SelectionWindowState::VectorTwoPointInteractive
            || mode == SelectionWindowState::VectorTwoPointHandleDrag) {
            if (interactionContext_.handleVectorTwoPointHandleMouseDown)
                interactionContext_.handleVectorTwoPointHandleMouseDown(x, y);
            if (interactionContext_.selectionMode()
                == SelectionWindowState::VectorTwoPointHandleDrag) {
                return;
            }
        }
    }

    if (isExtrusionSelection || isVectorDialogSelection || isSketchEditSelection) {
        // 拉伸/矢量拾取模式：左键只用于“点击确认”，不再承担旋转/平移
        // 选择逻辑将在 OnLeftButtonUp 中处理（如果是点击而不是拖拽）
        if (isSketchEditSelection) {
            if (interactionContext_.beginSketchBrushStroke)
                interactionContext_.beginSketchBrushStroke();
        }
    } else if (isCuboidInteractive) {
        if (interactionContext_.handleCuboidInteractiveMouseDown)
            interactionContext_.handleCuboidInteractiveMouseDown(x, y);
    } else if (mode == SelectionWindowState::PointSelection
               && interactionContext_.isCuboidInteractivePointSelectionActive
               && interactionContext_.isCuboidInteractivePointSelectionActive()) {
        // 块：点选模式下优先尝试尺寸/原点手柄，未命中则继续拾取原点点
        if (interactionContext_.handleCuboidInteractiveMouseDown)
            interactionContext_.handleCuboidInteractiveMouseDown(x, y);
        const bool cuboidDragActive = interactionContext_.isCuboidDragActive
            && interactionContext_.isCuboidDragActive();
        if (!cuboidDragActive
            && interactionContext_.handleVtkMouseClick) {
            interactionContext_.handleVtkMouseClick(x, y);
        }
    } else if (isPatternPitchInteractive) {
        if (interactionContext_.handlePatternPitchMouseDown)
            interactionContext_.handlePatternPitchMouseDown(x, y);
    } else {
        // 正常模式：左键仅用于拾取/选择，不再触发旋转
        if (interactionContext_.handleVtkMouseClick)
            interactionContext_.handleVtkMouseClick(x, y);
    }
    Q_UNUSED(isExtrusionHandle);
    Q_UNUSED(isRevolveHandle);
}

void MouseInteractorStyle::OnLeftButtonUp()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.selectionMode || !this->Interactor) {
        return;
    }

    // 获取当前选择模式
    const MouseInteractionContext::SelectionMode mode = interactionContext_.selectionMode();

    // 检查是否处于拉伸选择模式
    const bool isExtrusionSelection = (mode == SelectionWindowState::ExtrusionSelection
                                       || mode == SelectionWindowState::FaceSelection
                                       || mode == SelectionWindowState::EdgeSelection);
    const bool isVectorDialogSelection = (mode == SelectionWindowState::VectorDialogPickDirection
                                          || mode == SelectionWindowState::VectorDialogPickStartPoint
                                          || mode == SelectionWindowState::VectorDialogPickEndPoint);
    bool isSketchEditSelection =
        (mode == SelectionWindowState::SketchQuickTrim
         || mode == SelectionWindowState::SketchQuickExtend);

    if (mode == SelectionWindowState::SketchConicDragControl) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleSketchConicDragMouseUp)
            interactionContext_.handleSketchConicDragMouseUp(x, y);
    }

    if (mode == SelectionWindowState::SketchEllipseAdjust) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleSketchEllipseAdjustMouseUp)
            interactionContext_.handleSketchEllipseAdjustMouseUp(x, y);
    }

    if (mode == SelectionWindowState::CuboidInteractive
        || (mode == SelectionWindowState::PointSelection
            && interactionContext_.isCuboidInteractivePointSelectionActive
            && interactionContext_.isCuboidInteractivePointSelectionActive()
            && interactionContext_.isCuboidDragActive
            && interactionContext_.isCuboidDragActive())) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleCuboidInteractiveMouseUp)
            interactionContext_.handleCuboidInteractiveMouseUp(x, y);
    }

    if (mode == SelectionWindowState::PatternPitchInteractive) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handlePatternPitchMouseUp)
            interactionContext_.handlePatternPitchMouseUp(x, y);
    }

    if (mode == SelectionWindowState::ExtrusionHandleDrag) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleExtrusionHandleMouseUp)
            interactionContext_.handleExtrusionHandleMouseUp(x, y);
        mouseMoved = false;
        return;
    }

    if (mode == SelectionWindowState::RevolveHandleDrag) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleRevolveHandleMouseUp)
            interactionContext_.handleRevolveHandleMouseUp(x, y);
        mouseMoved = false;
        return;
    }

    if (mode == SelectionWindowState::ChamferAsymHandleDrag) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleChamferAsymHandleMouseUp)
            interactionContext_.handleChamferAsymHandleMouseUp(x, y);
        mouseMoved = false;
        return;
    }

    if (mode == SelectionWindowState::FilletRadiusHandleDrag) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleFilletRadiusHandleMouseUp)
            interactionContext_.handleFilletRadiusHandleMouseUp(x, y);
        mouseMoved = false;
        return;
    }

    if (mode == SelectionWindowState::VectorTwoPointHandleDrag) {
        int x = this->Interactor->GetEventPosition()[0];
        int y = this->Interactor->GetEventPosition()[1];
        if (interactionContext_.handleVectorTwoPointHandleMouseUp)
            interactionContext_.handleVectorTwoPointHandleMouseUp(x, y);
        mouseMoved = false;
        return;
    }

    if (isExtrusionSelection || isVectorDialogSelection || isSketchEditSelection) {
        // 在拉伸/矢量拾取模式下，如果鼠标没有移动（说明是点击而不是拖拽），处理选择逻辑
        if (!mouseMoved) {
            int x = this->Interactor->GetEventPosition()[0];
            int y = this->Interactor->GetEventPosition()[1];
            if (interactionContext_.handleVtkMouseClick)
                interactionContext_.handleVtkMouseClick(x, y);
        }
        if (isSketchEditSelection) {
            if (interactionContext_.endSketchBrushStroke)
                interactionContext_.endSketchBrushStroke();
        }
    }

    // 重置标志
    mouseMoved = false;
}

void MouseInteractorStyle::OnLeftButtonDoubleClick()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.handleVectorTwoPointArrowDoubleClick || !this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnLeftButtonDoubleClick();
        return;
    }

    const int x = this->Interactor->GetEventPosition()[0];
    const int y = this->Interactor->GetEventPosition()[1];
    interactionContext_.handleVectorTwoPointArrowDoubleClick(x, y);
}

void MouseInteractorStyle::OnRightButtonDown()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.selectionMode || !this->Interactor) {
        return;
    }

    // 右键点击/拖拽判定：在右键按下时重置一次，避免沿用左键/中键拖拽留下的 mouseMoved=true
    rightDownX_ = this->Interactor->GetEventPosition()[0];
    rightDownY_ = this->Interactor->GetEventPosition()[1];
    lastMouseX = rightDownX_;
    lastMouseY = rightDownY_;
    navLastX_ = rightDownX_;
    navLastY_ = rightDownY_;
    mouseMoved = false;

    rightDown_ = true;
    rightMenuPending_ = true;

    // 如果此时中键已按下，则进入“中键+右键”平移模式（右键不弹菜单）
    if (middleDown_) {
        if (interactionContext_.stopViewTransitionAnimation)
            interactionContext_.stopViewTransitionAnimation();
        rightMenuPending_ = false;
        if (navState_ == NavState::Rotating) {
            this->EndRotate();
        }
        if (navState_ != NavState::Panning) {
            this->StartPan();
            navState_ = NavState::Panning;
        }
        return;
    }

    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();

    // 不调用基类方法，避免默认右键缩放；右键菜单改为“点击抬起时”弹出
}

void MouseInteractorStyle::OnRightButtonUp()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.selectionMode || !this->Interactor) {
        return;
    }

    const int upX = this->Interactor->GetEventPosition()[0];
    const int upY = this->Interactor->GetEventPosition()[1];
    const int rdx = (rightDownX_ >= 0) ? (upX - rightDownX_) : 0;
    const int rdy = (rightDownY_ >= 0) ? (upY - rightDownY_) : 0;
    const bool rightDragged = (rdx * rdx + rdy * rdy) > 25; // >5px 认为是拖拽（比全局 mouseMoved 更宽松）

    // 右键抬起：如果没有发生拖拽且没有进入（中键+右键）平移，则弹出菜单
    if (rightMenuPending_ && !rightDragged && !middleDown_ && navState_ != NavState::Panning) {
        const int modelIndex = interactionContext_.pickModelAtPosition
            ? interactionContext_.pickModelAtPosition(upX, upY)
            : -1;
        QPoint globalPos = QCursor::pos();
        if (interactionContext_.showContextMenu)
            interactionContext_.showContextMenu(upX, upY, modelIndex, globalPos);
    }

    rightDown_ = false;
    rightMenuPending_ = false;
    rightDownX_ = -1;
    rightDownY_ = -1;

    // 如果当前处于平移状态且中键仍按下，且 Shift 未按下，则回到“中键旋转”
    if (navState_ == NavState::Panning && middleDown_ && !this->Interactor->GetShiftKey()) {
        this->EndPan();
        this->StartRotate();
        navState_ = NavState::Rotating;
        navLastX_ = upX;
        navLastY_ = upY;
    }

    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();
}

void MouseInteractorStyle::OnMiddleButtonDown()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.selectionMode || !this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
        return;
    }

    if (interactionContext_.stopViewTransitionAnimation)
        interactionContext_.stopViewTransitionAnimation();

    // 中键旋转/平移开始时，也重置点击/拖拽判定
    lastMouseX = this->Interactor->GetEventPosition()[0];
    lastMouseY = this->Interactor->GetEventPosition()[1];
    navLastX_ = lastMouseX;
    navLastY_ = lastMouseY;
    mouseMoved = false;

    middleDown_ = true;

    // Shift + 中键：平移；中键单独：旋转
    // 另外：如果右键也按着（中键+右键），也强制平移
    const bool wantPan = this->Interactor->GetShiftKey() || rightDown_;
    if (wantPan) {
        if (navState_ == NavState::Rotating) {
            this->EndRotate();
        }
        this->StartPan();
        navState_ = NavState::Panning;
    } else {
        if (navState_ == NavState::Panning) {
            this->EndPan();
        }
        this->StartRotate();
        navState_ = NavState::Rotating;
    }

    // 中键按下开始交互时，不再允许右键菜单弹出
    rightMenuPending_ = false;
}

void MouseInteractorStyle::OnMiddleButtonUp()
{
    if (preEventHook_) preEventHook_();
    if (!this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnMiddleButtonUp();
        return;
    }

    middleDown_ = false;

    if (navState_ == NavState::Rotating) {
        this->EndRotate();
        navState_ = NavState::None;
    } else if (navState_ == NavState::Panning) {
        this->EndPan();
        navState_ = NavState::None;
    }
    navLastX_ = -1;
    navLastY_ = -1;

    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();
    if (interactionContext_.refreshOverlayScreenScale)
        interactionContext_.refreshOverlayScreenScale();
}

void MouseInteractorStyle::OnMouseMove()
{
    if (preEventHook_) preEventHook_();
    if (!this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnMouseMove();
        return;
    }

    // 检查鼠标是否移动（用于区分点击和拖拽）
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    if (lastMouseX >= 0 && lastMouseY >= 0) {
        int dx = x - lastMouseX;
        int dy = y - lastMouseY;
        if (dx * dx + dy * dy > 4) { // 移动距离超过2像素，认为是拖拽
            mouseMoved = true;
        }
    }

    // 如果右键处于“等待弹菜单”的状态，但发生了拖拽，就取消菜单
    if (rightMenuPending_ && mouseMoved) {
        rightMenuPending_ = false;
    }

    // 始终更新三重轴预选（面/边 hover）
    if (interactionContext_.updateCenterTriadHover)
        interactionContext_.updateCenterTriadHover(x, y);

    if (navState_ == NavState::Rotating || navState_ == NavState::Panning) {
        if (interactionContext_.clearModelHoverHighlight)
            interactionContext_.clearModelHoverHighlight();
        if (navState_ == NavState::Rotating) {
            rotateCameraAroundWorldCenter(navLastX_, navLastY_, x, y);
        } else {
            vtkInteractorStyleTrackballCamera::OnMouseMove();
        }
        navLastX_ = x;
        navLastY_ = y;
        if (interactionContext_.refreshCameraClippingRange)
            interactionContext_.refreshCameraClippingRange();
        if (interactionContext_.syncCenterAxisCamera)
            interactionContext_.syncCenterAxisCamera();
        if (interactionContext_.refreshOverlayScreenScale)
            interactionContext_.refreshOverlayScreenScale();
        if (this->Interactor->GetRenderWindow()) {
            this->Interactor->GetRenderWindow()->Render();
        }
        return;
    }

    // 如果处于点选择/拉伸选择/倒圆角边选择模式/草图拾取与绘制，或捕捉点已启用，处理悬停提示
    const auto mode = interactionContext_.selectionMode
        ? interactionContext_.selectionMode()
        : SelectionWindowState::None;
    const bool featureDialogActive =
        (interactionContext_.hasExtrusionHandleContext
         && interactionContext_.hasExtrusionHandleContext())
        || (interactionContext_.hasRevolveHandleContext
            && interactionContext_.hasRevolveHandleContext());
    const bool snapActive = interactionContext_.snapArmed
        && interactionContext_.snapArmed();
    if (isHoverInteractionMode(mode) || featureDialogActive || snapActive) {
        if (interactionContext_.handleVtkMouseMove)
            interactionContext_.handleVtkMouseMove(x, y);
        // 仅当用户用“中键旋转/平移”在拖拽时，才驱动相机变化
        if (navState_ == NavState::Rotating || navState_ == NavState::Panning) {
            vtkInteractorStyleTrackballCamera::OnMouseMove();
            if (interactionContext_.refreshCameraClippingRange)
                interactionContext_.refreshCameraClippingRange();
            if (interactionContext_.syncCenterAxisCamera)
                interactionContext_.syncCenterAxisCamera();
            if (interactionContext_.refreshOverlayScreenScale)
                interactionContext_.refreshOverlayScreenScale();
        }
        return;
    }

    // 如果处于布尔运算选择模式，阻止相机旋转
    if (interactionContext_.isInSelectionMode
        && interactionContext_.isInSelectionMode()
        && navState_ == NavState::None) {
        // 在选择模式下，只更新鼠标位置，不旋转相机
        this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
        return;
    }

    if (mode == SelectionWindowState::None && interactionContext_.handleVtkMouseMove)
        interactionContext_.handleVtkMouseMove(x, y);

    // 相机在拖拽时可能发生变化，这里也同步一次中心轴相机与叠加物缩放
    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();
}

void MouseInteractorStyle::OnMouseWheelForward()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.renderer || !this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
        return;
    }

    if (interactionContext_.stopViewTransitionAnimation)
        interactionContext_.stopViewTransitionAnimation();

    // 强制只对主渲染器进行缩放（无论鼠标是否在中心轴 viewport 内）
    vtkRenderer* mainRenderer = interactionContext_.renderer();
    if (mainRenderer) {
        vtkCamera* cam = mainRenderer->GetActiveCamera();
        if (cam) {
            const double factor = 1.1;
            if (cam->GetParallelProjection()) {
                // 正交：改 ParallelScale；透视：Dolly 改变视距
                cam->SetParallelScale(cam->GetParallelScale() / factor);
            } else {
                cam->Dolly(factor);
            }
            if (interactionContext_.refreshCameraClippingRange)
                interactionContext_.refreshCameraClippingRange();
        }
    }

    // 同步中心轴相机方向（仅方向/投影视锥，大小保持固定）
    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();
    if (interactionContext_.refreshOverlayScreenScale)
        interactionContext_.refreshOverlayScreenScale();
    this->Interactor->GetRenderWindow()->Render();
}

void MouseInteractorStyle::OnMouseWheelBackward()
{
    if (preEventHook_) preEventHook_();
    if (!interactionContext_.renderer || !this->Interactor) {
        vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
        return;
    }

    if (interactionContext_.stopViewTransitionAnimation)
        interactionContext_.stopViewTransitionAnimation();

    // 强制只对主渲染器进行缩放（无论鼠标是否在中心轴 viewport 内）
    vtkRenderer* mainRenderer = interactionContext_.renderer();
    if (mainRenderer) {
        vtkCamera* cam = mainRenderer->GetActiveCamera();
        if (cam) {
            const double factor = 1.0 / 1.1;
            if (cam->GetParallelProjection()) {
                cam->SetParallelScale(cam->GetParallelScale() / factor);
            } else {
                cam->Dolly(factor);
            }
            if (interactionContext_.refreshCameraClippingRange)
                interactionContext_.refreshCameraClippingRange();
        }
    }

    // 同步中心轴相机方向（仅方向/投影视锥，大小保持固定）
    if (interactionContext_.syncCenterAxisCamera)
        interactionContext_.syncCenterAxisCamera();
    if (interactionContext_.refreshOverlayScreenScale)
        interactionContext_.refreshOverlayScreenScale();
    this->Interactor->GetRenderWindow()->Render();
}
