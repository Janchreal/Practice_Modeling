#include "widget_mirror_globals.h"
#include "widget_mirror_window.h"

#include "widget.h"

QHash<const Widget*, QList<QPointer<MirrorRenderWindow>>> g_mirrorWindowMap;
int g_mirrorWindowCounter = 1;
QHash<const Widget*, QVTKOpenGLNativeWidget*> g_mainVtkWidgetMap;
QHash<const Widget*, vtkSmartPointer<vtkRenderer>> g_mainRendererMap;
QHash<const Widget*, vtkSmartPointer<IVtkTools_ShapePicker>> g_mainPickerMap;
QHash<const Widget*, QHash<QObject*, MirrorRenderContext>> g_mirrorRenderContextMap;
QHash<const Widget*, QPointer<QMainWindow>> g_activeStatusWindowMap;
