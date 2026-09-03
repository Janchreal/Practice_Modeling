#ifndef MIRROR_VIEW_WINDOW_H
#define MIRROR_VIEW_WINDOW_H

#include <QMainWindow>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkSmartPointer.h>

class vtkRenderer;

// 独立建模视图子窗口（与主 Widget 镜像同步）
class MirrorRenderWindow : public QMainWindow
{
public:
    explicit MirrorRenderWindow(QWidget* parent = nullptr);

    QVTKOpenGLNativeWidget* vtkWidget = nullptr;
    vtkSmartPointer<vtkRenderer> renderer;
};

#endif
