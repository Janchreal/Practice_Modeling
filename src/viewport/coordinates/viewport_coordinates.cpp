// View triad and work coordinate system viewport behavior.
#include "main_window.h"
#include "mirror_view_types.h"
#include "ui_main_window.h"

#include <cmath>

#include <QCheckBox>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkLineSource.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPropPicker.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

void Widget::syncCenterAxisCamera()
{
    vtkRenderer* triadRenderer = centerAxesRenderer;
    if (MirrorRenderContext* mirrorCtx = mirrorContextForVtkWidget(vtkWidget)) {
        triadRenderer = mirrorCtx->centerAxesRenderer;
    }
    if (!renderer || !triadRenderer) return;

    vtkCamera* mainCam = renderer->GetActiveCamera();
    vtkCamera* axisCam = triadRenderer->GetActiveCamera();
    if (!mainCam || !axisCam) return;

    // 视线方向（由主相机决定）
    double pos[3], fp[3];
    mainCam->GetPosition(pos);
    mainCam->GetFocalPoint(fp);

    double viewDir[3] = { fp[0] - pos[0], fp[1] - pos[1], fp[2] - pos[2] };
    double len = std::sqrt(viewDir[0] * viewDir[0] + viewDir[1] * viewDir[1] + viewDir[2] * viewDir[2]);
    if (len < 1e-6) {
        viewDir[0] = 0.0; viewDir[1] = 0.0; viewDir[2] = -1.0;
        len = 1.0;
    }
    viewDir[0] /= len;
    viewDir[1] /= len;
    viewDir[2] /= len;

    // 让中心轴相机始终在固定距离上看向原点 (0,0,0)，方向与主相机一致
    double focal[3] = { 0.0, 0.0, 0.0 };
    double camPos[3] = {
        focal[0] - viewDir[0] * centerAxesCameraDistance,
        focal[1] - viewDir[1] * centerAxesCameraDistance,
        focal[2] - viewDir[2] * centerAxesCameraDistance
    };

    axisCam->SetPosition(camPos);
    axisCam->SetFocalPoint(focal);
    axisCam->SetViewUp(mainCam->GetViewUp());
    axisCam->SetParallelProjection(1);
    axisCam->SetParallelScale(1.5); // 稍大视锥，避免箭头被裁剪
    axisCam->Modified();
}

