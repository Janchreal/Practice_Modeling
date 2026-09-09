#ifndef MIRROR_VIEW_STATE_H
#define MIRROR_VIEW_STATE_H

#include <QHash>
#include <QList>
#include <QPointer>
#include <QMainWindow>

#include "mirror_view_types.h"

#include <vtkSmartPointer.h>

#include <IVtkTools_ShapePicker.hxx>

class Widget;
class QVTKOpenGLNativeWidget;
class vtkRenderer;
class MirrorRenderWindow;

// 多窗口镜像与主视图绑定（原 main_window.cpp 内 static，供 main_window_vtk_view / 镜像逻辑共用）
extern QHash<const Widget*, QList<QPointer<MirrorRenderWindow>>> g_mirrorWindowMap;
extern int g_mirrorWindowCounter;
extern QHash<const Widget*, QVTKOpenGLNativeWidget*> g_mainVtkWidgetMap;
extern QHash<const Widget*, vtkSmartPointer<vtkRenderer>> g_mainRendererMap;
extern QHash<const Widget*, vtkSmartPointer<IVtkTools_ShapePicker>> g_mainPickerMap;
extern QHash<const Widget*, QHash<QObject*, MirrorRenderContext>> g_mirrorRenderContextMap;
extern QHash<const Widget*, QPointer<QMainWindow>> g_activeStatusWindowMap;

#endif
