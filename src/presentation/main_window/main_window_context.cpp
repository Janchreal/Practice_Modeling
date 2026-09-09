#include "main_window.h"

#include "viewport/mirror/mirror_view_types.h"
#include "viewport/mirror/mirror_view_state.h"

#include <QVTKOpenGLNativeWidget.h>

static MirrorRenderContext* currentMirrorContextForWidget(const Widget* owner, QVTKOpenGLNativeWidget* currentWidget)
{
    if (!owner || !currentWidget || !g_mirrorRenderContextMap.contains(owner)) return nullptr;
    auto& map = g_mirrorRenderContextMap[owner];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().vtkWidget == currentWidget) {
            return &it.value();
        }
    }
    return nullptr;
}

MirrorRenderContext* Widget::mirrorContextForVtkWidget(QVTKOpenGLNativeWidget* w)
{
    return currentMirrorContextForWidget(this, w);
}

QWidget* Widget::dialogParentWidget() const
{
    QMainWindow* host = g_activeStatusWindowMap.value(this, const_cast<Widget*>(this));
    if (host) return host;
    return const_cast<Widget*>(this);
}