void Widget::setupCenterAxisSelector()
{
    // 渲染窗口四层：0=主场景，1=覆盖外观，2=参考操作柄，3=左下角三重轴
    renderPipeline_.initialize(vtkWidget->renderWindow(), renderer,
                               [](vtkRenderer* ren) { Widget::configureSceneLights(ren); });
    renderer->SetLayer(0);

    centerAxesRenderer = vtkSmartPointer<vtkRenderer>::New();
    centerAxesRenderer->SetLayer(3);
    // 将视图三重轴放在左下角，与原来的三轴视图位置一致
    centerAxesRenderer->SetViewport(0.0, 0.0, 0.2, 0.2); // 左下角，占窗口 20% 尺寸
    centerAxesRenderer->SetErase(0); // 不清除背景，叠加绘制
    configureSceneLights(centerAxesRenderer);
    vtkWidget->renderWindow()->AddRenderer(centerAxesRenderer);

    // 叠加层相机固定看向原点
    centerAxesRenderer->GetActiveCamera()->ParallelProjectionOn();
    centerAxesRenderer->GetActiveCamera()->SetPosition(0, 0, 5);
    centerAxesRenderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    centerAxesRenderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    centerAxesRenderer->GetActiveCamera()->SetParallelScale(1.2);

    auto makeAxisArrowActor = [&](AxisDirection axis, double r, double g, double b) -> vtkSmartPointer<vtkActor> {
        vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
        arrow->SetTipLength(0.35);
        arrow->SetTipRadius(0.10);
        arrow->SetShaftRadius(0.03);

        vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
        // vtkArrowSource 默认沿 +X 方向
        if (axis == AxisDirection::Y) {
            t->RotateZ(90.0);
        } else if (axis == AxisDirection::Z) {
            t->RotateY(-90.0);
        }
        t->Scale(centerAxesBaseScale, centerAxesBaseScale, centerAxesBaseScale);

        vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetTransform(t);
        tf->SetInputConnection(arrow->GetOutputPort());
        tf->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetOpacity(0.7);
        actor->GetProperty()->SetAmbient(0.7);
        actor->GetProperty()->SetDiffuse(0.3);
        actor->SetPickable(1);

        return actor;
    };

    centerAxisXActor = makeAxisArrowActor(AxisDirection::X, 1.0, 0.2, 0.2);
    centerAxisYActor = makeAxisArrowActor(AxisDirection::Y, 0.2, 0.9, 0.2);
    centerAxisZActor = makeAxisArrowActor(AxisDirection::Z, 0.2, 0.4, 1.0);

    // 初始整体缩放
    centerAxesCurrentScale = 1.0;
    centerAxisXActor->SetScale(centerAxesCurrentScale);
    centerAxisYActor->SetScale(centerAxesCurrentScale);
    centerAxisZActor->SetScale(centerAxesCurrentScale);

    // 未选中状态：颜色为柔和的基础色，透明度较低
    centerAxisXActor->GetProperty()->SetColor(1.0, 0.2, 0.2);
    centerAxisYActor->GetProperty()->SetColor(0.2, 0.9, 0.2);
    centerAxisZActor->GetProperty()->SetColor(0.2, 0.4, 1.0);
    centerAxisXActor->GetProperty()->SetOpacity(0.4);
    centerAxisYActor->GetProperty()->SetOpacity(0.4);
    centerAxisZActor->GetProperty()->SetOpacity(0.4);

    centerAxesRenderer->AddActor(centerAxisXActor);
    centerAxesRenderer->AddActor(centerAxisYActor);
    centerAxesRenderer->AddActor(centerAxisZActor);

    auto addAxisLabel = [&](const char* text, double x, double y, double z, double r, double g, double b) {
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(text);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(textSource->GetOutputPort());

        vtkSmartPointer<vtkActor> labelActor = vtkSmartPointer<vtkActor>::New();
        labelActor->SetMapper(mapper);
        labelActor->SetScale(0.25);
        labelActor->SetPosition(x, y, z);
        labelActor->GetProperty()->SetColor(r, g, b);
        labelActor->GetProperty()->SetAmbient(1.0);
        labelActor->GetProperty()->SetDiffuse(0.0);
        labelActor->SetPickable(0);
        centerAxesRenderer->AddActor(labelActor);
    };

    // 轴标签放在箭头尖端附近
    addAxisLabel("X", 1.05, 0.0, 0.0, 1.0, 0.2, 0.2);
    addAxisLabel("Y", 0.0, 1.05, 0.0, 0.2, 0.9, 0.2);
    addAxisLabel("Z", 0.0, 0.0, 1.05, 0.2, 0.4, 1.0);

    // 初始方向逻辑上仍默认为 Z，但视觉上不高亮任何轴
    currentAxisDirection = AxisDirection::Z;

    // 新增：可预选/可点击的三重轴立方体（面与边）
    setupCenterTriadCube();
}

