// 阵列特征：节距轴 Gizmo 与轴端输入框
#include "main_window.h"
#include "pattern_feature_dialog.h"

#include <Precision.hxx>

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkArrowSource.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

namespace {

constexpr double kPatternPickRadiusPx = 16.0;
constexpr int kPatternMargin = 8;
constexpr double kPatternGizmoArrowLength = 2.5;

void applyArrowTransform(vtkTransform* transform, const gp_Pnt& origin, const gp_Dir& dir, double length)
{
    if (!transform) return;
    transform->Identity();
    transform->Translate(origin.X(), origin.Y(), origin.Z());
    const gp_Dir from(1, 0, 0);
    gp_Vec axisVec = gp_Vec(from).Crossed(gp_Vec(dir));
    if (axisVec.Magnitude() > 1e-12) {
        axisVec /= axisVec.Magnitude();
        double dot = from.X() * dir.X() + from.Y() * dir.Y() + from.Z() * dir.Z();
        dot = std::max(-1.0, std::min(1.0, dot));
        const double angleDeg = vtkMath::DegreesFromRadians(std::acos(dot));
        transform->RotateWXYZ(angleDeg, axisVec.X(), axisVec.Y(), axisVec.Z());
    } else {
        double dot = from.X() * dir.X() + from.Y() * dir.Y() + from.Z() * dir.Z();
        if (dot < 0) {
            transform->RotateWXYZ(180.0, 0, 1, 0);
        }
    }
    transform->Scale(length, length, length);
}

bool clipSegmentToRect(double x0, double y0, double x1, double y1,
                       double xmin, double ymin, double xmax, double ymax,
                       double& outX, double& outY)
{
    double t0 = 0.0, t1 = 1.0;
    const double dx = x1 - x0, dy = y1 - y0;
    auto clipT = [&](double p, double q) -> bool {
        if (std::abs(p) < 1e-12) return q >= 0.0;
        const double r = q / p;
        if (p < 0.0) {
            if (r > t1) return false;
            if (r > t0) t0 = r;
        } else {
            if (r < t0) return false;
            if (r < t1) t1 = r;
        }
        return true;
    };
    if (!clipT(-dx, x0 - xmin) || !clipT(dx, xmax - x0) || !clipT(-dy, y0 - ymin) || !clipT(dy, ymax - y0)) {
        return false;
    }
    if (t0 > t1) return false;
    outX = x0 + t1 * dx;
    outY = y0 + t1 * dy;
    return true;
}

void worldToQtScreen(vtkRenderer* renderer, int vtkH, const gp_Pnt& p, double& qx, double& qy)
{
    renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
    renderer->WorldToDisplay();
    double d[3] = {0, 0, 0};
    renderer->GetDisplayPoint(d);
    qx = d[0];
    qy = static_cast<double>(vtkH) - d[1];
}

void computePitchOverlayPos(vtkRenderer* renderer, int vtkW, int vtkH, int overlayW, int overlayH,
                            const gp_Pnt& origin, const gp_Pnt& tip, int& posX, int& posY)
{
    double ox, oy, tx, ty;
    worldToQtScreen(renderer, vtkH, origin, ox, oy);
    worldToQtScreen(renderer, vtkH, tip, tx, ty);
    const double xmin = kPatternMargin, ymin = kPatternMargin;
    const double xmax = std::max(xmin + 1.0, static_cast<double>(vtkW) - kPatternMargin);
    const double ymax = std::max(ymin + 1.0, static_cast<double>(vtkH) - kPatternMargin);
    double ax = tx, ay = ty;
    if (!clipSegmentToRect(ox, oy, tx, ty, xmin, ymin, xmax, ymax, ax, ay)) {
        ax = std::max(xmin, std::min(tx, xmax));
        ay = std::max(ymin, std::min(ty, ymax));
    }
    posX = static_cast<int>(std::lround(ax)) - overlayW / 2;
    posY = static_cast<int>(std::lround(ay)) - overlayH - 12;
    posX = std::max(kPatternMargin, std::min(posX, vtkW - overlayW - kPatternMargin));
    posY = std::max(kPatternMargin, std::min(posY, vtkH - overlayH - kPatternMargin));
}

double screenCoordAlongAxis(int x, int y, double ox, double oy, double ax, double ay)
{
    const double lenSq = ax * ax + ay * ay;
    if (lenSq < 1e-6) return 0.0;
    return ((x - ox) * ax + (y - oy) * ay) / lenSq;
}

} // namespace

