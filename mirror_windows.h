#ifndef MIRROR_WINDOWS_H
#define MIRROR_WINDOWS_H

#include "modelinghistory.h"

#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>

#include <IVtkTools_ShapePicker.hxx>

#include <vtkActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class Widget;

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

class MirrorRenderWindow : public QMainWindow {
public:
    explicit MirrorRenderWindow(QWidget* parent = nullptr);

    QVTKOpenGLNativeWidget* vtkWidget = nullptr;
    vtkSmartPointer<vtkRenderer> renderer;
};

namespace MirrorWindows {

void cleanup(Widget* w);

void setActiveHost(Widget* w, QMainWindow* host);
QMainWindow* activeHost(const Widget* w, Widget* selfFallback);

QWidget* dialogParent(Widget* owner);

void setMainBindings(Widget* w, QVTKOpenGLNativeWidget* vtk,
                     const vtkSmartPointer<vtkRenderer>& renderer,
                     const vtkSmartPointer<IVtkTools_ShapePicker>& picker);

bool getMainBindings(const Widget* w, QVTKOpenGLNativeWidget*& outVtk,
                     vtkSmartPointer<vtkRenderer>& outRenderer,
                     vtkSmartPointer<IVtkTools_ShapePicker>& outPicker);

QVTKOpenGLNativeWidget* mainVtkWidget(const Widget* w);

bool hasRenderContexts(const Widget* w);
QHash<QObject*, MirrorRenderContext>& renderContexts(Widget* w);

bool hasMirrorWindows(const Widget* w);
QList<QPointer<MirrorRenderWindow>>* mirrorWindowsPtr(Widget* w);
void appendMirrorWindow(Widget* w, MirrorRenderWindow* win);
void removeMirrorWindowFromList(Widget* w, MirrorRenderWindow* win);

int bumpMirrorWindowCounter();

QObject* mirrorContextKeyForCurrentVtk(Widget* w, QVTKOpenGLNativeWidget* vtkWidget);

MirrorRenderContext* contextForVtk(const Widget* owner, QVTKOpenGLNativeWidget* currentWidget);

QObject* mirrorKeyForVtkObject(Widget* w, QObject* obj);

vtkSmartPointer<vtkActor> buildMirrorActorFromHistory(const ModelingHistory& record);

} // namespace MirrorWindows

#endif
