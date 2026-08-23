#include "myvtkwidget.h"
#include <vtkPropPicker.h>
#include <QDebug>

MyVTKWidget::MyVTKWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
{
    renderWindowVTK = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    this->setRenderWindow(renderWindowVTK);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.15, 0.2, 0.3);
    renderWindowVTK->AddRenderer(renderer);
}

void MyVTKWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        double dpr = devicePixelRatioF();
        int x = static_cast<int>(event->position().x() * dpr);
        int y = static_cast<int>((height() - event->position().y()) * dpr);

        vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
        picker->Pick(x, y, 0, renderer);

        vtkActor* pickedActor = picker->GetActor();
        emit actorPicked(pickedActor); // 发信号给 Widget
    }

    QVTKOpenGLNativeWidget::mousePressEvent(event);
}
