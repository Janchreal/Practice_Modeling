#ifndef VIEWPORT_MAIN_VIEW_VIEW_NAVIGATION_WINDOW_STATE_H
#define VIEWPORT_MAIN_VIEW_VIEW_NAVIGATION_WINDOW_STATE_H

#include <QElapsedTimer>
#include <QTimer>

/** Runtime state for animated standard-view camera transitions. */
class ViewNavigationWindowState {
protected:
    struct ViewCameraPose {
        double pos[3] = {0.0, 0.0, 0.0};
        double fp[3] = {0.0, 0.0, 0.0};
        double up[3] = {0.0, 1.0, 0.0};
        double parallelScale = 1.0;
        bool parallel = false;
    };

    QTimer* viewAnimTimer_ = nullptr;
    QElapsedTimer viewAnimClock_;
    ViewCameraPose viewAnimFrom_{};
    ViewCameraPose viewAnimTo_{};
    bool viewTransitionAnimating_ = false;
    static constexpr int kViewAnimDurationMs_ = 380;
};

#endif // VIEWPORT_MAIN_VIEW_VIEW_NAVIGATION_WINDOW_STATE_H
