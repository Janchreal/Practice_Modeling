// 长方体交互式预览：默认预览、原点拖拽、三轴尺寸拖拽与参数输入
#include "main_window.h"
#include "cuboid_params_dialog.h"
#include "handle_geometry.h"
#include "primitive_geometry.h"
#include "ui_cuboid_params_dialog.h"

#include <Precision.hxx>
#include <gp_Ax2.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <IntAna_IntConicQuad.hxx>

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkConeSource.h>
#include <vtkFollower.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

namespace {

constexpr double kCuboidMinDim = 0.1;
constexpr double kCuboidPickRadiusPx = 16.0;

gp_Ax2 buildCuboidAxisSystem(const gp_Pnt& origin,
                             AxisDirection axisDirection,
                             bool axisReversed,
                             bool hasCustomVectorDir,
                             const gp_Dir& customVectorDir)
{
    gp_Dir zDir(0, 0, 1);
    if (hasCustomVectorDir) {
        zDir = customVectorDir;
    } else if (axisDirection == AxisDirection::X) {
        zDir = gp_Dir(1, 0, 0);
    } else if (axisDirection == AxisDirection::Y) {
        zDir = gp_Dir(0, 1, 0);
    }
    if (axisReversed) {
        zDir.Reverse();
    }
    return gp_Ax2(origin, zDir);
}

static const char* cuboidAxisHandleId(int axisIndex)
{
    static const char* kIds[] = {"cuboid_length", "cuboid_width", "cuboid_height"};
    if (axisIndex < 0 || axisIndex > 2) return kIds[0];
    return kIds[axisIndex];
}

HandleStateStyle cuboidOriginStyle(ControlState state)
{
    return HandleGeom::styleForControl(QStringLiteral("cuboid_length"),
                                       QStringLiteral("origin_point"),
                                       state);
}

HandleStateStyle cuboidAxisTipStyle(int axisIndex, ControlState state)
{
    return HandleGeom::styleForControl(QString::fromLatin1(cuboidAxisHandleId(axisIndex)),
                                       QStringLiteral("axis_tip"),
                                       state);
}

double applyCuboidDimCr(int axisIndex, double raw)
{
    if (const ControlSpec* c = HandleGeom::findControlSpec(
            QString::fromLatin1(cuboidAxisHandleId(axisIndex)), QStringLiteral("axis_tip"))) {
        if (c->cr) return c->cr(raw);
    }
    return raw < kCuboidMinDim ? kCuboidMinDim : raw;
}

// 将线段 (x0,y0)-(x1,y1) 裁剪到矩形内，preferTip=true 时取靠近 (x1,y1) 的可见点
bool clipSegmentToRect(double x0, double y0, double x1, double y1,
                       double xmin, double ymin, double xmax, double ymax,
                       double& outX, double& outY, bool preferTip)
{
    double t0 = 0.0;
    double t1 = 1.0;
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    auto clipT = [&](double p, double q) -> bool {
        if (std::abs(p) < 1e-12) {
            return q >= 0.0;
        }
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
    if (!clipT(-dx, x0 - xmin)) return false;
    if (!clipT(dx, xmax - x0)) return false;
    if (!clipT(-dy, y0 - ymin)) return false;
    if (!clipT(dy, ymax - y0)) return false;
    if (t0 > t1) return false;
    const double t = preferTip ? t1 : (t0 + t1) * 0.5;
    outX = x0 + t * dx;
    outY = y0 + t * dy;
    return true;
}

void worldPointToQtScreen(vtkRenderer* renderer, int vtkH, const gp_Pnt& p, double& qx, double& qy)
{
    renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
    renderer->WorldToDisplay();
    double disp[3] = {0, 0, 0};
    renderer->GetDisplayPoint(disp);
    qx = disp[0];
    qy = static_cast<double>(vtkH) - disp[1];
}

// 轴端输入框锚点：优先轴端；超出视口时贴在原点→轴端线段与窗口的交点处
void computeCuboidDimOverlayPosition(vtkRenderer* renderer,
                                     int vtkW,
                                     int vtkH,
                                     int overlayW,
                                     int overlayH,
                                     const gp_Pnt& axisOrigin,
                                     const gp_Pnt& axisTip,
                                     int& posX,
                                     int& posY)
{
    constexpr int kMargin = 8;
    constexpr int kOffsetBelow = 12;
    const double xmin = kMargin;
    const double ymin = kMargin;
    const double xmax = std::max(xmin + 1.0, static_cast<double>(vtkW) - kMargin);
    const double ymax = std::max(ymin + 1.0, static_cast<double>(vtkH) - kMargin);

    double ox = 0.0, oy = 0.0, tx = 0.0, ty = 0.0;
    worldPointToQtScreen(renderer, vtkH, axisOrigin, ox, oy);
    worldPointToQtScreen(renderer, vtkH, axisTip, tx, ty);

    double ax = tx;
    double ay = ty;
    if (!clipSegmentToRect(ox, oy, tx, ty, xmin, ymin, xmax, ymax, ax, ay, true)) {
        if (ox >= xmin && ox <= xmax && oy >= ymin && oy <= ymax) {
            ax = ox;
            ay = oy;
        } else {
            ax = std::max(xmin, std::min(tx, xmax));
            ay = std::max(ymin, std::min(ty, ymax));
        }
    }

    posX = static_cast<int>(std::lround(ax)) - overlayW / 2;
    posY = static_cast<int>(std::lround(ay)) - overlayH - kOffsetBelow;
    posX = std::max(kMargin, std::min(posX, vtkW - overlayW - kMargin));
    posY = std::max(kMargin, std::min(posY, vtkH - overlayH - kMargin));
}

} // namespace

int Widget::cuboidAxisIndexFromGizmoPart(CuboidGizmoPart part)
{
    const int base = static_cast<int>(CuboidGizmoPart::AxisLength);
    const int p = static_cast<int>(part);
    if (p < base || p > static_cast<int>(CuboidGizmoPart::AxisHeight)) {
        return -1;
    }
    return p - base;
}

gp_Ax2 Widget::cuboidInteractiveAxisSystem() const
{
    const bool axisReversed = cuboidDialog ? cuboidDialog->isAxisReversed() : false;
    return buildCuboidAxisSystem(cuboidInteractiveOrigin_,
                                 currentAxisDirection,
                                 axisReversed,
                                 hasCustomVectorDir_,
                                 customVectorDir_);
}

gp_Dir Widget::cuboidInteractiveLengthDir() const
{
    return cuboidInteractiveAxisSystem().XDirection();
}

gp_Dir Widget::cuboidInteractiveWidthDir() const
{
    return cuboidInteractiveAxisSystem().YDirection();
}

gp_Dir Widget::cuboidInteractiveHeightDir() const
{
    return cuboidInteractiveAxisSystem().Direction();
}

void Widget::syncCuboidInteractiveFromDialog()
{
    if (!cuboidDialog) return;
    cuboidInteractiveLength_ = cuboidDialog->getLength();
    cuboidInteractiveWidth_ = cuboidDialog->getWidth();
    cuboidInteractiveHeight_ = cuboidDialog->getHeight();
    if (cuboidDialog->hasOriginPoint()) {
        double x = 0, y = 0, z = 0;
        cuboidDialog->getOriginPoint(x, y, z);
        cuboidInteractiveOrigin_ = gp_Pnt(x, y, z);
    }
}

void Widget::syncCuboidDialogFromInteractive()
{
    if (!cuboidDialog) return;
    cuboidDialog->setLength(cuboidInteractiveLength_);
    cuboidDialog->setWidth(cuboidInteractiveWidth_);
    cuboidDialog->setHeight(cuboidInteractiveHeight_);
    cuboidDialog->setOriginPoint(cuboidInteractiveOrigin_.X(),
                                 cuboidInteractiveOrigin_.Y(),
                                 cuboidInteractiveOrigin_.Z());
}

void Widget::clearCuboidInteractiveGizmo()
{
    if (!renderer) return;
    auto removeActor = [this](vtkActor* actor) {
        if (actor) {
            removeSceneActor(actor);
        }
    };
    removeActor(cuboidPreviewBoxActor_);
    cuboidPreviewBoxActor_ = nullptr;
    removeActor(cuboidGizmoOriginActor_);
    cuboidGizmoOriginActor_ = nullptr;
    for (int i = 0; i < 3; ++i) {
        removeActor(cuboidGizmoAxisLineActors_[i]);
        cuboidGizmoAxisLineActors_[i] = nullptr;
        removeActor(cuboidGizmoAxisArrowActors_[i]);
        cuboidGizmoAxisArrowActors_[i] = nullptr;
        if (cuboidGizmoAxisLabelActors_[i]) {
            removeSceneActor(cuboidGizmoAxisLabelActors_[i]);
            cuboidGizmoAxisLabelActors_[i] = nullptr;
        }
    }
    hideCuboidDimOverlays();
}

void Widget::hideCuboidDimOverlays()
{
    for (int i = 0; i < 3; ++i) {
        if (cuboidDimOverlayWidgets_[i]) {
            cuboidDimOverlayWidgets_[i]->hide();
        }
    }
}

void Widget::ensureCuboidDimOverlay(int axisIndex)
{
    if (axisIndex < 0 || axisIndex > 2 || !vtkWidget) return;
    if (cuboidDimOverlayWidgets_[axisIndex]) return;

    static const char* kLabels[] = {"长", "宽", "高"};
    QWidget* panel = new QWidget(vtkWidget);
    panel->setObjectName(QStringLiteral("cuboidDimOverlay_%1").arg(axisIndex));
    panel->setStyleSheet(
        QStringLiteral("QWidget { background: rgba(40,40,40,200); border-radius: 4px; }"
                       "QLabel { color: white; padding: 2px 4px; }"
                       "QLineEdit { background: white; color: black; min-width: 72px; padding: 2px; }"
                       "QPushButton { background: #4CAF50; color: white; border: none; min-width: 22px; }"));

    auto* lay = new QHBoxLayout(panel);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(4);
    auto* title = new QLabel(tr(kLabels[axisIndex]), panel);
    cuboidDimEdits_[axisIndex] = new QLineEdit(panel);
    cuboidDimEdits_[axisIndex]->setValidator(new QDoubleValidator(kCuboidMinDim, 1e6, 4, panel));
    auto* okBtn = new QPushButton(QStringLiteral("✓"), panel);
    okBtn->setFlat(true);
    lay->addWidget(title);
    lay->addWidget(cuboidDimEdits_[axisIndex]);
    lay->addWidget(okBtn);
    panel->adjustSize();

    const int capturedAxis = axisIndex;
    connect(cuboidDimEdits_[axisIndex], &QLineEdit::returnPressed, this, [this, capturedAxis]() {
        if (!cuboidDimEdits_[capturedAxis]) return;
        bool ok = false;
        const double v = cuboidDimEdits_[capturedAxis]->text().toDouble(&ok);
        if (!ok || v < kCuboidMinDim) return;
        if (capturedAxis == 0) cuboidInteractiveLength_ = v;
        else if (capturedAxis == 1) cuboidInteractiveWidth_ = v;
        else cuboidInteractiveHeight_ = v;
        syncCuboidDialogFromInteractive();
        updateCuboidInteractivePreview();
    });
    connect(cuboidDimEdits_[axisIndex], &QLineEdit::textEdited, this, [this, capturedAxis](const QString& text) {
        bool ok = false;
        const double v = text.toDouble(&ok);
        if (!ok || v < kCuboidMinDim) return;
        if (capturedAxis == 0) cuboidInteractiveLength_ = v;
        else if (capturedAxis == 1) cuboidInteractiveWidth_ = v;
        else cuboidInteractiveHeight_ = v;
        syncCuboidDialogFromInteractive();
        updateCuboidInteractivePreview();
    });
    connect(okBtn, &QPushButton::clicked, this, [this, capturedAxis]() {
        if (!cuboidDimEdits_[capturedAxis]) return;
        bool ok = false;
        const double v = cuboidDimEdits_[capturedAxis]->text().toDouble(&ok);
        if (!ok || v < kCuboidMinDim) return;
        if (capturedAxis == 0) cuboidInteractiveLength_ = v;
        else if (capturedAxis == 1) cuboidInteractiveWidth_ = v;
        else cuboidInteractiveHeight_ = v;
        syncCuboidDialogFromInteractive();
        updateCuboidInteractivePreview();
    });

    cuboidDimOverlayWidgets_[axisIndex] = panel;
}

void Widget::updateCuboidDimOverlays()
{
    if (!cuboidInteractiveActive_ || !renderer || !vtkWidget) {
        hideCuboidDimOverlays();
        return;
    }

    if (cuboidActiveDimAxis_ < 0 || cuboidActiveDimAxis_ > 2) {
        hideCuboidDimOverlays();
        return;
    }

    const gp_Ax2 ax2 = cuboidInteractiveAxisSystem();
    const gp_Pnt O = ax2.Location();
    const gp_Dir dirs[3] = {ax2.XDirection(), ax2.YDirection(), ax2.Direction()};
    const double dims[3] = {cuboidInteractiveLength_, cuboidInteractiveWidth_, cuboidInteractiveHeight_};

    for (int i = 0; i < 3; ++i) {
        if (i != cuboidActiveDimAxis_) {
            if (cuboidDimOverlayWidgets_[i]) {
                cuboidDimOverlayWidgets_[i]->hide();
            }
            continue;
        }

        ensureCuboidDimOverlay(i);
        if (!cuboidDimOverlayWidgets_[i]) continue;

        const gp_Pnt tip = O.Translated(gp_Vec(dirs[i]) * dims[i]);
        const int overlayW = cuboidDimOverlayWidgets_[i]->width();
        const int overlayH = cuboidDimOverlayWidgets_[i]->height();
        const int vtkW = vtkWidget->width();
        const int vtkH = vtkWidget->height();
        int posX = 0;
        int posY = 0;
        computeCuboidDimOverlayPosition(renderer, vtkW, vtkH, overlayW, overlayH, O, tip, posX, posY);

        if (cuboidDimEdits_[i] && !cuboidDimEdits_[i]->hasFocus()) {
            QSignalBlocker blk(cuboidDimEdits_[i]);
            cuboidDimEdits_[i]->setText(QString::number(dims[i], 'f', 4));
        }
        cuboidDimOverlayWidgets_[i]->move(posX, posY);
        cuboidDimOverlayWidgets_[i]->raise();
        cuboidDimOverlayWidgets_[i]->show();
    }
}

void Widget::setCuboidAxisHighlight(int axisIndex, bool active)
{
    if (axisIndex < 0 || axisIndex > 2) return;
    vtkActor* line = cuboidGizmoAxisLineActors_[axisIndex];
    vtkActor* arrow = cuboidGizmoAxisArrowActors_[axisIndex];
    if (!line || !arrow) return;
    const ControlState st = active ? ControlState::Drag : ControlState::Default;
    const HandleStateStyle tipStyle = cuboidAxisTipStyle(axisIndex, st);
    HandleGeom::applyStateStyle(arrow->GetProperty(), tipStyle);
    HandleGeom::applyStateStyle(line->GetProperty(), tipStyle);
}

void Widget::updateCuboidInteractivePreview()
{
    if (!cuboidInteractiveActive_ || !renderer) return;

    clearCuboidInteractiveGizmo();

    const gp_Ax2 ax2 = cuboidInteractiveAxisSystem();
    const gp_Pnt O = ax2.Location();
    const gp_Dir xDir = ax2.XDirection();
    const gp_Dir yDir = ax2.YDirection();
    const gp_Dir zDir = ax2.Direction();

    // 半透明实体预览 + 操作柄
    const bool axisReversed = cuboidDialog ? cuboidDialog->isAxisReversed() : false;
    TopoDS_Shape boxShape = PrimitiveGeometry::buildCuboidShape(cuboidInteractiveLength_,
                                                                 cuboidInteractiveWidth_,
                                                                 cuboidInteractiveHeight_,
                                                                 true,
                                                                 O.X(), O.Y(), O.Z(),
                                                                 currentAxisDirection,
                                                                 axisReversed,
                                                                 hasCustomVectorDir_,
                                                                 customVectorDir_);
    if (!boxShape.IsNull()) {
        vtkSmartPointer<vtkPolyData> poly = m_occConverter.convert(boxShape);
        if (poly && poly->GetNumberOfPoints() > 0) {
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(poly);
            cuboidPreviewBoxActor_ = vtkSmartPointer<vtkActor>::New();
            cuboidPreviewBoxActor_->SetMapper(mapper);
            cuboidPreviewBoxActor_->GetProperty()->SetColor(0.55, 0.78, 0.95);
            cuboidPreviewBoxActor_->GetProperty()->SetOpacity(0.55);
            cuboidPreviewBoxActor_->GetProperty()->SetLighting(true);
            cuboidPreviewBoxActor_->GetProperty()->SetBackfaceCulling(false);
            cuboidPreviewBoxActor_->SetPickable(false);
            addAppearanceActor(cuboidPreviewBoxActor_);
        }
    }

    // 原点点操作柄（球形，始终位于当前原点）
    {
        auto sphere = HandleGeom::makeSphereSource(HandleGeom::defaultHandleSphereParams(0.08));
        vtkSmartPointer<vtkPolyDataMapper> originMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        originMapper->SetInputConnection(sphere->GetOutputPort());
        cuboidGizmoOriginActor_ = vtkSmartPointer<vtkActor>::New();
        cuboidGizmoOriginActor_->SetMapper(originMapper);
        cuboidGizmoOriginActor_->SetPosition(O.X(), O.Y(), O.Z());
        const bool originDrag = cuboidDragActive_ && cuboidDragPart_ == CuboidGizmoPart::Origin;
        const bool originHover = !cuboidDragActive_ && cuboidHoverPart_ == CuboidGizmoPart::Origin;
        const ControlState ost = HandleGeom::resolveControlState(originDrag, false, originHover);
        HandleGeom::applyStateStyle(cuboidGizmoOriginActor_,
                                    cuboidOriginStyle(ost),
                                    overlayWorldScaleAt(O.X(), O.Y(), O.Z()),
                                    true);
        cuboidGizmoOriginActor_->SetPickable(true);
        addReferenceActor(cuboidGizmoOriginActor_);
    }

    const gp_Dir dirs[3] = {xDir, yDir, zDir};
    const double dims[3] = {cuboidInteractiveLength_, cuboidInteractiveWidth_, cuboidInteractiveHeight_};
    static const char* kAxisLetters[] = {"X", "Y", "Z"};
    const double gizmoScale = overlayWorldScaleAt(O.X(), O.Y(), O.Z());

    for (int i = 0; i < 3; ++i) {
        const gp_Pnt tip = O.Translated(gp_Vec(dirs[i]) * dims[i]);
        const double tipScale = overlayWorldScaleAt(tip.X(), tip.Y(), tip.Z());
        const CuboidGizmoPart axisPart =
            static_cast<CuboidGizmoPart>(static_cast<int>(CuboidGizmoPart::AxisLength) + i);
        const bool axisDrag = cuboidDragActive_ && cuboidDragPart_ == axisPart;
        const bool axisHover = !cuboidDragActive_ && cuboidHoverPart_ == axisPart;
        const ControlState ast = HandleGeom::resolveControlState(axisDrag, false, axisHover);
        const HandleStateStyle tipStyle = cuboidAxisTipStyle(i, ast);

        vtkSmartPointer<vtkLineSource> lineSrc = vtkSmartPointer<vtkLineSource>::New();
        lineSrc->SetPoint1(O.X(), O.Y(), O.Z());
        lineSrc->SetPoint2(tip.X(), tip.Y(), tip.Z());
        vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        lineMapper->SetInputConnection(lineSrc->GetOutputPort());
        cuboidGizmoAxisLineActors_[i] = vtkSmartPointer<vtkActor>::New();
        cuboidGizmoAxisLineActors_[i]->SetMapper(lineMapper);
        HandleGeom::applyStateStyle(cuboidGizmoAxisLineActors_[i]->GetProperty(), tipStyle);
        cuboidGizmoAxisLineActors_[i]->GetProperty()->SetLighting(false);
        cuboidGizmoAxisLineActors_[i]->SetPickable(false);
        addReferenceActor(cuboidGizmoAxisLineActors_[i]);

        // 轴端：箭头形不含箭柄（文档 3.1.4），屏幕恒定尺寸
        const double arrowLen = 0.45 * tipStyle.scaleFactor * tipScale;
        auto arrowFilter = HandleGeom::makeOrientedArrow(
            ControlShape::ArrowHeadOnly, tip, dirs[i], arrowLen,
            HandleGeom::defaultHeadOnlyArrowParams());
        vtkSmartPointer<vtkPolyDataMapper> arrowMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        arrowMapper->SetInputConnection(arrowFilter->GetOutputPort());
        cuboidGizmoAxisArrowActors_[i] = vtkSmartPointer<vtkActor>::New();
        cuboidGizmoAxisArrowActors_[i]->SetMapper(arrowMapper);
        HandleGeom::applyStateStyle(cuboidGizmoAxisArrowActors_[i]->GetProperty(), tipStyle);
        cuboidGizmoAxisArrowActors_[i]->SetPickable(true);
        addReferenceActor(cuboidGizmoAxisArrowActors_[i]);

        vtkSmartPointer<vtkVectorText> text = vtkSmartPointer<vtkVectorText>::New();
        text->SetText(kAxisLetters[i]);
        vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        textMapper->SetInputConnection(text->GetOutputPort());
        cuboidGizmoAxisLabelActors_[i] = vtkSmartPointer<vtkFollower>::New();
        cuboidGizmoAxisLabelActors_[i]->SetMapper(textMapper);
        const gp_Pnt labelPos = O.Translated(gp_Vec(dirs[i]) * (dims[i] * 0.55));
        cuboidGizmoAxisLabelActors_[i]->SetPosition(labelPos.X(), labelPos.Y(), labelPos.Z());
        cuboidGizmoAxisLabelActors_[i]->SetScale(0.15 * gizmoScale, 0.15 * gizmoScale, 0.15 * gizmoScale);
        cuboidGizmoAxisLabelActors_[i]->GetProperty()->SetColor(0.2, 0.35, 0.85);
        cuboidGizmoAxisLabelActors_[i]->SetPickable(false);
        if (renderer->GetActiveCamera()) {
            cuboidGizmoAxisLabelActors_[i]->SetCamera(renderer->GetActiveCamera());
        }
        addReferenceActor(cuboidGizmoAxisLabelActors_[i]);
    }

    updateCuboidDimOverlays();

    refreshCameraClippingRange();

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::startCuboidInteractiveMode()
{
    cuboidInteractiveActive_ = true;
    cuboidHoverPart_ = CuboidGizmoPart::None;
    cuboidDragPart_ = CuboidGizmoPart::None;
    cuboidDragActive_ = false;

    syncCuboidInteractiveFromDialog();
    if (!cuboidDialog || !cuboidDialog->hasOriginPoint()) {
        cuboidInteractiveOrigin_ = gp_Pnt(0, 0, 0);
        if (cuboidDialog) {
            cuboidDialog->setOriginPoint(0, 0, 0);
        }
    }

    currentSelectionMode = CuboidInteractive;
    updateCuboidInteractivePreview();

    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

void Widget::stopCuboidInteractiveMode()
{
    cuboidInteractiveActive_ = false;
    cuboidDragActive_ = false;
    cuboidHoverPart_ = CuboidGizmoPart::None;
    cuboidDragPart_ = CuboidGizmoPart::None;
    cuboidActiveDimAxis_ = -1;
    clearCuboidInteractiveGizmo();

    for (int i = 0; i < 3; ++i) {
        if (cuboidDimOverlayWidgets_[i]) {
            cuboidDimOverlayWidgets_[i]->deleteLater();
            cuboidDimOverlayWidgets_[i] = nullptr;
            cuboidDimEdits_[i] = nullptr;
        }
    }

    if (currentSelectionMode == CuboidInteractive) {
        currentSelectionMode = None;
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::applyCuboidInteractiveOrigin(const gp_Pnt& p)
{
    cuboidInteractiveOrigin_ = p;
    if (cuboidDialog) {
        cuboidDialog->setOriginPoint(p.X(), p.Y(), p.Z());
    }
    updateCuboidInteractivePreview();
    if (cuboidInteractiveActive_ && currentSelectionMode != PointSelection && !originSnapSelectionActive_) {
        currentSelectionMode = CuboidInteractive;
    }
}

gp_Pnt Widget::cuboidProjectMouseOnViewPlane(int x, int y, const gp_Pnt& planePoint) const
{
    if (!renderer) return planePoint;

    vtkCamera* cam = renderer->GetActiveCamera();
    if (!cam) return planePoint;

    double dop[3] = {0, 0, -1};
    cam->GetDirectionOfProjection(dop);
    gp_Dir n(dop[0], dop[1], dop[2]);
    gp_Pln pln(planePoint, n);

    double worldNear[4] = {0, 0, 0, 1};
    double worldFar[4] = {0, 0, 1, 1};
    renderer->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(worldNear);
    renderer->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(worldFar);

    if (std::abs(worldNear[3]) > 1e-10) {
        worldNear[0] /= worldNear[3];
        worldNear[1] /= worldNear[3];
        worldNear[2] /= worldNear[3];
    }
    if (std::abs(worldFar[3]) > 1e-10) {
        worldFar[0] /= worldFar[3];
        worldFar[1] /= worldFar[3];
        worldFar[2] /= worldFar[3];
    }

    gp_Pnt rayO(worldNear[0], worldNear[1], worldNear[2]);
    gp_Vec rayV(rayO, gp_Pnt(worldFar[0], worldFar[1], worldFar[2]));
    if (rayV.Magnitude() <= Precision::Confusion()) return planePoint;

    IntAna_IntConicQuad intersector(gp_Lin(rayO, gp_Dir(rayV)), pln, Precision::Confusion());
    if (!intersector.IsDone() || intersector.NbPoints() < 1) return planePoint;
    return intersector.Point(1);
}

bool Widget::cuboidPickGizmoPart(int x, int y, CuboidGizmoPart& outPart) const
{
    outPart = CuboidGizmoPart::None;
    if (!cuboidInteractiveActive_ || !renderer) return false;

    auto screenDist2 = [this](const gp_Pnt& p, int mx, int my) {
        renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
        renderer->WorldToDisplay();
        double disp[3] = {0, 0, 0};
        renderer->GetDisplayPoint(disp);
        const double dx = disp[0] - mx;
        const double dy = disp[1] - my;
        return dx * dx + dy * dy;
    };

    const gp_Ax2 ax2 = cuboidInteractiveAxisSystem();
    const gp_Pnt O = ax2.Location();
    const gp_Dir dirs[3] = {ax2.XDirection(), ax2.YDirection(), ax2.Direction()};
    const double dims[3] = {cuboidInteractiveLength_, cuboidInteractiveWidth_, cuboidInteractiveHeight_};

    const double pickR2 = kCuboidPickRadiusPx * kCuboidPickRadiusPx;
    double bestD2 = pickR2;
    CuboidGizmoPart best = CuboidGizmoPart::None;

    const double dOrigin = screenDist2(O, x, y);
    if (dOrigin <= bestD2) {
        bestD2 = dOrigin;
        best = CuboidGizmoPart::Origin;
    }

    for (int i = 0; i < 3; ++i) {
        const gp_Pnt tip = O.Translated(gp_Vec(dirs[i]) * dims[i]);
        const double dTip = screenDist2(tip, x, y);
        if (dTip <= bestD2) {
            bestD2 = dTip;
            best = static_cast<CuboidGizmoPart>(static_cast<int>(CuboidGizmoPart::AxisLength) + i);
        }
        const gp_Pnt mid = O.Translated(gp_Vec(dirs[i]) * (dims[i] * 0.85));
        const double dMid = screenDist2(mid, x, y);
        if (dMid <= bestD2) {
            bestD2 = dMid;
            best = static_cast<CuboidGizmoPart>(static_cast<int>(CuboidGizmoPart::AxisLength) + i);
        }
    }

    if (best != CuboidGizmoPart::None) {
        outPart = best;
        return true;
    }
    return false;
}

double Widget::cuboidScreenCoordAlongAxis(int x, int y, double ox, double oy, double ax, double ay) const
{
    const double lenSq = ax * ax + ay * ay;
    if (lenSq < 1e-6) return 0.0;
    const double mx = static_cast<double>(x) - ox;
    const double my = static_cast<double>(y) - oy;
    return (mx * ax + my * ay) / lenSq;
}

void Widget::cuboidBeginAxisDragFrame(int x, int y, const gp_Pnt& origin, const gp_Dir& axisDir, double currentDim)
{
    cuboidDragStartDim_ = currentDim;
    if (!renderer) {
        cuboidDragAxisScrOx_ = cuboidDragAxisScrOy_ = cuboidDragAxisScrDx_ = cuboidDragAxisScrDy_ = 0.0;
        cuboidDragStartParam_ = 0.0;
        return;
    }

    auto worldToDisplay = [this](const gp_Pnt& p, double& dx, double& dy) {
        renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
        renderer->WorldToDisplay();
        double d[3] = {0, 0, 0};
        renderer->GetDisplayPoint(d);
        dx = d[0];
        dy = d[1];
    };

    const double refLen = std::max(currentDim, kCuboidMinDim);
    const gp_Pnt tip = origin.Translated(gp_Vec(axisDir) * refLen);
    worldToDisplay(origin, cuboidDragAxisScrOx_, cuboidDragAxisScrOy_);
    double tx = 0.0, ty = 0.0;
    worldToDisplay(tip, tx, ty);
    cuboidDragAxisScrDx_ = tx - cuboidDragAxisScrOx_;
    cuboidDragAxisScrDy_ = ty - cuboidDragAxisScrOy_;

    cuboidDragStartParam_ = cuboidScreenCoordAlongAxis(x, y,
                                                       cuboidDragAxisScrOx_, cuboidDragAxisScrOy_,
                                                       cuboidDragAxisScrDx_, cuboidDragAxisScrDy_);
}

Qt::CursorShape Widget::cuboidCursorForAxis(const gp_Dir& axisDir) const
{
    if (!renderer) return Qt::SizeVerCursor;

    const gp_Pnt O(0, 0, 0);
    const gp_Pnt E(axisDir.X(), axisDir.Y(), axisDir.Z());
    renderer->SetWorldPoint(O.X(), O.Y(), O.Z(), 1.0);
    renderer->WorldToDisplay();
    double d0[3] = {0, 0, 0};
    renderer->GetDisplayPoint(d0);
    renderer->SetWorldPoint(E.X(), E.Y(), E.Z(), 1.0);
    renderer->WorldToDisplay();
    double d1[3] = {0, 0, 0};
    renderer->GetDisplayPoint(d1);

    const double dx = d1[0] - d0[0];
    const double dy = d1[1] - d0[1];
    const double ax = std::abs(dx);
    const double ay = std::abs(dy);
    if (ax > ay * 1.8) return Qt::SizeHorCursor;
    if (ay > ax * 1.8) return Qt::SizeVerCursor;
    return (dx * dy >= 0) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
}

void Widget::handleCuboidInteractiveMouseMove(int x, int y)
{
    if (!cuboidInteractiveActive_) return;

    if (cuboidDragActive_) {
        if (cuboidDragPart_ == CuboidGizmoPart::Origin) {
            const gp_Pnt p = cuboidProjectMouseOnViewPlane(x, y, cuboidInteractiveOrigin_);
            applyCuboidInteractiveOrigin(p);
            if (vtkWidget) vtkWidget->setCursor(Qt::SizeAllCursor);
        } else {
            gp_Dir dir = cuboidInteractiveLengthDir();
            if (cuboidDragPart_ == CuboidGizmoPart::AxisWidth) dir = cuboidInteractiveWidthDir();
            else if (cuboidDragPart_ == CuboidGizmoPart::AxisHeight) dir = cuboidInteractiveHeightDir();

            // 沿拖拽开始时固化的屏幕轴方向做相对位移，保证与鼠标同向
            const double tNow = cuboidScreenCoordAlongAxis(x, y,
                                                           cuboidDragAxisScrOx_, cuboidDragAxisScrOy_,
                                                           cuboidDragAxisScrDx_, cuboidDragAxisScrDy_);
            const double t0 = cuboidDragStartParam_;
            const double scale = (std::abs(t0) > 1e-6) ? (cuboidDragStartDim_ / t0) : cuboidDragStartDim_;
            const double dim = applyCuboidDimCr(
                cuboidAxisIndexFromGizmoPart(cuboidDragPart_),
                cuboidDragStartDim_ + (tNow - t0) * scale);

            if (cuboidDragPart_ == CuboidGizmoPart::AxisLength) cuboidInteractiveLength_ = dim;
            else if (cuboidDragPart_ == CuboidGizmoPart::AxisWidth) cuboidInteractiveWidth_ = dim;
            else cuboidInteractiveHeight_ = dim;

            syncCuboidDialogFromInteractive();
            updateCuboidInteractivePreview();
            if (vtkWidget) vtkWidget->setCursor(cuboidCursorForAxis(dir));
        }
        return;
    }

    CuboidGizmoPart part = CuboidGizmoPart::None;
    cuboidPickGizmoPart(x, y, part);
    if (part != cuboidHoverPart_) {
        cuboidHoverPart_ = part;
        // 仅刷新四状态样式，避免悬停时整框重建
        if (cuboidGizmoOriginActor_) {
            const bool originHover = cuboidHoverPart_ == CuboidGizmoPart::Origin;
            HandleGeom::applyStateStyle(
                cuboidGizmoOriginActor_,
                cuboidOriginStyle(originHover ? ControlState::Hover : ControlState::Default),
                overlayWorldScaleAt(cuboidInteractiveOrigin_.X(),
                                    cuboidInteractiveOrigin_.Y(),
                                    cuboidInteractiveOrigin_.Z()),
                true);
        }
        for (int i = 0; i < 3; ++i) {
            const CuboidGizmoPart axisPart =
                static_cast<CuboidGizmoPart>(static_cast<int>(CuboidGizmoPart::AxisLength) + i);
            const bool axisHover = cuboidHoverPart_ == axisPart;
            const HandleStateStyle tipStyle =
                cuboidAxisTipStyle(i, axisHover ? ControlState::Hover : ControlState::Default);
            if (cuboidGizmoAxisLineActors_[i]) {
                HandleGeom::applyStateStyle(cuboidGizmoAxisLineActors_[i]->GetProperty(), tipStyle);
            }
            if (cuboidGizmoAxisArrowActors_[i]) {
                HandleGeom::applyStateStyle(cuboidGizmoAxisArrowActors_[i]->GetProperty(), tipStyle);
            }
        }
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }

    if (vtkWidget) {
        if (part == CuboidGizmoPart::Origin) {
            vtkWidget->setCursor(Qt::SizeAllCursor);
        } else if (part == CuboidGizmoPart::AxisLength) {
            vtkWidget->setCursor(cuboidCursorForAxis(cuboidInteractiveLengthDir()));
        } else if (part == CuboidGizmoPart::AxisWidth) {
            vtkWidget->setCursor(cuboidCursorForAxis(cuboidInteractiveWidthDir()));
        } else if (part == CuboidGizmoPart::AxisHeight) {
            vtkWidget->setCursor(cuboidCursorForAxis(cuboidInteractiveHeightDir()));
        } else {
            vtkWidget->unsetCursor();
        }
    }
}

void Widget::handleCuboidInteractiveMouseDown(int x, int y)
{
    if (!cuboidInteractiveActive_) return;

    CuboidGizmoPart part = CuboidGizmoPart::None;
    if (!cuboidPickGizmoPart(x, y, part)) return;

    cuboidDragActive_ = true;
    cuboidDragPart_ = part;
    cuboidHoverPart_ = part;

    if (part == CuboidGizmoPart::Origin) {
        cuboidDragStartParam_ = 0.0;
    } else {
        const int axisIdx = cuboidAxisIndexFromGizmoPart(part);
        if (axisIdx >= 0) {
            cuboidActiveDimAxis_ = axisIdx;
        }
        gp_Dir dir = cuboidInteractiveLengthDir();
        double dim = cuboidInteractiveLength_;
        if (part == CuboidGizmoPart::AxisWidth) {
            dir = cuboidInteractiveWidthDir();
            dim = cuboidInteractiveWidth_;
        } else if (part == CuboidGizmoPart::AxisHeight) {
            dir = cuboidInteractiveHeightDir();
            dim = cuboidInteractiveHeight_;
        }
        cuboidBeginAxisDragFrame(x, y, cuboidInteractiveOrigin_, dir, dim);
    }
    updateCuboidInteractivePreview();
}

void Widget::handleCuboidInteractiveMouseUp(int x, int y)
{
    if (!cuboidDragActive_) return;

    const int releasedAxis = cuboidAxisIndexFromGizmoPart(cuboidDragPart_);
    cuboidDragActive_ = false;
    cuboidDragPart_ = CuboidGizmoPart::None;

    updateCuboidInteractivePreview();
    handleCuboidInteractiveMouseMove(x, y);

    // 松开后保持当前轴输入框，便于直接输入数值
    if (releasedAxis >= 0 && cuboidDimEdits_[releasedAxis]) {
        const int axis = releasedAxis;
        QTimer::singleShot(0, this, [this, axis]() {
            if (!cuboidInteractiveActive_ || !cuboidDimEdits_[axis]) return;
            cuboidDimEdits_[axis]->setFocus(Qt::MouseFocusReason);
            cuboidDimEdits_[axis]->selectAll();
        });
    }
}
