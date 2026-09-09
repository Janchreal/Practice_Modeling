#include "main_window.h"

#include "presentation/dialogs/sketch/sketch_ellipse_dialog.h"
#include "presentation/dialogs/sketch/sketch_mode_dialogs.h"
#include "presentation/dialogs/sketch/sketch_polygon_dialog.h"

#include <QDialog>
#include <QObject>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <Qt>

static void positionSketchAuxDialogImpl(QVTKOpenGLNativeWidget* vtkWidget, QDialog* dlg)
{
    if (!vtkWidget || !dlg) return;
    const QPoint g = vtkWidget->mapToGlobal(QPoint(0, 0));
    dlg->move(g.x() + vtkWidget->width() - dlg->width() - 8, g.y() + 8);
}

void Widget::positionSketchAuxDialog(QDialog* dlg)
{
    positionSketchAuxDialogImpl(vtkWidget, dlg);
}

void Widget::positionSketchToolInputDialog()
{
    if (!sketchToolInputDialog_ || !vtkWidget) return;
    const QPoint g = vtkWidget->mapToGlobal(QPoint(0, 0));
    sketchToolInputDialog_->move(g.x() + vtkWidget->width() - sketchToolInputDialog_->width() - 8,
                                 g.y() + 8);
}

void Widget::openOrRaiseSketchRectangleModeDialog()
{
    if (!sketchRectangleModeDialog_) {
        sketchRectangleModeDialog_ = new SketchRectangleModeDialog(this);
        sketchRectangleModeDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchRectangleModeDialog_, &SketchRectangleModeDialog::closedByUser, this, [this]() {
            sketchRectangleModeDialog_ = nullptr;
        });
        connect(sketchRectangleModeDialog_, &QObject::destroyed, this, [this]() {
            sketchRectangleModeDialog_ = nullptr;
        });
        connect(sketchRectangleModeDialog_, &SketchRectangleModeDialog::methodChanged, this,
                [this](SketchRectangleModeDialog::Method) {
                    sketchClickCount_ = 0;
                    clearSketchPreviewRectangle();
                    clearSketchPreviewLine();
                });
    }
    sketchRectangleModeDialog_->show();
    sketchRectangleModeDialog_->raise();
    positionSketchAuxDialogImpl(vtkWidget, sketchRectangleModeDialog_);
}

void Widget::closeSketchRectangleModeDialog()
{
    if (sketchRectangleModeDialog_) {
        sketchRectangleModeDialog_->close();
        sketchRectangleModeDialog_ = nullptr;
    }
}

void Widget::openOrRaiseSketchCircleModeDialog()
{
    if (!sketchCircleModeDialog_) {
        sketchCircleModeDialog_ = new SketchCircleModeDialog(this);
        sketchCircleModeDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchCircleModeDialog_, &SketchCircleModeDialog::closedByUser, this, [this]() {
            sketchCircleModeDialog_ = nullptr;
        });
        connect(sketchCircleModeDialog_, &QObject::destroyed, this, [this]() {
            sketchCircleModeDialog_ = nullptr;
        });
        connect(sketchCircleModeDialog_, &SketchCircleModeDialog::methodChanged, this,
                [this](SketchCircleModeDialog::Method) {
                    sketchClickCount_ = 0;
                    clearSketchPreviewCircle();
                    clearSketchPreviewLine();
                });
    }
    sketchCircleModeDialog_->show();
    sketchCircleModeDialog_->raise();
    positionSketchAuxDialogImpl(vtkWidget, sketchCircleModeDialog_);
}

void Widget::closeSketchCircleModeDialog()
{
    if (sketchCircleModeDialog_) {
        sketchCircleModeDialog_->close();
        sketchCircleModeDialog_ = nullptr;
    }
}

void Widget::openOrRaiseSketchPolygonValueDialog()
{
    if (!sketchPolygonValueDialog_) {
        sketchPolygonValueDialog_ = new SketchPolygonValueDialog(this);
        sketchPolygonValueDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchPolygonValueDialog_, &QObject::destroyed, this, [this]() {
            sketchPolygonValueDialog_ = nullptr;
        });
        connect(sketchPolygonValueDialog_, &SketchPolygonValueDialog::valuesCommitted, this,
                &Widget::sketchApplyPolygonManualInput);
        connect(sketchPolygonValueDialog_, &SketchPolygonValueDialog::closedByUser, this, [this]() {
            sketchPolygonValueDialog_ = nullptr;
        });
    }
    if (sketchPolygonDialog_) {
        sketchPolygonValueDialog_->setSizeMode(sketchPolygonDialog_->sizeMode());
    }
    sketchPolygonValueDialog_->show();
    sketchPolygonValueDialog_->raise();
}

