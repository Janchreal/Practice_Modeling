#ifndef WIDGET_MIRROR_TYPES_H
#define WIDGET_MIRROR_TYPES_H

#include <QHash>
#include <QList>
#include <QPointer>

#include <vtkSmartPointer.h>
#include <vtkActor.h>

class QVTKOpenGLNativeWidget;
class vtkRenderer;
class vtkOrientationMarkerWidget;

#include <IVtkTools_ShapePicker.hxx>

// 额外建模视图窗口的渲染上下文（与 Widget 主视图镜像同步）
struct MirrorRenderContext {
    QPointer<QVTKOpenGLNativeWidget> vtkWidget;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<IVtkTools_ShapePicker> shapePicker;
    vtkSmartPointer<vtkOrientationMarkerWidget> triadWidget;
    vtkSmartPointer<vtkRenderer> centerAxesRenderer;
    vtkSmartPointer<vtkActor> centerAxisXActor;
    vtkSmartPointer<vtkActor> centerAxisYActor;
    vtkSmartPointer<vtkActor> centerAxisZActor;
    vtkSmartPointer<vtkActor> centerTriadFaceActors[6];
    vtkSmartPointer<vtkActor> centerTriadEdgeActors[12];
    int centerTriadHoveredFace = -1;
    int centerTriadHoveredEdge = -1;
    int centerTriadCurrentFace = -1;
    QList<vtkSmartPointer<vtkActor>> modelActors;
    QHash<vtkActor*, int> modelActorToHistoryIndex;
};

#endif