void Widget::setupCenterTriadCube()
{
    if (!centerAxesRenderer) return;

    const double s = 0.42;
    struct FaceDef {
        int id;
        double ox, oy, oz;
        double nx, ny, nz;
    };
    const FaceDef faces[6] = {
        {0,  0,  0,  s,  0,  0,  1}, // front  +Z
        {1,  0,  0, -s,  0,  0, -1}, // back   -Z
        {2, -s,  0,  0, -1,  0,  0}, // left   -X
        {3,  s,  0,  0,  1,  0,  0}, // right  +X
        {4,  0,  s,  0,  0,  1,  0}, // top    +Y
        {5,  0, -s,  0,  0, -1,  0}, // bottom -Y
    };

    for (const auto& f : faces) {
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        if (std::abs(f.nz) > 0.5) {
            plane->SetOrigin(-s, -s, f.oz);
            plane->SetPoint1( s, -s, f.oz);
            plane->SetPoint2(-s,  s, f.oz);
        } else if (std::abs(f.nx) > 0.5) {
            plane->SetOrigin(f.ox, -s, -s);
            plane->SetPoint1(f.ox,  s, -s);
            plane->SetPoint2(f.ox, -s,  s);
        } else {
            plane->SetOrigin(-s, f.oy, -s);
            plane->SetPoint1( s, f.oy, -s);
            plane->SetPoint2(-s, f.oy,  s);
        }
        plane->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0.85, 0.85, 0.85);
        actor->GetProperty()->SetOpacity(0.18);
        actor->GetProperty()->SetAmbient(0.9);
        actor->GetProperty()->SetDiffuse(0.1);
        actor->SetPickable(1);
        centerTriadFaceActors_[f.id] = actor;
        centerAxesRenderer->AddActor(actor);
    }

    // 12 条边（用于“垂直于某面时，点击边跳到相邻面”）
    const double v[8][3] = {
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s},
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}
    };
    const int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; ++i) {
        vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(v[e[i][0]]);
        line->SetPoint2(v[e[i][1]]);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(line->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0.35, 0.35, 0.35);
        actor->GetProperty()->SetLineWidth(2.2);
        actor->SetPickable(1);
        centerTriadEdgeActors_[i] = actor;
        centerAxesRenderer->AddActor(actor);
    }
    refreshCenterTriadFaceStyle();
}

void Widget::refreshCenterTriadFaceStyle()
{
    vtkSmartPointer<vtkActor>* triadFaceActors = centerTriadFaceActors_;
    int currentFace = centerTriadCurrentFace_;
    int hoveredFace = centerTriadHoveredFace_;
    if (MirrorRenderContext* mirrorCtx = mirrorContextForVtkWidget(vtkWidget)) {
        triadFaceActors = mirrorCtx->centerTriadFaceActors;
        currentFace = mirrorCtx->centerTriadCurrentFace;
        hoveredFace = mirrorCtx->centerTriadHoveredFace;
    }
    for (int i = 0; i < 6; ++i) {
        if (!triadFaceActors[i]) continue;
        auto* p = triadFaceActors[i]->GetProperty();
        p->SetColor(0.85, 0.85, 0.85);
        p->SetOpacity(0.18);
        if (i == currentFace) {
            p->SetColor(1.0, 0.60, 0.0);
            p->SetOpacity(0.55);
        }
        if (i == hoveredFace) {
            p->SetColor(0.20, 0.75, 1.0);
            p->SetOpacity(0.60);
        }
    }
}

