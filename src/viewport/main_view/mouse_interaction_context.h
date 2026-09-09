#ifndef VIEWPORT_MAIN_VIEW_MOUSE_INTERACTION_CONTEXT_H
#define VIEWPORT_MAIN_VIEW_MOUSE_INTERACTION_CONTEXT_H

#include "interaction/selection/selection_window_state.h"

#include <QPoint>

#include <functional>

class vtkRenderer;

/**
 * Narrow callback boundary between the VTK mouse router and the window shell.
 * The router owns mouse-state policy; the host owns feature and UI behavior.
 */
struct MouseInteractionContext {
    using SelectionMode = SelectionWindowState::SelectionMode;

    std::function<vtkRenderer*()> renderer;
    std::function<SelectionMode()> selectionMode;
    std::function<bool()> isInSelectionMode;
    std::function<bool()> hasExtrusionHandleContext;
    std::function<bool()> hasRevolveHandleContext;
    std::function<bool()> hasChamferHandleContext;
    std::function<bool()> hasFilletHandleContext;
    std::function<bool()> isVectorTwoPointDialogActive;
    std::function<bool()> isCuboidInteractivePointSelectionActive;
    std::function<bool()> isCuboidDragActive;
    std::function<bool()> snapArmed;

    std::function<void()> syncCenterAxisCamera;
    std::function<void()> stopViewTransitionAnimation;
    std::function<void()> refreshCameraClippingRange;
    std::function<void()> refreshOverlayScreenScale;
    std::function<void(int, int)> updateCenterTriadHover;
    std::function<void()> clearModelHoverHighlight;
    std::function<void(int, int)> handleVtkMouseClick;
    std::function<void(int, int)> handleVtkMouseMove;
    std::function<void(int, int)> handleExtrusionHandleMouseDown;
    std::function<void(int, int)> handleExtrusionHandleMouseUp;
    std::function<void(int, int)> handleRevolveHandleMouseDown;
    std::function<void(int, int)> handleRevolveHandleMouseUp;
    std::function<void(int, int)> handleChamferAsymHandleMouseDown;
    std::function<void(int, int)> handleChamferAsymHandleMouseUp;
    std::function<void(int, int)> handleFilletRadiusHandleMouseDown;
    std::function<void(int, int)> handleFilletRadiusHandleMouseUp;
    std::function<void(int, int)> handleVectorTwoPointHandleMouseDown;
    std::function<void(int, int)> handleVectorTwoPointHandleMouseUp;
    std::function<void(int, int)> handleSketchConicDragMouseUp;
    std::function<void(int, int)> handleSketchEllipseAdjustMouseUp;
    std::function<void(int, int)> handleCuboidInteractiveMouseDown;
    std::function<void(int, int)> handleCuboidInteractiveMouseUp;
    std::function<void(int, int)> handlePatternPitchMouseDown;
    std::function<void(int, int)> handlePatternPitchMouseUp;
    std::function<void()> beginSketchBrushStroke;
    std::function<void()> endSketchBrushStroke;
    std::function<void(int, int)> handleVectorTwoPointArrowDoubleClick;
    std::function<int(int, int)> pickModelAtPosition;
    std::function<void(int, int, int, const QPoint&)> showContextMenu;
};

#endif // VIEWPORT_MAIN_VIEW_MOUSE_INTERACTION_CONTEXT_H