void Widget::clearPatternPitchGizmoOnly()
{
    if (!renderer) return;
    auto remove = [this](vtkActor* a) {
        if (a) removeSceneActor(a);
    };
    remove(patternGizmoLineActor_);
    patternGizmoLineActor_ = nullptr;
    remove(patternGizmoArrowActor_);
    patternGizmoArrowActor_ = nullptr;
}

void Widget::updatePatternPitchGizmo()
{
    if (!patternPitchInteractiveActive_ || !renderer || !patternDialog_) {
        clearPatternPitchGizmoOnly();
        return;
    }
    clearPatternPitchGizmoOnly();

    gp_Pnt O;
    gp_Pnt tip;
    computePatternGizmoSegment(O, tip, patternActivePitchAxis_);
    const gp_Dir dir = patternGizmoArrowDirection(O, tip, patternActivePitchAxis_);
    vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
    line->SetPoint1(O.X(), O.Y(), O.Z());
    line->SetPoint2(tip.X(), tip.Y(), tip.Z());
    vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    lineMapper->SetInputConnection(line->GetOutputPort());
    patternGizmoLineActor_ = vtkSmartPointer<vtkActor>::New();
    patternGizmoLineActor_->SetMapper(lineMapper);
    patternGizmoLineActor_->GetProperty()->SetColor(0.55, 0.68, 0.88);
    patternGizmoLineActor_->GetProperty()->SetLineWidth(2.0);
    patternGizmoLineActor_->SetPickable(false);
    addReferenceActor(patternGizmoLineActor_);

    // 屏幕尺寸恒定（不随节距/相机距离漂移）
    const double arrowLen = kPatternGizmoArrowLength
        * overlayWorldScaleAt(tip.X(), tip.Y(), tip.Z());
    vtkSmartPointer<vtkArrowSource> arrowSrc = vtkSmartPointer<vtkArrowSource>::New();
    arrowSrc->SetTipLength(0.35);
    arrowSrc->SetTipRadius(0.12);
    vtkSmartPointer<vtkTransform> xf = vtkSmartPointer<vtkTransform>::New();
    applyArrowTransform(xf, tip, dir, arrowLen);
    vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetInputConnection(arrowSrc->GetOutputPort());
    filter->SetTransform(xf);
    vtkSmartPointer<vtkPolyDataMapper> arrowMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    arrowMapper->SetInputConnection(filter->GetOutputPort());
    patternGizmoArrowActor_ = vtkSmartPointer<vtkActor>::New();
    patternGizmoArrowActor_->SetMapper(arrowMapper);
    patternGizmoArrowActor_->GetProperty()->SetColor(0.78, 0.68, 0.32);
    patternGizmoArrowActor_->SetPickable(false);
    addReferenceActor(patternGizmoArrowActor_);
}

void Widget::hidePatternPitchOverlay()
{
    if (patternPitchOverlay_) {
        patternPitchOverlay_->hide();
    }
}

