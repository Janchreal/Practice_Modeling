#include "mirror_windows.h"
#include "widget.h"

#include <QVBoxLayout>

#include <BRepMesh_IncrementalMesh.hxx>
#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>

#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace {

QHash<const Widget*, QList<QPointer<MirrorRenderWindow>>> g_mirrorWindowMap;
int g_mirrorWindowCounter = 1;
QHash<const Widget*, QVTKOpenGLNativeWidget*> g_mainVtkWidgetMap;
QHash<const Widget*, vtkSmartPointer<vtkRenderer>> g_mainRendererMap;
QHash<const Widget*, vtkSmartPointer<IVtkTools_ShapePicker>> g_mainPickerMap;
QHash<const Widget*, QHash<QObject*, MirrorRenderContext>> g_mirrorRenderContextMap;
QHash<const Widget*, QPointer<QMainWindow>> g_activeStatusWindowMap;

const Widget* widgetKey(Widget* w) { return w; }

} // namespace

MirrorRenderWindow::MirrorRenderWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QStringLiteral("建模视图 - 新窗口"));
    resize(960, 640);

    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    vtkWidget = new QVTKOpenGLNativeWidget(container);
    vtkWidget->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(vtkWidget);
    setCentralWidget(container);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkWidget->renderWindow()->AddRenderer(renderer);
    Widget::configureSceneLights(renderer);
    renderer->SetBackground(0.8, 0.8, 0.8);
    renderer->SetBackground2(0.9, 0.9, 0.9);
    renderer->GradientBackgroundOn();
    renderer->ResetCamera();
}

namespace MirrorWindows {

void cleanup(Widget* w)
{
    const Widget* k = widgetKey(w);
    g_mirrorWindowMap.remove(k);
    g_mirrorRenderContextMap.remove(k);
    g_mainVtkWidgetMap.remove(k);
    g_mainRendererMap.remove(k);
    g_mainPickerMap.remove(k);
    g_activeStatusWindowMap.remove(k);
}

void setActiveHost(Widget* w, QMainWindow* host)
{
    g_activeStatusWindowMap[widgetKey(w)] = host;
}

QMainWindow* activeHost(const Widget* w, Widget* selfFallback)
{
    const QPointer<QMainWindow> fallback(static_cast<QMainWindow*>(selfFallback));
    return g_activeStatusWindowMap.value(widgetKey(w), fallback).data();
}

QWidget* dialogParent(Widget* owner)
{
    if (!owner) return nullptr;
    QMainWindow* host = g_activeStatusWindowMap.value(widgetKey(owner), owner).data();
    if (host) return host;
    return owner;
}

void setMainBindings(Widget* w, QVTKOpenGLNativeWidget* vtk,
                     const vtkSmartPointer<vtkRenderer>& renderer,
                     const vtkSmartPointer<IVtkTools_ShapePicker>& picker)
{
    const Widget* k = widgetKey(w);
    g_mainVtkWidgetMap[k] = vtk;
    g_mainRendererMap[k] = renderer;
    g_mainPickerMap[k] = picker;
}

bool getMainBindings(const Widget* w, QVTKOpenGLNativeWidget*& outVtk,
                     vtkSmartPointer<vtkRenderer>& outRenderer,
                     vtkSmartPointer<IVtkTools_ShapePicker>& outPicker)
{
    if (!g_mainVtkWidgetMap.contains(w) || !g_mainRendererMap.contains(w) || !g_mainPickerMap.contains(w)) {
        return false;
    }
    outVtk = g_mainVtkWidgetMap.value(w);
    outRenderer = g_mainRendererMap.value(w);
    outPicker = g_mainPickerMap.value(w);
    return true;
}

QVTKOpenGLNativeWidget* mainVtkWidget(const Widget* w)
{
    return g_mainVtkWidgetMap.value(w, nullptr);
}

bool hasRenderContexts(const Widget* w)
{
    return g_mirrorRenderContextMap.contains(w);
}

QHash<QObject*, MirrorRenderContext>& renderContexts(Widget* w)
{
    return g_mirrorRenderContextMap[widgetKey(w)];
}

bool hasMirrorWindows(const Widget* w)
{
    return g_mirrorWindowMap.contains(w);
}

QList<QPointer<MirrorRenderWindow>>* mirrorWindowsPtr(Widget* w)
{
    const Widget* k = widgetKey(w);
    auto it = g_mirrorWindowMap.find(k);
    if (it == g_mirrorWindowMap.end()) {
        return nullptr;
    }
    return &(*it);
}

void appendMirrorWindow(Widget* w, MirrorRenderWindow* win)
{
    g_mirrorWindowMap[widgetKey(w)].append(QPointer<MirrorRenderWindow>(win));
}

void removeMirrorWindowFromList(Widget* w, MirrorRenderWindow* win)
{
    const Widget* k = widgetKey(w);
    if (!g_mirrorWindowMap.contains(k)) {
        return;
    }
    g_mirrorWindowMap[k].removeAll(QPointer<MirrorRenderWindow>(win));
}

int bumpMirrorWindowCounter()
{
    return g_mirrorWindowCounter++;
}

QObject* mirrorContextKeyForCurrentVtk(Widget* w, QVTKOpenGLNativeWidget* vtkWidget)
{
    if (!w || !vtkWidget || !g_mirrorRenderContextMap.contains(widgetKey(w))) {
        return nullptr;
    }
    auto& map = g_mirrorRenderContextMap[widgetKey(w)];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().vtkWidget == vtkWidget) {
            return it.key();
        }
    }
    return nullptr;
}

