#ifndef MOUSE_INTERACTOR_H
#define MOUSE_INTERACTOR_H

#include <functional>

#include <vtkInteractorStyleTrackballCamera.h>

class Widget;

class MouseInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static MouseInteractorStyle* New();
    vtkTypeMacro(MouseInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetWidget(Widget* widget)
    {
        this->widget = widget;
    }

    void SetPreEventHook(const std::function<void()>& hook)
    {
        preEventHook_ = hook;
    }

    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnLeftButtonDoubleClick() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnMouseMove() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

private:
    void rotateCameraAroundWorldCenter(int prevX, int prevY, int x, int y);

    Widget* widget = nullptr;
    std::function<void()> preEventHook_;

    enum class NavState {
        None = 0,
        Rotating,
        Panning
    };

    NavState navState_ = NavState::None;
    bool middleDown_ = false;
    bool rightDown_ = false;
    bool rightMenuPending_ = false;
    int rightDownX_ = -1;
    int rightDownY_ = -1;
    int navLastX_ = -1;
    int navLastY_ = -1;
};

void resetVtkMouseDragTracking();

#endif // MOUSE_INTERACTOR_H