void Widget::updatePatternPitchOverlay()
{
    if (!patternPitchInteractiveActive_ || !patternDialog_ || !renderer || !vtkWidget) {
        hidePatternPitchOverlay();
        return;
    }

    if (!patternPitchOverlay_) {
        patternPitchOverlay_ = new QWidget(vtkWidget);
        patternPitchOverlay_->setStyleSheet(
            QStringLiteral("QWidget { background: rgba(40,40,40,200); border-radius: 4px; }"
                           "QLabel { color: white; }"
                           "QLineEdit { background: white; color: black; min-width: 72px; }"
                           "QPushButton { background: #4CAF50; color: white; }"));
        auto* lay = new QHBoxLayout(patternPitchOverlay_);
        lay->setContentsMargins(6, 4, 6, 4);
        lay->addWidget(new QLabel(tr("节距"), patternPitchOverlay_));
        patternPitchEdit_ = new QLineEdit(patternPitchOverlay_);
        patternPitchEdit_->setValidator(new QDoubleValidator(0.1, 1e6, 4, patternPitchOverlay_));
        auto* okBtn = new QPushButton(QStringLiteral("✓"), patternPitchOverlay_);
        lay->addWidget(patternPitchEdit_);
        lay->addWidget(okBtn);
        patternPitchOverlay_->adjustSize();

        connect(patternPitchEdit_, &QLineEdit::returnPressed, this, [this]() {
            if (!patternPitchEdit_ || !patternDialog_) return;
            bool ok = false;
            const double v = patternPitchEdit_->text().toDouble(&ok);
            if (!ok || v < 0.1) return;
            applyPatternPitchInteractiveValue(v, patternActivePitchAxis_);
            updatePatternPreview();
        });
        connect(patternPitchEdit_, &QLineEdit::textEdited, this, [this](const QString& text) {
            if (!patternDialog_) return;
            bool ok = false;
            const double v = text.toDouble(&ok);
            if (!ok || v < 0.1) return;
            applyPatternPitchInteractiveValue(v, patternActivePitchAxis_);
            updatePatternPreview();
        });
        connect(okBtn, &QPushButton::clicked, patternPitchEdit_, &QLineEdit::returnPressed);
    }

    gp_Pnt O;
    gp_Pnt tip;
    computePatternGizmoSegment(O, tip, patternActivePitchAxis_);
    const gp_Dir dir = patternDirection(patternActivePitchAxis_);
    const double pitch = patternPitchInteractiveValue(patternActivePitchAxis_);
    const bool polygonalSpan =
        patternDialog_->layoutType() == PatternLayoutType::Polygonal && patternActivePitchAxis_ == 0;
    if (auto* label = patternPitchOverlay_->findChild<QLabel*>()) {
        label->setText(polygonalSpan ? tr("跨距") : tr("节距"));
    }

    const int w = patternPitchOverlay_->width();
    const int h = patternPitchOverlay_->height();
    int posX = 0, posY = 0;
    computePitchOverlayPos(renderer, vtkWidget->width(), vtkWidget->height(), w, h, O, tip, posX, posY);

    if (patternPitchEdit_) {
        const bool angular = patternAxisUsesAngularPitch(patternActivePitchAxis_);
        const double minV = polygonalSpan ? 1.0 : 0.1;
        patternPitchEdit_->setValidator(
            new QDoubleValidator(minV, angular ? 360.0 : 1e6, 4, patternPitchOverlay_));
        if (!patternPitchEdit_->hasFocus()) {
            QSignalBlocker b(patternPitchEdit_);
            patternPitchEdit_->setText(QString::number(pitch, 'f', 4));
        }
    }
    patternPitchOverlay_->move(posX, posY);
    patternPitchOverlay_->raise();
    patternPitchOverlay_->show();
}

void Widget::startPatternPitchInteractive(int axisIndex)
{
    if (!patternDialog_ || !patternDialog_->hasDirection1()) {
        return;
    }
    if (axisIndex == 1) {
        if (patternDialog_->layoutType() != PatternLayoutType::Linear
            || !patternDialog_->useDirection2() || !patternDialog_->hasDirection2()) {
            return;
        }
    }
    patternPitchInteractiveActive_ = true;
    patternActivePitchAxis_ = axisIndex;
    patternArrayOrigin_ = computePatternArrayOrigin();
    currentSelectionMode = PatternPitchInteractive;
    updatePatternPitchGizmo();
    updatePatternPitchOverlay();
    if (vtkWidget) vtkWidget->setFocus();
}

void Widget::stopPatternPitchInteractive()
{
    patternPitchInteractiveActive_ = false;
    patternPitchDragActive_ = false;
    clearPatternPitchGizmoOnly();
    hidePatternPitchOverlay();
    if (patternPitchOverlay_) {
        patternPitchOverlay_->deleteLater();
        patternPitchOverlay_ = nullptr;
        patternPitchEdit_ = nullptr;
    }
    if (patternDialog_ && currentSelectionMode == PatternPitchInteractive) {
        currentSelectionMode = PatternBodySelection;
    }
}

