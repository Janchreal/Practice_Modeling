#include "mirror_view_state.h"
#include "mirror_view_window.h"

QHash<const QObject*, QList<QPointer<MirrorRenderWindow>>> g_mirrorWindowMap;
int g_mirrorWindowCounter = 1;
QHash<const QObject*, QVTKOpenGLNativeWidget*> g_mainVtkWidgetMap;
QHash<const QObject*, vtkSmartPointer<vtkRenderer>> g_mainRendererMap;
QHash<const QObject*, vtkSmartPointer<IVtkTools_ShapePicker>> g_mainPickerMap;
QHash<const QObject*, QHash<QObject*, MirrorRenderContext>> g_mirrorRenderContextMap;
QHash<const QObject*, QPointer<QMainWindow>> g_activeStatusWindowMap;
