// 默认参考坐标系（原点基准 CSYS）：VTK 三轴箭头 + 三主平面
// 平面点击 → 带动画切到对应正交视图；箭头在矢量选择时可指定方向
#include "main_window.h"
#include "rendering/handles/handle_geometry.h"
#include "presentation/dialogs/tools/vector_dialog.h"

#include <QDateTime>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <gp_Dir.hxx>

#include <cmath>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkPlaneSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

namespace {

constexpr double kRefCsysScale = 0.55;
constexpr double kRefPlaneSize = 0.42; // 相对单位，再乘 kRefCsysScale
constexpr double kRefAxisPickRadiusPx = 18.0;

// 经典 RGB 轴色
constexpr double kAxisXR = 0.90, kAxisXG = 0.22, kAxisXB = 0.18;
constexpr double kAxisYR = 0.18, kAxisYG = 0.72, kAxisYB = 0.28;
constexpr double kAxisZR = 0.20, kAxisZG = 0.42, kAxisZB = 0.92;

// 平面半透明色（按法向轴着色）
constexpr double kPlaneXY_R = 0.25, kPlaneXY_G = 0.50, kPlaneXY_B = 0.95; // 法向 Z
constexpr double kPlaneXZ_R = 0.25, kPlaneXZ_G = 0.80, kPlaneXZ_B = 0.35; // 法向 Y
constexpr double kPlaneYZ_R = 0.92, kPlaneYZ_G = 0.30, kPlaneYZ_B = 0.25; // 法向 X
constexpr double kPlaneOpacity = 0.28;
constexpr double kPlaneHoverOpacity = 0.55;

enum class RefCsysPlane { XY, YZ, XZ };

void worldToQtScreen(vtkRenderer* renderer, int vtkH, const gp_Pnt& p, double& qx, double& qy)
{
    renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
    renderer->WorldToDisplay();
    double d[3] = {0, 0, 0};
    renderer->GetDisplayPoint(d);
    qx = d[0];
    qy = static_cast<double>(vtkH) - d[1];
}

gp_Dir axisDirectionToGpDir(AxisDirection axis)
{
    switch (axis) {
    case AxisDirection::X: return gp_Dir(1, 0, 0);
    case AxisDirection::Y: return gp_Dir(0, 1, 0);
    case AxisDirection::Z:
    default: return gp_Dir(0, 0, 1);
    }
}

void axisRgb(AxisDirection axis, double& r, double& g, double& b)
{
    switch (axis) {
    case AxisDirection::X: r = kAxisXR; g = kAxisXG; b = kAxisXB; break;
    case AxisDirection::Y: r = kAxisYR; g = kAxisYG; b = kAxisYB; break;
    case AxisDirection::Z:
    default: r = kAxisZR; g = kAxisZG; b = kAxisZB; break;
    }
}

void planeRgb(RefCsysPlane plane, double& r, double& g, double& b)
{
    switch (plane) {
    case RefCsysPlane::XY: r = kPlaneXY_R; g = kPlaneXY_G; b = kPlaneXY_B; break;
    case RefCsysPlane::XZ: r = kPlaneXZ_R; g = kPlaneXZ_G; b = kPlaneXZ_B; break;
    case RefCsysPlane::YZ:
    default: r = kPlaneYZ_R; g = kPlaneYZ_G; b = kPlaneYZ_B; break;
    }
}

vtkSmartPointer<vtkActor> makeRefAxisArrowActor(AxisDirection axis, vtkTransform* userXf)
{
    HandleGeom::ArrowParams params = HandleGeom::defaultShaftArrowParams();
    params.tipLength = 0.28;
    params.tipRadius = 0.075;
    params.shaftRadius = 0.018;
    params.tipResolution = 24;
    params.shaftResolution = 16;

    gp_Dir dir = axisDirectionToGpDir(axis);
    auto oriented = HandleGeom::makeOrientedArrow(
        ControlShape::ArrowWithShaft,
        gp_Pnt(0, 0, 0),
        dir,
        kRefCsysScale,
        params);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(oriented->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (userXf) actor->SetUserTransform(userXf);
    double r = 0, g = 0, b = 0;
    axisRgb(axis, r, g, b);
    actor->GetProperty()->SetColor(r, g, b);
    actor->GetProperty()->SetOpacity(1.0);
    actor->GetProperty()->SetAmbient(0.85);
    actor->GetProperty()->SetDiffuse(0.35);
    actor->GetProperty()->SetSpecular(0.05);
    actor->SetPickable(1);
    return actor;
}

vtkSmartPointer<vtkActor> makeRefAxisLabelActor(const char* text,
                                                double x, double y, double z,
                                                AxisDirection axis,
                                                vtkTransform* userXf)
{
    vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(text);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(textSource->GetOutputPort());

    vtkSmartPointer<vtkActor> labelActor = vtkSmartPointer<vtkActor>::New();
    labelActor->SetMapper(mapper);
    labelActor->SetScale(0.18 * kRefCsysScale);
    labelActor->SetPosition(x * kRefCsysScale, y * kRefCsysScale, z * kRefCsysScale);
    if (userXf) labelActor->SetUserTransform(userXf);
    double r = 0, g = 0, b = 0;
    axisRgb(axis, r, g, b);
    labelActor->GetProperty()->SetColor(r, g, b);
    labelActor->GetProperty()->SetAmbient(1.0);
    labelActor->GetProperty()->SetDiffuse(0.0);
    labelActor->GetProperty()->SetLighting(false);
    labelActor->SetPickable(0);
    return labelActor;
}

/** 第一卦限四分之一平面：原点 → 两正半轴 */
vtkSmartPointer<vtkActor> makeRefPlaneActor(RefCsysPlane plane, vtkTransform* userXf)
{
    const double s = kRefPlaneSize;
    vtkSmartPointer<vtkPlaneSource> src = vtkSmartPointer<vtkPlaneSource>::New();
    switch (plane) {
    case RefCsysPlane::XY:
        src->SetOrigin(0, 0, 0);
        src->SetPoint1(s, 0, 0);
        src->SetPoint2(0, s, 0);
        break;
    case RefCsysPlane::XZ:
        src->SetOrigin(0, 0, 0);
        src->SetPoint1(s, 0, 0);
        src->SetPoint2(0, 0, s);
        break;
    case RefCsysPlane::YZ:
    default:
        src->SetOrigin(0, 0, 0);
        src->SetPoint1(0, s, 0);
        src->SetPoint2(0, 0, s);
        break;
    }
    src->SetXResolution(1);
    src->SetYResolution(1);
    src->Update();

    vtkSmartPointer<vtkTransform> local = vtkSmartPointer<vtkTransform>::New();
    local->Scale(kRefCsysScale, kRefCsysScale, kRefCsysScale);

    vtkSmartPointer<vtkTransformPolyDataFilter> tf =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    tf->SetTransform(local);
    tf->SetInputConnection(src->GetOutputPort());
    tf->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(tf->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (userXf) actor->SetUserTransform(userXf);
    double r = 0, g = 0, b = 0;
    planeRgb(plane, r, g, b);
    actor->GetProperty()->SetColor(r, g, b);
    actor->GetProperty()->SetOpacity(kPlaneOpacity);
    actor->GetProperty()->SetAmbient(0.75);
    actor->GetProperty()->SetDiffuse(0.40);
    actor->GetProperty()->SetSpecular(0.0);
    actor->GetProperty()->BackfaceCullingOff();
    actor->GetProperty()->SetRepresentationToSurface();
    actor->SetPickable(1);
    return actor;
}

void appendClosedSquare(vtkPoints* pts, vtkCellArray* lines,
                        const double p0[3], const double p1[3],
                        const double p2[3], const double p3[3])
{
    const vtkIdType i0 = pts->InsertNextPoint(p0);
    const vtkIdType i1 = pts->InsertNextPoint(p1);
    const vtkIdType i2 = pts->InsertNextPoint(p2);
    const vtkIdType i3 = pts->InsertNextPoint(p3);
    vtkIdType ids0[2] = {i0, i1};
    vtkIdType ids1[2] = {i1, i2};
    vtkIdType ids2[2] = {i2, i3};
    vtkIdType ids3[2] = {i3, i0};
    lines->InsertNextCell(2, ids0);
    lines->InsertNextCell(2, ids1);
    lines->InsertNextCell(2, ids2);
    lines->InsertNextCell(2, ids3);
}

/** 三平面外周闭合方框（原点角立方体的三面轮廓） */
vtkSmartPointer<vtkActor> makeRefPlaneFrameActor(vtkTransform* userXf)
{
    const double s = kRefPlaneSize;
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    // XY
    {
        const double a[3] = {0, 0, 0};
        const double b[3] = {s, 0, 0};
        const double c[3] = {s, s, 0};
        const double d[3] = {0, s, 0};
        appendClosedSquare(pts, lines, a, b, c, d);
    }
    // YZ
    {
        const double a[3] = {0, 0, 0};
        const double b[3] = {0, s, 0};
        const double c[3] = {0, s, s};
        const double d[3] = {0, 0, s};
        appendClosedSquare(pts, lines, a, b, c, d);
    }
    // XZ
    {
        const double a[3] = {0, 0, 0};
        const double b[3] = {s, 0, 0};
        const double c[3] = {s, 0, s};
        const double d[3] = {0, 0, s};
        appendClosedSquare(pts, lines, a, b, c, d);
    }

    vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
    pd->SetPoints(pts);
    pd->SetLines(lines);

    vtkSmartPointer<vtkTransform> local = vtkSmartPointer<vtkTransform>::New();
    local->Scale(kRefCsysScale, kRefCsysScale, kRefCsysScale);

    vtkSmartPointer<vtkTransformPolyDataFilter> tf =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    tf->SetTransform(local);
    tf->SetInputData(pd);
    tf->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(tf->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (userXf) actor->SetUserTransform(userXf);
    // 深棕框线，贴近参考图轮廓效果，并与半透明平面区分
    actor->GetProperty()->SetColor(0.42, 0.22, 0.16);
    actor->GetProperty()->SetOpacity(1.0);
    actor->GetProperty()->SetLineWidth(2.0);
    actor->GetProperty()->SetAmbient(1.0);
    actor->GetProperty()->SetDiffuse(0.0);
    actor->GetProperty()->SetLighting(false);
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->SetPickable(0);
    return actor;
}

} // namespace

bool Widget::isVectorAxisPickContext() const
{
    if (currentSelectionMode == VectorDialogPickDirection) {
        return true;
    }
    if (patternVectorPick_ != PatternVectorPick::None) {
        return true;
    }
    // 两点定矢量期间禁止坐标系轴抢点击，否则易误选到原点/轴向
    if (currentSelectionMode == VectorDialogPickStartPoint
        || currentSelectionMode == VectorDialogPickEndPoint
        || currentSelectionMode == VectorTwoPointInteractive
        || currentSelectionMode == VectorTwoPointHandleDrag) {
        return false;
    }
    // 矢量对话框打开期间均可点选基准坐标系轴向
    if (vectorDialog_ && vectorDialog_->isVisible()) {
        return true;
    }
    return false;
}

void Widget::applyVectorDirFromDatumAxis(const gp_Dir& baseDir)
{
    setCustomVectorDirFromDialog(baseDir);

    if (patternVectorPick_ != PatternVectorPick::None) {
        applyPatternVectorPick(customVectorDir_);
        clearVectorDialogHoverShape();
        return;
    }

    if (hasVectorDialogArrowOrigin_) {
        updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
    }
    if (vectorDialog_) {
        vectorDialog_->setVectorDirDisplay(
            customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
    }
    clearVectorDialogHoverShape();

    if (currentSelectionMode == VectorDialogPickDirection) {
        currentSelectionMode = None;
    }

    if (statusBar()) {
        statusBar()->showMessage(
            tr("已指定矢量方向: (%1, %2, %3)")
                .arg(customVectorDir_.X(), 0, 'f', 3)
                .arg(customVectorDir_.Y(), 0, 'f', 3)
                .arg(customVectorDir_.Z(), 0, 'f', 3),
            2000);
    }
}

bool Widget::ensureReferenceCsysActorsCreated()
{
    if (!renderer || !vtkWidget) {
        return false;
    }

    // 已有完整三轴+三平面，仅补框线
    if (refCsysAxisXActor_ && refCsysAxisYActor_ && refCsysAxisZActor_
        && refCsysLabelXActor_ && refCsysLabelYActor_ && refCsysLabelZActor_
        && refCsysPlaneXYActor_ && refCsysPlaneYZActor_ && refCsysPlaneXZActor_) {
        if (!refCsysPlaneFrameActor_) {
            if (!refCsysTransform_) {
                refCsysTransform_ = vtkSmartPointer<vtkTransform>::New();
                refCsysTransform_->Identity();
            }
            refCsysPlaneFrameActor_ = makeRefPlaneFrameActor(refCsysTransform_);
            addReferenceActor(refCsysPlaneFrameActor_);
        }
        return true;
    }

    if (!refCsysTransform_) {
        refCsysTransform_ = vtkSmartPointer<vtkTransform>::New();
        refCsysTransform_->Identity();
    }

    // 若仅缺平面/框线，补建
    if (refCsysAxisXActor_ && refCsysAxisYActor_ && refCsysAxisZActor_) {
        if (!refCsysPlaneXYActor_) {
            refCsysPlaneXYActor_ = makeRefPlaneActor(RefCsysPlane::XY, refCsysTransform_);
            addReferenceActor(refCsysPlaneXYActor_);
        }
        if (!refCsysPlaneYZActor_) {
            refCsysPlaneYZActor_ = makeRefPlaneActor(RefCsysPlane::YZ, refCsysTransform_);
            addReferenceActor(refCsysPlaneYZActor_);
        }
        if (!refCsysPlaneXZActor_) {
            refCsysPlaneXZActor_ = makeRefPlaneActor(RefCsysPlane::XZ, refCsysTransform_);
            addReferenceActor(refCsysPlaneXZActor_);
        }
        if (!refCsysPlaneFrameActor_) {
            refCsysPlaneFrameActor_ = makeRefPlaneFrameActor(refCsysTransform_);
            addReferenceActor(refCsysPlaneFrameActor_);
        }
        return true;
    }

    // 重建：先清旧 actor（避免残留）
    auto removeIf = [this](vtkSmartPointer<vtkActor>& a) {
        if (a) {
            removeSceneActor(a);
            a = nullptr;
        }
    };
    removeIf(refCsysAxisXActor_);
    removeIf(refCsysAxisYActor_);
    removeIf(refCsysAxisZActor_);
    removeIf(refCsysLabelXActor_);
    removeIf(refCsysLabelYActor_);
    removeIf(refCsysLabelZActor_);
    removeIf(refCsysPlaneXYActor_);
    removeIf(refCsysPlaneYZActor_);
    removeIf(refCsysPlaneXZActor_);
    removeIf(refCsysPlaneFrameActor_);

    refCsysAxisXActor_ = makeRefAxisArrowActor(AxisDirection::X, refCsysTransform_);
    refCsysAxisYActor_ = makeRefAxisArrowActor(AxisDirection::Y, refCsysTransform_);
    refCsysAxisZActor_ = makeRefAxisArrowActor(AxisDirection::Z, refCsysTransform_);

    refCsysLabelXActor_ = makeRefAxisLabelActor("X", 1.10, 0.0, 0.0, AxisDirection::X, refCsysTransform_);
    refCsysLabelYActor_ = makeRefAxisLabelActor("Y", 0.0, 1.10, 0.0, AxisDirection::Y, refCsysTransform_);
    refCsysLabelZActor_ = makeRefAxisLabelActor("Z", 0.0, 0.0, 1.10, AxisDirection::Z, refCsysTransform_);

    refCsysPlaneXYActor_ = makeRefPlaneActor(RefCsysPlane::XY, refCsysTransform_);
    refCsysPlaneYZActor_ = makeRefPlaneActor(RefCsysPlane::YZ, refCsysTransform_);
    refCsysPlaneXZActor_ = makeRefPlaneActor(RefCsysPlane::XZ, refCsysTransform_);
    refCsysPlaneFrameActor_ = makeRefPlaneFrameActor(refCsysTransform_);

    addReferenceActor(refCsysPlaneXYActor_);
    addReferenceActor(refCsysPlaneYZActor_);
    addReferenceActor(refCsysPlaneXZActor_);
    addReferenceActor(refCsysPlaneFrameActor_);
    addReferenceActor(refCsysAxisXActor_);
    addReferenceActor(refCsysAxisYActor_);
    addReferenceActor(refCsysAxisZActor_);
    addReferenceActor(refCsysLabelXActor_);
    addReferenceActor(refCsysLabelYActor_);
    addReferenceActor(refCsysLabelZActor_);

    refCsysActor_ = refCsysAxisXActor_;
    resetReferenceAxisHighlight();
    resetReferencePlaneHighlight();
    refreshReferenceCsysScreenScale();
    return true;
}

void Widget::setReferenceCsysVisible(bool visible)
{
    const int v = visible ? 1 : 0;
    if (refCsysAxisXActor_) refCsysAxisXActor_->SetVisibility(v);
    if (refCsysAxisYActor_) refCsysAxisYActor_->SetVisibility(v);
    if (refCsysAxisZActor_) refCsysAxisZActor_->SetVisibility(v);
    if (refCsysLabelXActor_) refCsysLabelXActor_->SetVisibility(v);
    if (refCsysLabelYActor_) refCsysLabelYActor_->SetVisibility(v);
    if (refCsysLabelZActor_) refCsysLabelZActor_->SetVisibility(v);
    if (refCsysPlaneXYActor_) refCsysPlaneXYActor_->SetVisibility(v);
    if (refCsysPlaneYZActor_) refCsysPlaneYZActor_->SetVisibility(v);
    if (refCsysPlaneXZActor_) refCsysPlaneXZActor_->SetVisibility(v);
    if (refCsysPlaneFrameActor_) refCsysPlaneFrameActor_->SetVisibility(v);
}

void Widget::refreshReferenceCsysScreenScale()
{
    if (!hasReferenceCsys_ || !refCsysTransform_) return;

    // 屏幕恒定尺寸：与操作柄相同，用 overlay 缩放乘到 UserTransform。
    // 注意：矩阵若已含 Scale，平移分量仍在第 4 列；先记下平移再重建，避免叠乘。
    double ox = 0.0, oy = 0.0, oz = 0.0;
    if (vtkMatrix4x4* m = refCsysTransform_->GetMatrix()) {
        ox = m->GetElement(0, 3);
        oy = m->GetElement(1, 3);
        oz = m->GetElement(2, 3);
    }
    const double s = overlayWorldScaleAt(ox, oy, oz);
    if (!(s > 1e-12) || !std::isfinite(s)) return;

    refCsysTransform_->Identity();
    if (std::abs(ox) > 1e-15 || std::abs(oy) > 1e-15 || std::abs(oz) > 1e-15) {
        refCsysTransform_->Translate(ox, oy, oz);
    }
    refCsysTransform_->Scale(s, s, s);
    refCsysTransform_->Modified();
}

void Widget::resetReferenceAxisHighlight()
{
    auto resetAxis = [&](vtkSmartPointer<vtkActor>& a, AxisDirection dir) {
        if (!a) return;
        double r = 0, g = 0, b = 0;
        axisRgb(dir, r, g, b);
        a->GetProperty()->SetColor(r, g, b);
        a->GetProperty()->SetOpacity(1.0);
    };
    resetAxis(refCsysAxisXActor_, AxisDirection::X);
    resetAxis(refCsysAxisYActor_, AxisDirection::Y);
    resetAxis(refCsysAxisZActor_, AxisDirection::Z);
}

void Widget::applyReferenceAxisHighlight(AxisDirection dir)
{
    resetReferenceAxisHighlight();

    vtkSmartPointer<vtkActor> target;
    if (dir == AxisDirection::X) target = refCsysAxisXActor_;
    else if (dir == AxisDirection::Y) target = refCsysAxisYActor_;
    else target = refCsysAxisZActor_;

    if (target) {
        target->GetProperty()->SetColor(1.0, 0.75, 0.15);
        target->GetProperty()->SetOpacity(1.0);
    }
}

void Widget::resetReferencePlaneHighlight()
{
    auto resetPlane = [&](vtkSmartPointer<vtkActor>& a, RefCsysPlane plane) {
        if (!a) return;
        double r = 0, g = 0, b = 0;
        planeRgb(plane, r, g, b);
        a->GetProperty()->SetColor(r, g, b);
        a->GetProperty()->SetOpacity(kPlaneOpacity);
    };
    resetPlane(refCsysPlaneXYActor_, RefCsysPlane::XY);
    resetPlane(refCsysPlaneYZActor_, RefCsysPlane::YZ);
    resetPlane(refCsysPlaneXZActor_, RefCsysPlane::XZ);
}

void Widget::applyReferencePlaneHighlight(int planeId)
{
    resetReferencePlaneHighlight();
    vtkSmartPointer<vtkActor> target;
    RefCsysPlane plane = RefCsysPlane::XY;
    if (planeId == 0) {
        target = refCsysPlaneXYActor_;
        plane = RefCsysPlane::XY;
    } else if (planeId == 1) {
        target = refCsysPlaneYZActor_;
        plane = RefCsysPlane::YZ;
    } else {
        target = refCsysPlaneXZActor_;
        plane = RefCsysPlane::XZ;
    }
    if (!target) return;
    double r = 0, g = 0, b = 0;
    planeRgb(plane, r, g, b);
    target->GetProperty()->SetColor(r, g, b);
    target->GetProperty()->SetOpacity(kPlaneHoverOpacity);
}

bool Widget::pickReferenceCsysAxisAt(int x, int y, AxisDirection& outAxis) const
{
    if (!renderer || !vtkWidget || !hasReferenceCsys_) {
        return false;
    }
    if (refCsysAxisXActor_ && refCsysAxisXActor_->GetVisibility() == 0) {
        return false;
    }

    const int vtkH = vtkWidget->height();
    const double mx = static_cast<double>(x);
    const double my = static_cast<double>(y);
    const gp_Pnt origin(0, 0, 0);
    const double tipScale = overlayWorldScaleAt(0.0, 0.0, 0.0);

    struct AxisCandidate {
        AxisDirection dir;
        gp_Pnt tip;
    };
    const AxisCandidate candidates[] = {
        {AxisDirection::X, gp_Pnt(kRefCsysScale * tipScale, 0.0, 0.0)},
        {AxisDirection::Y, gp_Pnt(0.0, kRefCsysScale * tipScale, 0.0)},
        {AxisDirection::Z, gp_Pnt(0.0, 0.0, kRefCsysScale * tipScale)},
    };

    double ox = 0.0, oy = 0.0;
    worldToQtScreen(renderer, vtkH, origin, ox, oy);

    const double pickR2 = kRefAxisPickRadiusPx * kRefAxisPickRadiusPx;
    double bestDist2 = pickR2;
    bool found = false;

    for (const AxisCandidate& c : candidates) {
        double tx = 0.0, ty = 0.0;
        worldToQtScreen(renderer, vtkH, c.tip, tx, ty);
        const double ax = tx - ox;
        const double ay = ty - oy;
        const double lenSq = ax * ax + ay * ay;
        if (lenSq < 1e-6) {
            continue;
        }
        double t = ((mx - ox) * ax + (my - oy) * ay) / lenSq;
        if (t < 0.12) {
            continue;
        }
        if (t > 1.05) t = 1.05;
        const double px = ox + t * ax;
        const double py = oy + t * ay;
        const double dx = mx - px;
        const double dy = my - py;
        const double d2 = dx * dx + dy * dy;
        if (d2 < bestDist2) {
            bestDist2 = d2;
            outAxis = c.dir;
            found = true;
        }
    }
    return found;
}

bool Widget::pickReferenceCsysPlaneAt(int x, int y, int& outPlaneId) const
{
    if (!renderer || !hasReferenceCsys_) return false;
    if (!refCsysPlaneXYActor_ || refCsysPlaneXYActor_->GetVisibility() == 0) return false;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.02);
    picker->PickFromListOn();
    picker->AddPickList(const_cast<vtkActor*>(refCsysPlaneXYActor_.GetPointer()));
    picker->AddPickList(const_cast<vtkActor*>(refCsysPlaneYZActor_.GetPointer()));
    picker->AddPickList(const_cast<vtkActor*>(refCsysPlaneXZActor_.GetPointer()));

    vtkRenderer* r = const_cast<Widget*>(this)->referenceOverlay()
                         ? const_cast<Widget*>(this)->referenceOverlay()
                         : renderer.GetPointer();
    picker->Pick(x, y, 0, r);
    vtkActor* picked = picker->GetActor();
    if (picked == refCsysPlaneXYActor_.GetPointer()) {
        outPlaneId = 0;
        return true;
    }
    if (picked == refCsysPlaneYZActor_.GetPointer()) {
        outPlaneId = 1;
        return true;
    }
    if (picked == refCsysPlaneXZActor_.GetPointer()) {
        outPlaneId = 2;
        return true;
    }
    return false;
}

QString Widget::viewNameForReferenceCsysPlane(int planeId) const
{
    // 按当前相机落在法向正/负侧，选择更“正面朝向”的正交视图
    double pos[3] = {0, 0, 1};
    double fp[3] = {0, 0, 0};
    if (renderer && renderer->GetActiveCamera()) {
        renderer->GetActiveCamera()->GetPosition(pos);
        renderer->GetActiveCamera()->GetFocalPoint(fp);
    }
    const double dx = pos[0] - fp[0];
    const double dy = pos[1] - fp[1];
    const double dz = pos[2] - fp[2];

    if (planeId == 0) { // XY，法向 Z
        return (dz >= 0.0) ? QStringLiteral("前视图") : QStringLiteral("后视图");
    }
    if (planeId == 1) { // YZ，法向 X
        return (dx >= 0.0) ? QStringLiteral("右视图") : QStringLiteral("左视图");
    }
    // XZ，法向 Y
    return (dy >= 0.0) ? QStringLiteral("俯视图") : QStringLiteral("仰视图");
}

void Widget::updateReferenceCsysAxisHover(int x, int y)
{
    if (!hasReferenceCsys_) return;
    if (!ensureReferenceCsysActorsCreated()) return;

    // 矢量上下文：优先高亮轴；否则高亮平面
    if (isVectorAxisPickContext()) {
        AxisDirection axis = AxisDirection::Z;
        if (pickReferenceCsysAxisAt(x, y, axis)) {
            applyReferenceAxisHighlight(axis);
            resetReferencePlaneHighlight();
        } else {
            resetReferenceAxisHighlight();
            int planeId = -1;
            if (pickReferenceCsysPlaneAt(x, y, planeId)) {
                applyReferencePlaneHighlight(planeId);
            } else {
                resetReferencePlaneHighlight();
            }
        }
    } else {
        resetReferenceAxisHighlight();
        int planeId = -1;
        if (pickReferenceCsysPlaneAt(x, y, planeId)) {
            applyReferencePlaneHighlight(planeId);
        } else {
            resetReferencePlaneHighlight();
        }
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

bool Widget::handleReferenceCsysAxisPick(int x, int y)
{
    if (!hasReferenceCsys_ || !isVectorAxisPickContext()) {
        return false;
    }
    if (!renderer) {
        return false;
    }
    if (!ensureReferenceCsysActorsCreated()) {
        return false;
    }
    if (refCsysAxisXActor_ && refCsysAxisXActor_->GetVisibility() == 0) {
        return false;
    }

    AxisDirection axis = AxisDirection::Z;
    bool hit = false;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.06);
    picker->PickFromListOn();
    if (refCsysAxisXActor_) picker->AddPickList(refCsysAxisXActor_);
    if (refCsysAxisYActor_) picker->AddPickList(refCsysAxisYActor_);
    if (refCsysAxisZActor_) picker->AddPickList(refCsysAxisZActor_);

    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* picked = picker->GetActor();
    if (picked == refCsysAxisXActor_.GetPointer()) {
        axis = AxisDirection::X;
        hit = true;
    } else if (picked == refCsysAxisYActor_.GetPointer()) {
        axis = AxisDirection::Y;
        hit = true;
    } else if (picked == refCsysAxisZActor_.GetPointer()) {
        axis = AxisDirection::Z;
        hit = true;
    }

    if (!hit) {
        hit = pickReferenceCsysAxisAt(x, y, axis);
    }
    if (!hit) {
        return false;
    }

    applyReferenceAxisHighlight(axis);
    resetReferencePlaneHighlight();
    applyVectorDirFromDatumAxis(axisDirectionToGpDir(axis));

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    return true;
}

bool Widget::handleReferenceCsysPlanePick(int x, int y)
{
    if (!hasReferenceCsys_ || !renderer) return false;
    // 几何拾取进行中不抢平面，避免干扰选面/选边
    if (currentSelectionMode == FaceSelection
        || currentSelectionMode == EdgeSelection
        || currentSelectionMode == ExtrusionSelection
        || currentSelectionMode == FilletEdgeSelection
        || currentSelectionMode == FilletRadiusHandleDrag
        || currentSelectionMode == ChamferEdgeSelection
        || currentSelectionMode == ChamferAsymHandleDrag
        || currentSelectionMode == SketchPlaneSelection
        || currentSelectionMode == VectorDialogPickStartPoint
        || currentSelectionMode == VectorDialogPickEndPoint
        || currentSelectionMode == PointSelection) {
        return false;
    }
    if (!ensureReferenceCsysActorsCreated()) return false;
    if (refCsysPlaneXYActor_ && refCsysPlaneXYActor_->GetVisibility() == 0) return false;

    // 矢量上下文下若点中轴，交给轴处理（调用方应先试轴）
    int planeId = -1;
    if (!pickReferenceCsysPlaneAt(x, y, planeId)) {
        return false;
    }

    applyReferencePlaneHighlight(planeId);
    const QString viewName = viewNameForReferenceCsysPlane(planeId);
    switchToView(viewName);

    if (statusBar()) {
        statusBar()->showMessage(tr("基准坐标系：切换到%1").arg(viewName), 2000);
    }
    return true;
}

void Widget::clearReferenceCsysState()
{
    if (renderer) {
        auto remove = [this](vtkActor* a) {
            if (a) removeSceneActor(a);
        };
        remove(refCsysAxisXActor_);
        remove(refCsysAxisYActor_);
        remove(refCsysAxisZActor_);
        remove(refCsysLabelXActor_);
        remove(refCsysLabelYActor_);
        remove(refCsysLabelZActor_);
        remove(refCsysPlaneXYActor_);
        remove(refCsysPlaneYZActor_);
        remove(refCsysPlaneXZActor_);
        remove(refCsysPlaneFrameActor_);
    }

    refCsysActor_ = nullptr;
    refCsysTransform_ = nullptr;
    refCsysAxisXActor_ = nullptr;
    refCsysAxisYActor_ = nullptr;
    refCsysAxisZActor_ = nullptr;
    refCsysLabelXActor_ = nullptr;
    refCsysLabelYActor_ = nullptr;
    refCsysLabelZActor_ = nullptr;
    refCsysPlaneXYActor_ = nullptr;
    refCsysPlaneYZActor_ = nullptr;
    refCsysPlaneXZActor_ = nullptr;
    refCsysPlaneFrameActor_ = nullptr;
    hasReferenceCsys_ = false;
    referenceCsysHistoryIndex_ = -1;
}

void Widget::initDefaultReferenceCsys()
{
    if (hasReferenceCsys_ && referenceCsysHistoryIndex_ >= 0
        && referenceCsysHistoryIndex_ < historyList.size()) {
        return;
    }

    if (!ensureReferenceCsysActorsCreated()) {
        return;
    }

    refCsysTransform_->Identity();
    hasReferenceCsys_ = true;
    setReferenceCsysVisible(true);
    refreshReferenceCsysScreenScale();

    const QColor color(80, 120, 200);
    addToHistory(REFERENCE_CSYS, tr("基准坐标系(1)"), refCsysActor_, color, 0.0, 0.0, 0.0,
                 nullptr, TopoDS_Shape(), nullptr, nullptr);
    referenceCsysHistoryIndex_ = historyList.size() - 1;

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    applyInitialSceneView();
}

void Widget::ensureDefaultReferenceCsysIfMissing()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].type != REFERENCE_CSYS) {
            continue;
        }
        hasReferenceCsys_ = true;
        referenceCsysHistoryIndex_ = i;
        if (!ensureReferenceCsysActorsCreated()) {
            return;
        }
        refCsysTransform_->Identity();
        const bool vis = refCsysAxisXActor_ ? (refCsysAxisXActor_->GetVisibility() != 0) : true;
        setReferenceCsysVisible(vis);
        refreshReferenceCsysScreenScale();
        return;
    }
    initDefaultReferenceCsys();
}

void Widget::restoreReferenceCsys(int index, const QString& name, const QColor& color)
{
    Q_UNUSED(color);
    if (!renderer || !vtkWidget) {
        return;
    }
    if (!ensureReferenceCsysActorsCreated()) {
        return;
    }

    refCsysTransform_->Identity();
    hasReferenceCsys_ = true;
    setReferenceCsysVisible(true);
    refreshReferenceCsysScreenScale();
    applyReferenceAxisHighlight(AxisDirection::Z);

    ModelingHistory record;
    record.type = REFERENCE_CSYS;
    record.name = name;
    record.timestamp = QDateTime::currentDateTime();
    record.color = QColor(80, 120, 200);
    record.param1 = 0.0;
    record.param2 = 0.0;
    record.param3 = 0.0;

    if (index < 0) index = 0;
    if (index > historyList.size()) index = historyList.size();
    remapHistoryIndicesAfterInsertion(index);
    modelDocument_.insert(index, record);
    renderStateFor(historyList[index]).actor = refCsysActor_;
    referenceCsysHistoryIndex_ = index;

    updateFeatureTree();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}