bool Widget::patternPickPitchArrow(int x, int y) const
{
    if (!renderer || !patternDialog_) {
        return false;
    }
    gp_Pnt O;
    gp_Pnt tip;
    computePatternGizmoSegment(O, tip, patternActivePitchAxis_);
    const gp_Dir dir = patternDirection(patternActivePitchAxis_);
    if (!vtkWidget) {
        return false;
    }

    double ox = 0.0, oy = 0.0, tx = 0.0, ty = 0.0;
    worldToQtScreen(renderer, vtkWidget->height(), O, ox, oy);
    worldToQtScreen(renderer, vtkWidget->height(), tip, tx, ty);
    const double mx = static_cast<double>(x);
    const double my = static_cast<double>(y);
    const double ax = tx - ox;
    const double ay = ty - oy;
    const double lenSq = ax * ax + ay * ay;

    if (lenSq > 1e-6) {
        const double t = ((mx - ox) * ax + (my - oy) * ay) / lenSq;
        if (t >= 0.72) {
            const double px = ox + t * ax;
            const double py = oy + t * ay;
            const double dx = mx - px;
            const double dy = my - py;
            const double r2 = kPatternPickRadiusPx * kPatternPickRadiusPx;
            if (dx * dx + dy * dy <= r2) {
                return true;
            }
        }
    }

    renderer->SetWorldPoint(tip.X(), tip.Y(), tip.Z(), 1.0);
    renderer->WorldToDisplay();
    double d[3] = {0, 0, 0};
    renderer->GetDisplayPoint(d);
    const double dx = d[0] - mx;
    const double dy = d[1] - my;
    const double r2 = kPatternPickRadiusPx * kPatternPickRadiusPx;
    return (dx * dx + dy * dy <= r2);
}

void Widget::handlePatternPitchMouseMove(int x, int y)
{
    if (!patternPitchInteractiveActive_ || !patternDialog_) return;

    if (patternPitchDragActive_) {
        const double tNow = screenCoordAlongAxis(x, y, patternDragAxisScrOx_, patternDragAxisScrOy_,
                                                 patternDragAxisScrDx_, patternDragAxisScrDy_);
        const double t0 = patternDragStartParam_;
        const double scale = (std::abs(t0) > 1e-6) ? (patternDragStartPitch_ / t0) : patternDragStartPitch_;
        const double minPitch =
            (patternDialog_->layoutType() == PatternLayoutType::Polygonal && patternActivePitchAxis_ == 0)
                ? 1.0
                : 0.1;
        const double newPitch = std::max(minPitch, patternDragStartPitch_ + (tNow - t0) * scale);
        applyPatternPitchInteractiveValue(newPitch, patternActivePitchAxis_);
        updatePatternPreview();
        if (vtkWidget) {
            gp_Pnt o;
            gp_Pnt t;
            computePatternGizmoSegment(o, t, patternActivePitchAxis_);
            vtkWidget->setCursor(cuboidCursorForAxis(patternGizmoArrowDirection(o, t, patternActivePitchAxis_)));
        }
        return;
    }

    const bool hit = patternPickPitchArrow(x, y);
    if (vtkWidget) {
        if (hit) {
            gp_Pnt o;
            gp_Pnt t;
            computePatternGizmoSegment(o, t, patternActivePitchAxis_);
            vtkWidget->setCursor(cuboidCursorForAxis(patternGizmoArrowDirection(o, t, patternActivePitchAxis_)));
        } else {
            vtkWidget->unsetCursor();
        }
    }
}

void Widget::handlePatternPitchMouseDown(int x, int y)
{
    if (!patternPitchInteractiveActive_ || !patternDialog_) return;
    if (!patternPickPitchArrow(x, y)) return;

    patternPitchDragActive_ = true;
    patternDragStartPitch_ = patternPitchInteractiveValue(patternActivePitchAxis_);

    if (renderer && vtkWidget) {
        double ox = 0.0, oy = 0.0, dx = 0.0, dy = 0.0;
        computePatternPitchDragScreenAxis(ox, oy, dx, dy, patternActivePitchAxis_);
        patternDragAxisScrOx_ = ox;
        patternDragAxisScrOy_ = oy;
        patternDragAxisScrDx_ = dx;
        patternDragAxisScrDy_ = dy;
        patternDragStartParam_ = screenCoordAlongAxis(x, y, ox, oy, dx, dy);
    }
    updatePatternPitchOverlay();
}

void Widget::handlePatternPitchMouseUp(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    if (!patternPitchDragActive_) return;
    patternPitchDragActive_ = false;
    updatePatternPitchOverlay();
    if (patternPitchEdit_) {
        QTimer::singleShot(0, this, [this]() {
            if (patternPitchEdit_) {
                patternPitchEdit_->setFocus(Qt::MouseFocusReason);
                patternPitchEdit_->selectAll();
            }
        });
    }
    handlePatternPitchMouseMove(x, y);
}