MirrorRenderContext* contextForVtk(const Widget* owner, QVTKOpenGLNativeWidget* currentWidget)
{
    if (!owner || !currentWidget || !g_mirrorRenderContextMap.contains(owner)) {
        return nullptr;
    }
    auto& map = g_mirrorRenderContextMap[owner];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().vtkWidget == currentWidget) {
            return &it.value();
        }
    }
    return nullptr;
}

QObject* mirrorKeyForVtkObject(Widget* w, QObject* obj)
{
    if (!w || !obj || !g_mirrorRenderContextMap.contains(widgetKey(w))) {
        return nullptr;
    }
    auto& map = g_mirrorRenderContextMap[widgetKey(w)];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().vtkWidget == obj) {
            return it.key();
        }
    }
    return nullptr;
}

vtkSmartPointer<vtkActor> buildMirrorActorFromHistory(const ModelingHistory& record)
{
    vtkSmartPointer<vtkPolyData> renderData = nullptr;

    if (!record.occShape.IsNull()) {
        try {
            BRepMesh_IncrementalMesh mesh(record.occShape, 0.08, Standard_False, 0.3, Standard_True);
            mesh.Perform();

            Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(record.occShape);
            Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
            IVtkOCC_ShapeMesher mesher;
            mesher.Build(shapeWrapper, shapeData);
            vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
            if (meshPolyData) {
                renderData = vtkSmartPointer<vtkPolyData>::New();
                renderData->ShallowCopy(meshPolyData);
            }
        } catch (...) {
            renderData = nullptr;
        }
    }

    if (!renderData && record.polyData) {
        renderData = vtkSmartPointer<vtkPolyData>::New();
        renderData->ShallowCopy(record.polyData);
    }

    if (!renderData) {
        return nullptr;
    }

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(renderData);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (record.actor) {
        const double* c = record.actor->GetProperty()->GetColor();
        actor->GetProperty()->SetColor(c[0], c[1], c[2]);
        actor->GetProperty()->SetOpacity(record.actor->GetProperty()->GetOpacity());
        actor->GetProperty()->SetInterpolation(record.actor->GetProperty()->GetInterpolation());
        actor->GetProperty()->SetSpecular(record.actor->GetProperty()->GetSpecular());
        actor->GetProperty()->SetSpecularPower(record.actor->GetProperty()->GetSpecularPower());
        actor->GetProperty()->SetAmbient(record.actor->GetProperty()->GetAmbient());
        actor->GetProperty()->SetDiffuse(record.actor->GetProperty()->GetDiffuse());
        actor->GetProperty()->SetEdgeVisibility(record.actor->GetProperty()->GetEdgeVisibility());
    } else {
        actor->GetProperty()->SetColor(record.color.redF(), record.color.greenF(), record.color.blueF());
        Widget::applySolidActorMaterial(actor->GetProperty());
    }

    const bool visible = (record.actor != nullptr) ? (record.actor->GetVisibility() != 0) : true;
    actor->SetVisibility(visible ? 1 : 0);
    return actor;
}

} // namespace MirrorWindows
