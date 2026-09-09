// Sketch tab paging helpers (split from main_window.cpp)
#include "main_window.h"
#include "rendering/model/model_display_style.h"
#include "ui_main_window.h"
#include "presentation/dialogs/sketch/sketch_create_dialog.h"
#include "presentation/dialogs/sketch/sketch_mode_dialogs.h"
#include "sketch_tool_input_dialog.h"
#include "sketch_conic_dialog.h"
#include "sketch_polygon_dialog.h"
#include "sketch_ellipse_dialog.h"
#include "application/commands/sketcheditcommand.h"
#include "geometry/sketch/sketch_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QDialog>
#include <QImage>
#include <QMessageBox>
#include <QObject>
#include <QPalette>
#include <QPushButton>
#include <QSignalBlocker>
#include <QPixmap>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolButton>
#include <Qt>

#include <BRep_Builder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_Ellipse.hxx>
#include <gp_Ax2.hxx>
#include <gp_Elips.hxx>
#include <Standard_Failure.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <gce_MakeCirc.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <gp_Lin.hxx>

#include <IVtkTools_ShapePicker.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtk_Types.hxx>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCellArray.h>
#include <vtkFeatureEdges.h>
#include <vtkLineSource.h>
#include <vtkMapper.h>
#include <vtkPlaneSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
namespace {

/** 将系统标准图标转为近似单色（使用调色板 ButtonText），便于侧栏小按钮与 NX 风格接近。 */
static QIcon monochromeStandardIcon(const QWidget* w, QStyle::StandardPixmap sp, const QSize& sz)
{
    if (!w || !w->style())
        return {};
    const QIcon src = w->style()->standardIcon(sp, nullptr, w);
    const QPixmap pm = src.pixmap(sz, QIcon::Normal, QIcon::Off);
    if (pm.isNull())
        return {};
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    const QColor ink = w->palette().color(QPalette::ButtonText);
    for (int y = 0; y < img.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb px = line[x];
            const int a0 = qAlpha(px);
            if (a0 == 0)
                continue;
            const int lum = qGray(px);
            const int a = qBound(30, (a0 * lum + 64) / 256 + 48, 255);
            line[x] = qRgba(ink.red(), ink.green(), ink.blue(), a);
        }
    }
    return QIcon(QPixmap::fromImage(img));
}

void cycleSketchStackedPage(QStackedWidget* sw, int delta)
{
    if (!sw)
        return;
    const int n = sw->count();
    if (n <= 0)
        return;
    sw->setCurrentIndex((sw->currentIndex() + delta + n) % n);
}

} // namespace
void Widget::wireSketchTabStackedPages()
{
    if (!ui->stackedWidget || !ui->stackedWidget_2)
        return;
    if (!ui->toolButton_sketchCurveStack_prev || !ui->toolButton_sketchCurveStack_next
        || !ui->toolButton_sketchEditStack_prev || !ui->toolButton_sketchEditStack_next)
        return;

    const QSize iconSz(14, 14);
    const QSize btnSz(20, 20);
    const QIcon icoPrev = monochromeStandardIcon(this, QStyle::SP_ArrowUp, iconSz);
    const QIcon icoNext = monochromeStandardIcon(this, QStyle::SP_ArrowDown, iconSz);

    ui->toolButton_sketchCurveStack_prev->setText(QString());
    ui->toolButton_sketchCurveStack_prev->setIcon(icoPrev);
    ui->toolButton_sketchCurveStack_prev->setIconSize(iconSz);
    ui->toolButton_sketchCurveStack_prev->setFixedSize(btnSz);
    ui->toolButton_sketchCurveStack_prev->setToolTip(tr("上一页"));
    ui->toolButton_sketchCurveStack_prev->setAutoRaise(true);

    ui->toolButton_sketchCurveStack_next->setText(QString());
    ui->toolButton_sketchCurveStack_next->setIcon(icoNext);
    ui->toolButton_sketchCurveStack_next->setIconSize(iconSz);
    ui->toolButton_sketchCurveStack_next->setFixedSize(btnSz);
    ui->toolButton_sketchCurveStack_next->setToolTip(tr("下一页"));
    ui->toolButton_sketchCurveStack_next->setAutoRaise(true);

    ui->toolButton_sketchEditStack_prev->setText(QString());
    ui->toolButton_sketchEditStack_prev->setIcon(icoPrev);
    ui->toolButton_sketchEditStack_prev->setIconSize(iconSz);
    ui->toolButton_sketchEditStack_prev->setFixedSize(btnSz);
    ui->toolButton_sketchEditStack_prev->setToolTip(tr("上一页"));
    ui->toolButton_sketchEditStack_prev->setAutoRaise(true);

    ui->toolButton_sketchEditStack_next->setText(QString());
    ui->toolButton_sketchEditStack_next->setIcon(icoNext);
    ui->toolButton_sketchEditStack_next->setIconSize(iconSz);
    ui->toolButton_sketchEditStack_next->setFixedSize(btnSz);
    ui->toolButton_sketchEditStack_next->setToolTip(tr("下一页"));
    ui->toolButton_sketchEditStack_next->setAutoRaise(true);

    connect(ui->toolButton_sketchCurveStack_prev, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget, -1);
    });
    connect(ui->toolButton_sketchCurveStack_next, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget, +1);
    });
    connect(ui->toolButton_sketchEditStack_prev, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget_2, -1);
    });
    connect(ui->toolButton_sketchEditStack_next, &QToolButton::clicked, this, [this]() {
        cycleSketchStackedPage(ui->stackedWidget_2, +1);
    });
}