void Widget::closeSketchPolygonValueDialog()
{
    if (sketchPolygonValueDialog_) {
        sketchPolygonValueDialog_->close();
        sketchPolygonValueDialog_ = nullptr;
    }
}

void Widget::positionSketchPolygonValueDialog(int screenX, int screenY)
{
    if (!sketchPolygonValueDialog_) return;
    const int ox = 16;
    const int oy = 16;
    sketchPolygonValueDialog_->move(screenX + ox, screenY + oy);
}

void Widget::openOrRaiseSketchPolygonDialog()
{
    if (!sketchPolygonDialog_) {
        sketchPolygonDialog_ = new SketchPolygonDialog(this);
        sketchPolygonDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchPolygonDialog_, &QObject::destroyed, this, [this]() { sketchPolygonDialog_ = nullptr; });
        connect(sketchPolygonDialog_, &QDialog::rejected, this, [this]() {
            sketchPolygonHasCenter_ = false;
            sketchPolygonPendingField_ = -1;
            sketchClickCount_ = 0;
            if (currentSelectionMode == SketchPolygonPick || currentSelectionMode == SketchDrawPolygon)
                currentSelectionMode = None;
            clearSketchPreviewPolygon();
            clearSketchPreviewLine();
            closeSketchPolygonValueDialog();
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::pickCenterRequested, this, [this]() {
            beginSketchPolygonPick(0);
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::pickSizeRequested, this, [this]() {
            if (!sketchPolygonHasCenter_) {
                statusBar()->showMessage(tr("请先指定中心点。"), 2500);
                return;
            }
            beginSketchPolygonPick(1);
        });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::sizeModeChanged, this,
                [this](SketchPolygonDialog::SizeMode mode) {
                    if (sketchPolygonValueDialog_) {
                        sketchPolygonValueDialog_->setSizeMode(mode);
                    }
                    if (sketchLastHoverValid_)
                        rebuildSketchPolygonPreviewFromHover(sketchLastHoverPoint_);
                });
        connect(sketchPolygonDialog_, &SketchPolygonDialog::paramsChanged, this, [this]() {
            if (sketchPolygonHasCenter_ && sketchLastHoverValid_)
                rebuildSketchPolygonPreviewFromHover(sketchLastHoverPoint_);
        });
    }

    sketchPolygonHasCenter_ = false;
    sketchClickCount_ = 0;
    sketchPolygonDialog_->setCenterText(QString());
    sketchPolygonDialog_->setSizeText(QString());
    sketchPolygonDialog_->show();
    sketchPolygonDialog_->raise();
    positionSketchAuxDialogImpl(vtkWidget, sketchPolygonDialog_);
    beginSketchPolygonPick(0);
}

void Widget::closeSketchPolygonDialog()
{
    if (sketchPolygonDialog_) {
        sketchPolygonDialog_->close();
        sketchPolygonDialog_ = nullptr;
    }
    closeSketchPolygonValueDialog();
    sketchPolygonHasCenter_ = false;
    sketchPolygonPendingField_ = -1;
    sketchClickCount_ = 0;
    if (currentSelectionMode == SketchPolygonPick || currentSelectionMode == SketchDrawPolygon)
        currentSelectionMode = None;
}

void Widget::openOrRaiseSketchEllipseAngleDialog()
{
    if (!sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_ = new SketchEllipseAngleDialog(this);
        sketchEllipseAngleDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchEllipseAngleDialog_, &QObject::destroyed, this, [this]() {
            sketchEllipseAngleDialog_ = nullptr;
        });
        connect(sketchEllipseAngleDialog_, &SketchEllipseAngleDialog::angleCommitted, this,
                &Widget::sketchApplyEllipseAngleFromDialog);
        connect(sketchEllipseAngleDialog_, &SketchEllipseAngleDialog::closedByUser, this, [this]() {
            sketchEllipseAngleDialog_ = nullptr;
        });
    }
    if (sketchEllipseDialog_) {
        sketchEllipseAngleDialog_->setRotationDeg(sketchEllipseDialog_->rotationDeg());
    }
    sketchEllipseAngleDialog_->show();
    sketchEllipseAngleDialog_->raise();
}

