// 基准平面 / 基准轴：对话框、实时预览与创建
#include "main_window.h"
#include "datum_plane_dialog.h"
#include "datum_axis.h"

#include <algorithm>
#include <cmath>

#include <QMessageBox>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <math_Jacobi.hxx>
#include <math_Matrix.hxx>
#include <math_Vector.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

namespace {

constexpr double kDatumPreviewR = 1.0;
constexpr double kDatumPreviewG = 0.92;
constexpr double kDatumPreviewB = 0.45;
constexpr double kDatumPreviewOpacity = 0.42;
/** 基准轴长度接近参考坐标系箭头，避免过大 */
constexpr double kDatumAxisScale = 0.85;

void applyDatumPreviewProperty(vtkActor* actor, bool edgeOn = true)
{
    if (!actor) return;
    actor->GetProperty()->SetColor(kDatumPreviewR, kDatumPreviewG, kDatumPreviewB);
    actor->GetProperty()->SetOpacity(kDatumPreviewOpacity);
    actor->GetProperty()->SetLighting(false);
    actor->GetProperty()->SetBackfaceCulling(false);
    if (edgeOn) {
        actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(kDatumPreviewR * 0.85, kDatumPreviewG * 0.85, kDatumPreviewB * 0.7);
        actor->GetProperty()->SetLineWidth(1.5);
    }
    actor->SetPickable(false);
}

double computeDatumHalfSize(const QList<ModelingHistory>& historyList,
                            int currentSelectedIndex,
                            const QList<gp_Pnt>& snapPoints)
{
    double bounds[6] = {0, 0, 0, 0, 0, 0};
    bool hasBounds = false;

    auto tryActorBounds = [&](vtkActor* a) {
        if (!a || a->GetVisibility() == 0) return;
        double b[6];
        a->GetBounds(b);
        if (!(b[0] <= b[1] && b[2] <= b[3] && b[4] <= b[5])) return;
        if (!hasBounds) {
            for (int i = 0; i < 6; ++i) bounds[i] = b[i];
            hasBounds = true;
        } else {
            bounds[0] = std::min(bounds[0], b[0]);
            bounds[1] = std::max(bounds[1], b[1]);
            bounds[2] = std::min(bounds[2], b[2]);
            bounds[3] = std::max(bounds[3], b[3]);
            bounds[4] = std::min(bounds[4], b[4]);
            bounds[5] = std::max(bounds[5], b[5]);
        }
    };

    if (currentSelectedIndex >= 0 && currentSelectedIndex < historyList.size()) {
        tryActorBounds(historyList[currentSelectedIndex].actor);
    }
    if (!hasBounds) {
        for (const auto& h : historyList) {
            tryActorBounds(h.actor);
        }
    }
    if (!hasBounds && !snapPoints.isEmpty()) {
        double minX = snapPoints.first().X(), maxX = minX;
        double minY = snapPoints.first().Y(), maxY = minY;
        double minZ = snapPoints.first().Z(), maxZ = minZ;
        for (const auto& p : snapPoints) {
            minX = std::min(minX, p.X()); maxX = std::max(maxX, p.X());
            minY = std::min(minY, p.Y()); maxY = std::max(maxY, p.Y());
            minZ = std::min(minZ, p.Z()); maxZ = std::max(maxZ, p.Z());
        }
        bounds[0] = minX; bounds[1] = maxX;
        bounds[2] = minY; bounds[3] = maxY;
        bounds[4] = minZ; bounds[5] = maxZ;
        hasBounds = true;
    }

    const double fallbackHalf = 10.0;
    if (!hasBounds) return fallbackHalf;

    const double dx = std::abs(bounds[1] - bounds[0]);
    const double dy = std::abs(bounds[3] - bounds[2]);
    const double dz = std::abs(bounds[5] - bounds[4]);
    const double maxDim = std::max(dx, std::max(dy, dz));
    double half = maxDim * 0.75;
    if (half < 2.0) half = 2.0;
    if (half > 200.0) half = 200.0;
    return half;
}

bool computeDatumPlaneGeometry(const QList<ModelingHistory>& historyList,
                               int currentSelectedIndex,
                               const QString& mode,
                               bool offsetEnabled,
                               double offsetValue,
                               const QList<gp_Pnt>& snapPoints,
                               gp_Pnt& origin,
                               gp_Dir& normal,
                               gp_Pnt& pOrigin,
                               gp_Pnt& p1w,
                               gp_Pnt& p2w,
                               QString* errorMessage)
{
    origin = gp_Pnt(0, 0, 0);
    normal = gp_Dir(0, 0, 1);

    if (mode == QStringLiteral("YC-ZC平面")) {
        origin = gp_Pnt(0, 0, 0);
        normal = gp_Dir(1, 0, 0);
    } else if (mode == QStringLiteral("XC-ZC平面")) {
        origin = gp_Pnt(0, 0, 0);
        normal = gp_Dir(0, 1, 0);
    } else if (mode == QStringLiteral("XC-YC平面")) {
        origin = gp_Pnt(0, 0, 0);
        normal = gp_Dir(0, 0, 1);
    } else {
        if (snapPoints.size() < 3) {
            if (errorMessage) {
                *errorMessage = Widget::tr("至少需要捕捉 3 个点才能定义平面。");
            }
            return false;
        }

        double cx = 0, cy = 0, cz = 0;
        for (const auto& p : snapPoints) {
            cx += p.X(); cy += p.Y(); cz += p.Z();
        }
        cx /= snapPoints.size();
        cy /= snapPoints.size();
        cz /= snapPoints.size();
        const gp_Pnt centroid(cx, cy, cz);

        double c00 = 0, c01 = 0, c02 = 0, c11 = 0, c12 = 0, c22 = 0;
        for (const auto& p : snapPoints) {
            const double x = p.X() - cx;
            const double y = p.Y() - cy;
            const double z = p.Z() - cz;
            c00 += x * x;
            c01 += x * y;
            c02 += x * z;
            c11 += y * y;
            c12 += y * z;
            c22 += z * z;
        }
        const double denom = std::max<double>(1.0, static_cast<double>(snapPoints.size()));
        c00 /= denom; c01 /= denom; c02 /= denom; c11 /= denom; c12 /= denom; c22 /= denom;

        math_Matrix A(1, 3, 1, 3);
        A(1, 1) = c00; A(1, 2) = c01; A(1, 3) = c02;
        A(2, 1) = c01; A(2, 2) = c11; A(2, 3) = c12;
        A(3, 1) = c02; A(3, 2) = c12; A(3, 3) = c22;

        math_Jacobi jac(A);
        if (!jac.IsDone()) {
            const gp_Pnt p0 = snapPoints[snapPoints.size() - 3];
            const gp_Pnt p1 = snapPoints[snapPoints.size() - 2];
            const gp_Pnt p2 = snapPoints[snapPoints.size() - 1];
            gp_Vec n = gp_Vec(p0, p1).Crossed(gp_Vec(p0, p2));
            if (n.Magnitude() <= 1e-9) {
                if (errorMessage) {
                    *errorMessage = Widget::tr("捕捉点退化，无法定义平面。");
                }
                return false;
            }
            origin = p0;
            normal = gp_Dir(n);
        } else {
            const math_Vector& vals = jac.Values();
            const math_Matrix& vecs = jac.Vectors();

            int minIdx = 1;
            if (vals(2) < vals(minIdx)) minIdx = 2;
            if (vals(3) < vals(minIdx)) minIdx = 3;

            gp_Vec n(vecs(1, minIdx), vecs(2, minIdx), vecs(3, minIdx));
            if (n.Magnitude() <= 1e-12) {
                if (errorMessage) {
                    *errorMessage = Widget::tr("捕捉点退化，无法定义平面。");
                }
                return false;
            }
            origin = centroid;
            normal = gp_Dir(n);
        }
    }

    if (offsetEnabled) {
        origin.Translate(gp_Vec(normal) * offsetValue);
    }

    const double size = computeDatumHalfSize(historyList, currentSelectedIndex, snapPoints);

    gp_Dir t1;
    if (std::abs(normal.Z()) < 0.9) {
        t1 = gp_Dir(0, 0, 1).Crossed(normal);
    } else {
        t1 = gp_Dir(0, 1, 0).Crossed(normal);
    }
    gp_Dir t2 = normal.Crossed(t1);

    const bool isCsysPlane = (mode == QStringLiteral("YC-ZC平面")
                              || mode == QStringLiteral("XC-ZC平面")
                              || mode == QStringLiteral("XC-YC平面"));

    if (!isCsysPlane && !snapPoints.isEmpty()) {
        double minU = 0, maxU = 0, minV = 0, maxV = 0;
        bool has = false;
        for (const auto& p : snapPoints) {
            gp_Vec op(origin, p);
            const double u = op.Dot(gp_Vec(t1));
            const double v = op.Dot(gp_Vec(t2));
            if (!has) {
                minU = maxU = u;
                minV = maxV = v;
                has = true;
            } else {
                minU = std::min(minU, u); maxU = std::max(maxU, u);
                minV = std::min(minV, v); maxV = std::max(maxV, v);
            }
        }
        const double spanU = std::max(1e-6, maxU - minU);
        const double spanV = std::max(1e-6, maxV - minV);
        const double eps = 1e-3 * std::max(spanU, spanV);
        minU -= eps; maxU += eps;
        minV -= eps; maxV += eps;

        pOrigin = origin.Translated(gp_Vec(t1) * minU + gp_Vec(t2) * minV);
        p1w     = origin.Translated(gp_Vec(t1) * maxU + gp_Vec(t2) * minV);
        p2w     = origin.Translated(gp_Vec(t1) * minU + gp_Vec(t2) * maxV);
    } else {
        pOrigin = origin.Translated(gp_Vec(t1) * (-size) + gp_Vec(t2) * (-size));
        p1w     = origin.Translated(gp_Vec(t1) * ( size) + gp_Vec(t2) * (-size));
        p2w     = origin.Translated(gp_Vec(t1) * (-size) + gp_Vec(t2) * ( size));
    }
    return true;
}

vtkSmartPointer<vtkActor> makeDatumPlaneActor(const gp_Pnt& pOrigin,
                                              const gp_Pnt& p1w,
                                              const gp_Pnt& p2w,
                                              bool preview,
                                              vtkSmartPointer<vtkPolyData>* outPolyData)
{
    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetOrigin(pOrigin.X(), pOrigin.Y(), pOrigin.Z());
    plane->SetPoint1(p1w.X(), p1w.Y(), p1w.Z());
    plane->SetPoint2(p2w.X(), p2w.Y(), p2w.Z());
    plane->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(plane->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (preview) {
        applyDatumPreviewProperty(actor, true);
    } else {
        actor->GetProperty()->SetColor(0.2, 0.8, 1.0);
        actor->GetProperty()->SetOpacity(0.25);
        actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetLineWidth(2.0);
        actor->SetPickable(false);
    }

    if (outPolyData) {
        *outPolyData = vtkSmartPointer<vtkPolyData>::New();
        (*outPolyData)->ShallowCopy(plane->GetOutput());
    }
    return actor;
}

vtkSmartPointer<vtkActor> makeDatumAxisActor(AxisDirection axis,
                                             bool reversed,
                                             bool preview,
                                             vtkSmartPointer<vtkPolyData>* outPolyData)
{
    vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
    arrow->SetTipLength(0.28);
    arrow->SetTipRadius(0.09);
    arrow->SetShaftRadius(0.03);
    arrow->SetShaftResolution(16);
    arrow->SetTipResolution(16);

    gp_Dir dir(1, 0, 0);
    if (axis == AxisDirection::Y) {
        dir = gp_Dir(0, 1, 0);
    } else if (axis == AxisDirection::Z) {
        dir = gp_Dir(0, 0, 1);
    }
    if (reversed) {
        dir.Reverse();
    }

    // 与矢量箭头一致：把默认 +X 映射到目标方向，再缩放（避免负缩放导致 Y/Z 反向失效）
    vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
    t->Identity();
    const gp_Dir from(1, 0, 0);
    gp_Vec axisVec = gp_Vec(from).Crossed(gp_Vec(dir));
    if (axisVec.Magnitude() > 1e-12) {
        axisVec /= axisVec.Magnitude();
        double dot = from.X() * dir.X() + from.Y() * dir.Y() + from.Z() * dir.Z();
        dot = std::max(-1.0, std::min(1.0, dot));
        const double angleDeg = std::acos(dot) * 180.0 / 3.14159265358979323846;
        t->RotateWXYZ(angleDeg, axisVec.X(), axisVec.Y(), axisVec.Z());
    } else if (from.X() * dir.X() + from.Y() * dir.Y() + from.Z() * dir.Z() < 0) {
        t->RotateWXYZ(180.0, 0, 1, 0);
    }
    t->Scale(kDatumAxisScale, kDatumAxisScale, kDatumAxisScale);

    vtkSmartPointer<vtkTransformPolyDataFilter> tf =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    tf->SetTransform(t);
    tf->SetInputConnection(arrow->GetOutputPort());
    tf->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(tf->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (preview) {
        applyDatumPreviewProperty(actor, false);
        actor->GetProperty()->SetOpacity(0.55);
    } else {
        actor->GetProperty()->SetColor(0.95, 0.78, 0.25);
        actor->GetProperty()->SetOpacity(0.85);
        actor->GetProperty()->SetAmbient(0.55);
        actor->GetProperty()->SetDiffuse(0.55);
        actor->SetPickable(false);
    }

    if (outPolyData) {
        *outPolyData = vtkSmartPointer<vtkPolyData>::New();
        (*outPolyData)->ShallowCopy(tf->GetOutput());
    }
    return actor;
}


} // namespace

void Widget::clearDatumPlanePreview()
{
    if (datumPlanePreviewActor_ && renderer) {
        removeSceneActor(datumPlanePreviewActor_);
        datumPlanePreviewActor_ = nullptr;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
}

void Widget::updateDatumPlanePreview()
{
    if (!datumPlaneDialog_ || !renderer || !vtkWidget) {
        return;
    }

    clearDatumPlanePreview();

    gp_Pnt origin, pOrigin, p1w, p2w;
    gp_Dir normal;
    QString err;
    if (!computeDatumPlaneGeometry(historyList, currentSelectedIndex,
                                   datumPlaneDialog_->modeText(),
                                   datumPlaneDialog_->isOffsetEnabled(),
                                   datumPlaneDialog_->offsetValue(),
                                   snapPersistentPoints_,
                                   origin, normal, pOrigin, p1w, p2w, &err)) {
        return;
    }

    datumPlanePreviewActor_ = makeDatumPlaneActor(pOrigin, p1w, p2w, true, nullptr);
    addAppearanceActor(datumPlanePreviewActor_);
    vtkWidget->renderWindow()->Render();
}

void Widget::commitDatumPlane()
{
    if (!datumPlaneDialog_ || !renderer || !vtkWidget) {
        return;
    }

    clearDatumPlanePreview();

    gp_Pnt origin, pOrigin, p1w, p2w;
    gp_Dir normal;
    QString err;
    if (!computeDatumPlaneGeometry(historyList, currentSelectedIndex,
                                   datumPlaneDialog_->modeText(),
                                   datumPlaneDialog_->isOffsetEnabled(),
                                   datumPlaneDialog_->offsetValue(),
                                   snapPersistentPoints_,
                                   origin, normal, pOrigin, p1w, p2w, &err)) {
        QMessageBox::warning(this, tr("基准平面"),
                             err.isEmpty() ? tr("无法创建基准平面。") : err);
        return;
    }

    vtkSmartPointer<vtkPolyData> planePd;
    vtkSmartPointer<vtkActor> actor =
        makeDatumPlaneActor(pOrigin, p1w, p2w, false, &planePd);
    renderer->AddActor(actor);
    vtkWidget->renderWindow()->Render();

    int datumPlaneCount = 0;
    for (const auto& h : historyList) {
        if (h.type == DATUM_PLANE) {
            ++datumPlaneCount;
        }
    }
    const QString planeName = QString("基准平面%1").arg(datumPlaneCount + 1);
    addToHistory(
        DATUM_PLANE,
        planeName,
        actor,
        QColor(51, 204, 255),
        0.0, 0.0, 0.0,
        planePd
    );
}

void Widget::on_datum_plane_Button_clicked()
{
    if (!renderer || !vtkWidget) {
        return;
    }

    if (datumPlaneDialog_) {
        datumPlaneDialog_->raise();
        datumPlaneDialog_->activateWindow();
        return;
    }

    auto* dialog = new datum_plane(dialogParentWidget());
    dialog->setModal(false);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setCapturedPoints(snapPersistentPoints_);
    datumPlaneDialog_ = dialog;

    connect(dialog, &datum_plane::parametersChanged, this, &Widget::updateDatumPlanePreview);
    connect(dialog, &datum_plane::accepted, this, &Widget::commitDatumPlane);
    connect(dialog, &QDialog::finished, this, [this](int) {
        clearDatumPlanePreview();
        datumPlaneDialog_ = nullptr;
        if (vtkWidget) {
            vtkWidget->setFocus();
        }
    });

    updateDatumPlanePreview();
    dialog->show();
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

void Widget::clearDatumAxisPreview()
{
    if (datumAxisPreviewActor_ && renderer) {
        removeSceneActor(datumAxisPreviewActor_);
        datumAxisPreviewActor_ = nullptr;
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
}

void Widget::updateDatumAxisPreview()
{
    if (!datumAxisDialog_ || !renderer || !vtkWidget) {
        return;
    }

    clearDatumAxisPreview();
    datumAxisPreviewActor_ = makeDatumAxisActor(
        datumAxisDialog_->axisDirection(),
        datumAxisDialog_->isReversed(),
        true,
        nullptr);
    addAppearanceActor(datumAxisPreviewActor_);
    vtkWidget->renderWindow()->Render();
}

void Widget::commitDatumAxis()
{
    if (!datumAxisDialog_ || !renderer || !vtkWidget) {
        return;
    }

    clearDatumAxisPreview();

    vtkSmartPointer<vtkPolyData> axisPd;
    vtkSmartPointer<vtkActor> actor = makeDatumAxisActor(
        datumAxisDialog_->axisDirection(),
        datumAxisDialog_->isReversed(),
        false,
        &axisPd);
    renderer->AddActor(actor);
    vtkWidget->renderWindow()->Render();

    int datumAxisCount = 0;
    for (const auto& h : historyList) {
        if (h.type == DATUM_AXIS) {
            ++datumAxisCount;
        }
    }
    const QString axisName = QString("基准轴%1 (%2%3)")
        .arg(datumAxisCount + 1)
        .arg(datumAxisDialog_->axisText())
        .arg(datumAxisDialog_->isReversed() ? tr("·反向") : QString());

    const int histIndex = historyList.size();
    addToHistory(
        DATUM_AXIS,
        axisName,
        actor,
        QColor(242, 198, 64),
        0.85, 0.0, 0.0,
        axisPd
    );
    if (histIndex >= 0 && histIndex < historyList.size()) {
        historyList[histIndex].axisDirection = datumAxisDialog_->axisDirection();
        historyList[histIndex].axisReversed = datumAxisDialog_->isReversed();
    }

    if (statusBar()) {
        statusBar()->showMessage(tr("已创建 %1").arg(axisName), 2500);
    }
}

void Widget::on_pushButton_4_clicked()
{
    if (!renderer || !vtkWidget) {
        return;
    }

    if (datumAxisDialog_) {
        datumAxisDialog_->raise();
        datumAxisDialog_->activateWindow();
        return;
    }

    auto* dialog = new DatumAxisDialog(dialogParentWidget());
    dialog->setModal(false);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    datumAxisDialog_ = dialog;

    connect(dialog, &DatumAxisDialog::parametersChanged, this, &Widget::updateDatumAxisPreview);
    connect(dialog, &DatumAxisDialog::accepted, this, &Widget::commitDatumAxis);
    connect(dialog, &QDialog::finished, this, [this](int) {
        clearDatumAxisPreview();
        datumAxisDialog_ = nullptr;
        if (vtkWidget) {
            vtkWidget->setFocus();
        }
    });

    updateDatumAxisPreview();
    dialog->show();
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}
