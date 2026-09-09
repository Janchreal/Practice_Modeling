#ifndef MIRROR_VIEW_STATE_H
#define MIRROR_VIEW_STATE_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QMainWindow>

#include "mirror_view_types.h"

#include <vtkSmartPointer.h>

#include <IVtkTools_ShapePicker.hxx>

class QVTKOpenGLNativeWidget;
class vtkRenderer;
class MirrorRenderWindow;

// 多窗口镜像与主视图绑定（原 main_window.cpp 内 static，供 main_window_vtk_view / 镜像逻辑共用）
// 键只表示注册表宿主，不要求宿主必须是主窗口类型。
extern QHash<const QObject*, QList<QPointer<MirrorRenderWindow>>> g_mirrorWindowMap;
extern int g_mirrorWindowCounter;
extern QHash<const QObject*, QVTKOpenGLNativeWidget*> g_mainVtkWidgetMap;
extern QHash<const QObject*, vtkSmartPointer<vtkRenderer>> g_mainRendererMap;
extern QHash<const QObject*, vtkSmartPointer<IVtkTools_ShapePicker>> g_mainPickerMap;
extern QHash<const QObject*, QHash<QObject*, MirrorRenderContext>> g_mirrorRenderContextMap;
extern QHash<const QObject*, QPointer<QMainWindow>> g_activeStatusWindowMap;

#endif