void Widget::updateCenterTriadHover(int x, int y)
{
    vtkRenderer* triadRenderer = centerAxesRenderer;
    vtkSmartPointer<vtkActor>* triadFaceActors = centerTriadFaceActors_;
    vtkSmartPointer<vtkActor>* triadEdgeActors = centerTriadEdgeActors_;
    int* hoveredFace = &centerTriadHoveredFace_;
    int* hoveredEdge = &centerTriadHoveredEdge_;

    if (MirrorRenderContext* mirrorCtx = mirrorContextForVtkWidget(vtkWidget)) {
        triadRenderer = mirrorCtx->centerAxesRenderer;
        triadFaceActors = mirrorCtx->centerTriadFaceActors;
        triadEdgeActors = mirrorCtx->centerTriadEdgeActors;
        hoveredFace = &mirrorCtx->centerTriadHoveredFace;
        hoveredEdge = &mirrorCtx->centerTriadHoveredEdge;
    } else if (ui && ui->checkBox && !ui->checkBox->isChecked()) {
        return;
    }
    if (!triadRenderer) return;
    if (!vtkWidget || !vtkWidget->renderWindow()) return;

    // 悬停同样限制在三重轴视口内，避免模型区域误亮三重轴
    int* size = vtkWidget->renderWindow()->GetSize();
    if (!size || size[0] <= 0 || size[1] <= 0) return;
    double vp[4] = {0.0, 0.0, 0.2, 0.2};
    triadRenderer->GetViewport(vp);
    const double nx = static_cast<double>(x) / static_cast<double>(size[0]);
    const double ny = static_cast<double>(y) / static_cast<double>(size[1]);
    if (nx < vp[0] || nx > vp[2] || ny < vp[1] || ny > vp[3]) {
        if (*hoveredFace >= 0 || *hoveredEdge >= 0) {
            *hoveredFace = -1;
            *hoveredEdge = -1;
            refreshCenterTriadFaceStyle();
            if (vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        }
        return;
    }

    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->Pick(x, y, 0, triadRenderer);
    vtkActor* picked = picker->GetActor();

    int newHoverFace = -1;
    int newHoverEdge = -1;
    for (int i = 0; i < 6; ++i) {
        if (triadFaceActors[i] && picked == triadFaceActors[i].GetPointer()) {
            newHoverFace = i;
            break;
        }
    }
    if (newHoverFace < 0) {
        for (int i = 0; i < 12; ++i) {
            if (triadEdgeActors[i] && picked == triadEdgeActors[i].GetPointer()) {
                newHoverEdge = i;
                break;
            }
        }
    }
    if (newHoverFace == *hoveredFace && newHoverEdge == *hoveredEdge) return;
    *hoveredFace = newHoverFace;
    *hoveredEdge = newHoverEdge;
    refreshCenterTriadFaceStyle();
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

int Widget::triadFaceFromViewName(const QString& viewName) const
{
    const QString v = viewName.trimmed();
    if (v == "正视图" || v == "前视图") return 0;
    if (v == "后视图") return 1;
    if (v == "左视图") return 2;
    if (v == "右视图") return 3;
    if (v == "俯视图") return 4;
    if (v == "仰视图") return 5;
    return -1;
}

QString Widget::viewNameFromTriadFace(int faceId) const
{
    switch (faceId) {
    case 0: return "前视图";
    case 1: return "后视图";
    case 2: return "左视图";
    case 3: return "右视图";
    case 4: return "俯视图";
    case 5: return "仰视图";
    default: return QString();
    }
}

void Widget::updateCenterTriadViewport(bool rightBottom)
{
    vtkRenderer* triadRenderer = centerAxesRenderer;
    if (MirrorRenderContext* mirrorCtx = mirrorContextForVtkWidget(vtkWidget)) {
        triadRenderer = mirrorCtx->centerAxesRenderer;
    }
    if (!triadRenderer) return;
    triadRenderer->SetViewport(rightBottom ? 0.8 : 0.0, 0.0, rightBottom ? 1.0 : 0.2, 0.2);
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

bool Widget::handleCenterAxisPick(int x, int y)
{
    vtkRenderer* triadRenderer = centerAxesRenderer;
    vtkSmartPointer<vtkActor>* triadFaceActors = centerTriadFaceActors_;
    vtkSmartPointer<vtkActor>* triadEdgeActors = centerTriadEdgeActors_;
    vtkSmartPointer<vtkActor>* axisXActor = &centerAxisXActor;
    vtkSmartPointer<vtkActor>* axisYActor = &centerAxisYActor;
    vtkSmartPointer<vtkActor>* axisZActor = &centerAxisZActor;
    int* currentFace = &centerTriadCurrentFace_;
    int* hoveredFace = &centerTriadHoveredFace_;

    if (MirrorRenderContext* mirrorCtx = mirrorContextForVtkWidget(vtkWidget)) {
        triadRenderer = mirrorCtx->centerAxesRenderer;
        triadFaceActors = mirrorCtx->centerTriadFaceActors;
        triadEdgeActors = mirrorCtx->centerTriadEdgeActors;
        axisXActor = &mirrorCtx->centerAxisXActor;
        axisYActor = &mirrorCtx->centerAxisYActor;
        axisZActor = &mirrorCtx->centerAxisZActor;
        currentFace = &mirrorCtx->centerTriadCurrentFace;
        hoveredFace = &mirrorCtx->centerTriadHoveredFace;
    }

    if (!triadRenderer || !vtkWidget || !vtkWidget->renderWindow()) return false;

    // 仅当点击落在三重轴视口内才处理，避免模型投影到左下角时被三重轴“吃掉”单击高亮
    int* size = vtkWidget->renderWindow()->GetSize();
    if (!size || size[0] <= 0 || size[1] <= 0) return false;
    double vp[4] = {0.0, 0.0, 0.2, 0.2};
    triadRenderer->GetViewport(vp);
    const double nx = static_cast<double>(x) / static_cast<double>(size[0]);
    const double ny = static_cast<double>(y) / static_cast<double>(size[1]);
    if (nx < vp[0] || nx > vp[2] || ny < vp[1] || ny > vp[3]) {
        return false;
    }

    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->Pick(x, y, 0, triadRenderer);
    vtkActor* picked = picker->GetActor();
    if (!picked) return false;

    // 已点中三重轴控件：直接处理，勿再用“稳健邻近拾取”误判为点中模型而吞掉
    // （邻近外壳会让左下角几乎永远 early-hit 模型，导致立方体无法切视角）

    // 1) 面点击：定向到垂直该面的视图
    for (int i = 0; i < 6; ++i) {
        if (triadFaceActors[i] && picked == triadFaceActors[i].GetPointer()) {
            const QString viewName = viewNameFromTriadFace(i);
            if (!viewName.isEmpty()) {
                *currentFace = i;
                *hoveredFace = -1;
                refreshCenterTriadFaceStyle();
                switchToView(viewName);
                return true;
            }
        }
    }

    // 2) 边点击：仅当当前视图已垂直于某面时，跳到该边相邻的另一面
    // face id: 0前(+Z) 1后(-Z) 2左(-X) 3右(+X) 4上(+Y) 5下(-Y)
    const int edgeFaces[12][2] = {
        {1,5}, {1,3}, {1,4}, {1,2},
        {0,5}, {0,3}, {0,4}, {0,2},
        {2,5}, {3,5}, {3,4}, {2,4}
    };
    if (*currentFace >= 0) {
        for (int e = 0; e < 12; ++e) {
            if (!triadEdgeActors[e] || picked != triadEdgeActors[e].GetPointer()) continue;
            int targetFace = -1;
            if (edgeFaces[e][0] == *currentFace) targetFace = edgeFaces[e][1];
            else if (edgeFaces[e][1] == *currentFace) targetFace = edgeFaces[e][0];
            if (targetFace >= 0) {
                const QString viewName = viewNameFromTriadFace(targetFace);
                if (!viewName.isEmpty()) {
                    *currentFace = targetFace;
                    *hoveredFace = -1;
                    refreshCenterTriadFaceStyle();
                    switchToView(viewName);
                    return true;
                }
            }
            return false;
        }
    }

    // 3) 保留原来的箭头点击选向量能力
    AxisDirection newDir = currentAxisDirection;
    if (picked == axisXActor->GetPointer()) newDir = AxisDirection::X;
    else if (picked == axisYActor->GetPointer()) newDir = AxisDirection::Y;
    else if (picked == axisZActor->GetPointer()) newDir = AxisDirection::Z;
    else return false;

    centerAxisXActor->GetProperty()->SetColor(1.0, 0.2, 0.2);
    centerAxisYActor->GetProperty()->SetColor(0.2, 0.9, 0.2);
    centerAxisZActor->GetProperty()->SetColor(0.2, 0.4, 1.0);
    centerAxisXActor->GetProperty()->SetOpacity(0.4);
    centerAxisYActor->GetProperty()->SetOpacity(0.4);
    centerAxisZActor->GetProperty()->SetOpacity(0.4);
    if (newDir == AxisDirection::X) { centerAxisXActor->GetProperty()->SetColor(1.0, 0.6, 0.0); centerAxisXActor->GetProperty()->SetOpacity(1.0); }
    else if (newDir == AxisDirection::Y) { centerAxisYActor->GetProperty()->SetColor(1.0, 0.6, 0.0); centerAxisYActor->GetProperty()->SetOpacity(1.0); }
    else { centerAxisZActor->GetProperty()->SetColor(1.0, 0.6, 0.0); centerAxisZActor->GetProperty()->SetOpacity(1.0); }
    currentAxisDirection = newDir;
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    return true;
}

// 设置基准坐标系
void Widget::setupCoordinateAxes()
{
    // 原始的左下角三轴视图（vtkOrientationMarkerWidget）不再显示，
    // 视图三重轴由自定义的 centerAxesRenderer 实现。
    Q_UNUSED(axesWidget);
}

bool Widget::ensureWorkCsysActorsCreated()
{
    if (!renderer || !vtkWidget) return false;
    if (workCsysAxisXActor_ && workCsysAxisYActor_ && workCsysAxisZActor_
        && workCsysLabelXActor_ && workCsysLabelYActor_ && workCsysLabelZActor_) {
        return true;
    }

    if (!workCsysTransform) {
        workCsysTransform = vtkSmartPointer<vtkTransform>::New();
        workCsysTransform->Identity();
    }

    // 工作坐标系整体缩放（更小，避免遮挡模型视觉）
    const double wcScale = 0.55;

    auto makeAxisArrowActor = [&](AxisDirection axis, double r, double g, double b) -> vtkSmartPointer<vtkActor> {
        vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
        arrow->SetTipLength(0.35);
        arrow->SetTipRadius(0.10);
        arrow->SetShaftRadius(0.03);

        vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
        // vtkArrowSource 默认沿 +X 方向
        if (axis == AxisDirection::Y) {
            t->RotateZ(90.0);
        } else if (axis == AxisDirection::Z) {
            t->RotateY(-90.0);
        }
        // 工作坐标系大小：缩小一档，并保持箭头比例
        t->Scale(wcScale, wcScale, wcScale);

        vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetTransform(t);
        tf->SetInputConnection(arrow->GetOutputPort());
        tf->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetUserTransform(workCsysTransform);
        actor->GetProperty()->SetColor(r, g, b);
        // 作为 overlay 显示：适当透明，避免“挡住模型像缺块”
        actor->GetProperty()->SetOpacity(0.60);
        actor->GetProperty()->SetAmbient(0.7);
        actor->GetProperty()->SetDiffuse(0.3);
        actor->GetProperty()->SetLineWidth(2.0);
        actor->SetPickable(1); // 允许点击选矢量
        return actor;
    };

    auto makeAxisLabelActor = [&](const char* text, double x, double y, double z, double r, double g, double b) -> vtkSmartPointer<vtkActor> {
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(text);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(textSource->GetOutputPort());

        vtkSmartPointer<vtkActor> labelActor = vtkSmartPointer<vtkActor>::New();
        labelActor->SetMapper(mapper);
        labelActor->SetScale(0.20 * wcScale);
        labelActor->SetPosition(x * wcScale, y * wcScale, z * wcScale);
        labelActor->SetUserTransform(workCsysTransform);
        labelActor->GetProperty()->SetColor(r, g, b);
        labelActor->GetProperty()->SetAmbient(1.0);
        labelActor->GetProperty()->SetDiffuse(0.0);
        labelActor->SetPickable(0);
        return labelActor;
    };

    workCsysAxisXActor_ = makeAxisArrowActor(AxisDirection::X, 1.0, 0.2, 0.2);
    workCsysAxisYActor_ = makeAxisArrowActor(AxisDirection::Y, 0.2, 0.9, 0.2);
    workCsysAxisZActor_ = makeAxisArrowActor(AxisDirection::Z, 0.2, 0.4, 1.0);

    // 标签使用 XC/YC/ZC
    workCsysLabelXActor_ = makeAxisLabelActor("XC", 1.05, 0.0, 0.0, 1.0, 0.2, 0.2);
    workCsysLabelYActor_ = makeAxisLabelActor("YC", 0.0, 1.05, 0.0, 0.2, 0.9, 0.2);
    workCsysLabelZActor_ = makeAxisLabelActor("ZC", 0.0, 0.0, 1.05, 0.2, 0.4, 1.0);

    addReferenceActor(workCsysAxisXActor_);
    addReferenceActor(workCsysAxisYActor_);
    addReferenceActor(workCsysAxisZActor_);
    addReferenceActor(workCsysLabelXActor_);
    addReferenceActor(workCsysLabelYActor_);
    addReferenceActor(workCsysLabelZActor_);

    // 让 historyList 里保存的 actor 指向 X 轴（用于定位/删除的标识）
    workCsysActor = workCsysAxisXActor_;

    // 初始高亮与当前方向一致（与左下角一致：橙色高亮当前轴）
    applyAxisDirectionHighlight(currentAxisDirection);
    return true;
}

void Widget::setWorkCsysVisible(bool visible)
{
    const int v = visible ? 1 : 0;
    if (workCsysAxisXActor_) workCsysAxisXActor_->SetVisibility(v);
    if (workCsysAxisYActor_) workCsysAxisYActor_->SetVisibility(v);
    if (workCsysAxisZActor_) workCsysAxisZActor_->SetVisibility(v);
    if (workCsysLabelXActor_) workCsysLabelXActor_->SetVisibility(v);
    if (workCsysLabelYActor_) workCsysLabelYActor_->SetVisibility(v);
    if (workCsysLabelZActor_) workCsysLabelZActor_->SetVisibility(v);
}

void Widget::applyAxisDirectionHighlight(AxisDirection dir)
{
    // 基础色 + 半透明
    auto resetAxis = [&](vtkSmartPointer<vtkActor>& a, double r, double g, double b) {
        if (!a) return;
        a->GetProperty()->SetColor(r, g, b);
        a->GetProperty()->SetOpacity(0.45);
    };
    resetAxis(workCsysAxisXActor_, 1.0, 0.2, 0.2);
    resetAxis(workCsysAxisYActor_, 0.2, 0.9, 0.2);
    resetAxis(workCsysAxisZActor_, 0.2, 0.4, 1.0);

    vtkSmartPointer<vtkActor> target;
    if (dir == AxisDirection::X) target = workCsysAxisXActor_;
    else if (dir == AxisDirection::Y) target = workCsysAxisYActor_;
    else target = workCsysAxisZActor_;
    if (target) {
        target->GetProperty()->SetColor(1.0, 0.6, 0.0); // 橙色高亮
        target->GetProperty()->SetOpacity(1.0);
    }
}

bool Widget::handleWorkCsysAxisPick(int x, int y)
{
    if (!hasWorkCsys) return false;
    if (!renderer) return false;
    if (!ensureWorkCsysActorsCreated()) return false;

    // 用 CellPicker 提高细小箭头的命中率，并且只在三根轴上拾取
    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.02);
    picker->PickFromListOn();
    if (workCsysAxisXActor_) picker->AddPickList(workCsysAxisXActor_);
    if (workCsysAxisYActor_) picker->AddPickList(workCsysAxisYActor_);
    if (workCsysAxisZActor_) picker->AddPickList(workCsysAxisZActor_);

    // 坐标系：与工程里 shapePicker->Pick(x,y,0) 一致，这里不做 Y 翻转
    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* picked = picker->GetActor();
    if (!picked) return false;

    AxisDirection newDir = currentAxisDirection;
    if (picked == workCsysAxisXActor_.GetPointer()) {
        newDir = AxisDirection::X;
    } else if (picked == workCsysAxisYActor_.GetPointer()) {
        newDir = AxisDirection::Y;
    } else if (picked == workCsysAxisZActor_.GetPointer()) {
        newDir = AxisDirection::Z;
    } else {
        return false;
    }

    if (isVectorAxisPickContext()) {
        gp_Dir pickDir(0, 0, 1);
        if (newDir == AxisDirection::X) pickDir = gp_Dir(1, 0, 0);
        else if (newDir == AxisDirection::Y) pickDir = gp_Dir(0, 1, 0);
        applyVectorDirFromDatumAxis(pickDir);
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        return true;
    }

    currentAxisDirection = newDir;
    applyAxisDirectionHighlight(newDir);
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
    if (statusBar()) {
        statusBar()->showMessage(tr("已选择矢量方向：%1").arg(newDir == AxisDirection::X ? "X" : (newDir == AxisDirection::Y ? "Y" : "Z")), 1500);
    }
    return true;
}