void Widget::closeSketchEllipseAngleDialog()
{
    if (sketchEllipseAngleDialog_) {
        sketchEllipseAngleDialog_->close();
        sketchEllipseAngleDialog_ = nullptr;
    }
}

void Widget::positionSketchEllipseAngleDialog(int screenX, int screenY)
{
    if (!sketchEllipseAngleDialog_ || !vtkWidget) return;

    const QRect vtkLocal(0, 0, vtkWidget->width(), vtkWidget->height());
    if (!vtkLocal.contains(QPoint(screenX, screenY))) {
        return;
    }

    const QPoint g = vtkWidget->mapToGlobal(QPoint(screenX, screenY));
    const QSize sz = sketchEllipseAngleDialog_->sizeHint();
    const QRect vtkGlobal(vtkWidget->mapToGlobal(QPoint(0, 0)), vtkWidget->size());
    int px = g.x() + 16;
    int py = g.y() + 16;
    px = qBound(vtkGlobal.left(), px, qMax(vtkGlobal.left(), vtkGlobal.right() - sz.width()));
    py = qBound(vtkGlobal.top(), py, qMax(vtkGlobal.top(), vtkGlobal.bottom() - sz.height()));
    sketchEllipseAngleDialog_->move(px, py);
}

void Widget::openOrRaiseSketchEllipseDialog()
{
    if (!sketchEllipseDialog_) {
        sketchEllipseDialog_ = new SketchEllipseDialog(this);
        sketchEllipseDialog_->setAttribute(Qt::WA_DeleteOnClose);
        connect(sketchEllipseDialog_, &QObject::destroyed, this, [this]() { sketchEllipseDialog_ = nullptr; });
        connect(sketchEllipseDialog_, &QDialog::rejected, this, [this]() {
            sketchEllipseHasCenter_ = false;
            sketchEllipsePendingField_ = -1;
            sketchEllipseGeomIndex_ = -1;
            if (currentSelectionMode == SketchEllipsePick || currentSelectionMode == SketchEllipseAdjust)
                currentSelectionMode = None;
            closeSketchEllipseAngleDialog();
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickCenterRequested, this, [this]() {
            beginSketchEllipsePick(0);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickMajorRequested, this, [this]() {
            beginSketchEllipsePick(1);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::pickMinorRequested, this, [this]() {
            beginSketchEllipsePick(2);
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::paramsChanged, this, [this]() {
            if (sketchEllipseHasCenter_) {
                if (sketchEllipseAngleDialog_) {
                    sketchEllipseAngleDialog_->setRotationDeg(sketchEllipseDialog_->rotationDeg());
                }
                updateSketchEllipseGeometry();
            }
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::applyRequested, this, [this]() {
            if (sketchEllipseHasCenter_) updateSketchEllipseGeometry();
        });
        connect(sketchEllipseDialog_, &SketchEllipseDialog::okRequested, this, [this]() {
            if (sketchEllipseHasCenter_) updateSketchEllipseGeometry();
            statusBar()->showMessage(tr("椭圆参数已确认。"), 2500);
        });
    }

    sketchEllipseHasCenter_ = false;
    sketchEllipseGeomIndex_ = -1;
    sketchEllipsePendingField_ = -1;
    sketchEllipseDialog_->setCenterText(QString());
    sketchEllipseDialog_->setMajorPickText(QString());
    sketchEllipseDialog_->setMinorPickText(QString());
    sketchEllipseDialog_->show();
    sketchEllipseDialog_->raise();
    positionSketchAuxDialogImpl(vtkWidget, sketchEllipseDialog_);
    beginSketchEllipsePick(0);
}

void Widget::closeSketchEllipseDialog()
{
    if (sketchEllipseDialog_) {
        sketchEllipseDialog_->close();
        sketchEllipseDialog_ = nullptr;
    }
    sketchEllipseHasCenter_ = false;
    sketchEllipsePendingField_ = -1;
    sketchEllipseGeomIndex_ = -1;
    sketchEllipseAngleDragActive_ = false;
    closeSketchEllipseAngleDialog();
    if (currentSelectionMode == SketchEllipsePick || currentSelectionMode == SketchEllipseAdjust)
        currentSelectionMode = None;
}
