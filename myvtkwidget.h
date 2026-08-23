#ifndef MYVTKWIDGET_H
#define MYVTKWIDGET_H

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkActor.h>
#include <QMouseEvent>

class MyVTKWidget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT
public:
    explicit MyVTKWidget(QWidget* parent = nullptr);

    vtkRenderer* getRenderer() { return renderer; }

signals:
    void actorPicked(vtkActor* actor);  // 鼠标点击 actor 信号

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowVTK;
    vtkSmartPointer<vtkRenderer> renderer;
};

#endif // MYVTKWIDGET_H
