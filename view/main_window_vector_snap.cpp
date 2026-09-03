// 矢量对话框 / 方向拾取 + 捕捉点 UI 与拾取（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "cone_params_dialog.h"
#include "cuboid_params_dialog.h"
#include "cylinder_dialog.h"
#include "extrusion_dialog.h"
#include "handle_geometry.h"
#include "sketch_geometry.h"
#include "sphere_params_dialog.h"
#include "vector_dialog.h"
#include "pattern_feature_dialog.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QColor>
#include <QDialog>
#include <QHash>
#include <QInputDialog>
#include <QObject>
#include <QGroupBox>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>
#include <QStatusBar>
#include <QTimer>
#include <QtGlobal>

#include <Standard_Failure.hxx>
#include <Standard_Real.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <Geom_Surface.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkVTK_ShapeData.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_ShapePicker.hxx>
#include <IVtk_Types.hxx>

#include <vtkSmartPointer.h>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkFeatureEdges.h>
#include <vtkFollower.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkVectorText.h>

static bool buildPickRayFromDisplay(vtkRenderer* renderer, int x, int y, gp_Lin& outRay);

// 根据对话框与当前方向状态得到拉伸方向（自动判断 / 自定义矢量 / 三重轴）
gp_Dir Widget::getExtrusionDirection(ExtrusionDialog* dialog) const
{
    if (hasCustomVectorDir_) {
        gp_Dir d = customVectorDir_;
        if (dialog && dialog->isAxisReversed()) {
            d.Reverse();
        }
        return d;
    }

    if (dialog && dialog->isAxisVectorSelected()) {
        gp_Dir d;
        switch (currentAxisDirection) {
        case AxisDirection::X: d = gp_Dir(1, 0, 0); break;
        case AxisDirection::Y: d = gp_Dir(0, 1, 0); break;
        case AxisDirection::Z: d = gp_Dir(0, 0, 1); break;
        }
        if (dialog->isAxisReversed()) {
            d.Reverse();
        }
        return d;
    }

    // 自动模式兜底：尝试从已选截面推断
    gp_Dir inferred(0, 0, 1);
    if (inferDirectionFromExtrusionSelection(inferred, nullptr)) {
        if (dialog && dialog->isAxisReversed()) inferred.Reverse();
        return inferred;
    }
    return gp_Dir(0, 0, 1);
}

// 根据对话框、当前三重轴方向和已选中心点得到旋转轴
gp_Ax1 Widget::getRevolutionAxis(revolvedialog* dialog) const
{
    gp_Pnt origin(0, 0, 0);
    if (hasSelectedOriginPoint) {
        origin = selectedOriginPoint;
    }

    if (hasCustomVectorDir_) {
        gp_Dir d = customVectorDir_;
        if (dialog && dialog->isAxisReversed()) {
            d.Reverse();
        }
        return gp_Ax1(origin, d);
    }

    if (dialog && dialog->isAxisVectorSelected()) {
        gp_Dir dir(0, 0, 1);
        switch (currentAxisDirection) {
        case AxisDirection::X: dir = gp_Dir(1, 0, 0); break;
        case AxisDirection::Y: dir = gp_Dir(0, 1, 0); break;
        case AxisDirection::Z: dir = gp_Dir(0, 0, 1); break;
        }
        if (dialog->isAxisReversed()) {
            dir.Reverse();
        }
        return gp_Ax1(origin, dir);
    }

    gp_Dir inferred(0, 0, 1);
    inferDirectionFromExtrusionSelection(inferred, nullptr);
    if (dialog && dialog->isAxisReversed()) inferred.Reverse();
    return gp_Ax1(origin, inferred);
}

gp_Pnt Widget::revolveVectorArrowOrigin() const
{
    if (!revolveDialog) {
        return gp_Pnt(0, 0, 0);
    }

    const gp_Ax1 axis = getRevolutionAxis(revolveDialog);
    const gp_Pnt O = axis.Location();
    const gp_Dir az = axis.Direction();

    // 箭头从旋转中心球心出发
    gp_Pnt center = O;
    gp_Pnt profileCenter;
    if (computeExtrusionProfileCenter(profileCenter)) {
        const gp_Vec oc(O, profileCenter);
        const double axial = oc.Dot(gp_Vec(az));
        center = O.Translated(gp_Vec(az) * axial);
    }
    return center;
}

void Widget::setCustomVectorDirFromDialog(const gp_Dir& baseDir)
{
    vectorDialogBaseDir_ = baseDir;
    hasVectorDialogBaseDir_ = true;

    gp_Dir d = baseDir;
    if (vectorDialogReverse_) {
        d.Reverse();
    }

    customVectorDir_ = d;
    hasCustomVectorDir_ = true;

    if (cuboidInteractiveActive_) {
        updateCuboidInteractivePreview();
    }
    if (extrusionDialog) {
        if (!extrusionDialog->isAutoVectorMode()) {
            // 手动矢量模式：保持用户模式
        }
        updateExtrusionHandles();
        refreshExtrusionLivePreview();
    }
    if (revolveDialog) {
        updateRevolveHandles();
        refreshRevolveLivePreview();
    }
}

void Widget::ensureVectorDialogArrowActor()
{
    if (!renderer) return;

    // 与操作柄同一套 VTK 箭头：vtkArrowSource（含箭柄），参数与拉伸终止箭头一致
    auto arrowSource = HandleGeom::makeArrowSource(
        ControlShape::ArrowWithShaft, HandleGeom::defaultShaftArrowParams());

    const HandleStateStyle style = HandleGeom::styleForControl(
        QStringLiteral("vector_direction"),
        QStringLiteral("direction_arrow"),
        ControlState::Default);

    if (vectorDialogArrowActor_) {
        if (auto* mapper = vtkPolyDataMapper::SafeDownCast(vectorDialogArrowActor_->GetMapper())) {
            mapper->SetInputConnection(arrowSource->GetOutputPort());
            mapper->Modified();
        }
        HandleGeom::applyStateStyle(vectorDialogArrowActor_->GetProperty(), style);
        vectorDialogArrowActor_->SetPickable(false);
        return;
    }

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(arrowSource->GetOutputPort());

    vectorDialogArrowActor_ = vtkSmartPointer<vtkActor>::New();
    vectorDialogArrowActor_->SetMapper(mapper);
    HandleGeom::applyStateStyle(vectorDialogArrowActor_->GetProperty(), style);
    vectorDialogArrowActor_->SetPickable(false);
    vectorDialogArrowActor_->SetVisibility(false);

    vectorDialogArrowTransform_ = vtkSmartPointer<vtkTransform>::New();
    vectorDialogArrowActor_->SetUserTransform(vectorDialogArrowTransform_);

    addReferenceActor(vectorDialogArrowActor_);
}

void Widget::updateVectorDialogArrow(const gp_Dir& dir, const gp_Pnt& origin)
{
    if (!renderer || !vtkWidget) return;
    ensureVectorDialogArrowActor();
    if (!vectorDialogArrowActor_ || !vectorDialogArrowTransform_) return;

    hasVectorDialogArrowOrigin_ = true;
    vectorDialogArrowOrigin_ = origin;

    // 与操作柄相同：vtkArrowSource 默认 +X，经 Transform 定向；屏幕尺寸恒定
    constexpr double kVectorArrowScreenLen = 0.90;
    const double arrowWorldLen =
        kVectorArrowScreenLen * overlayWorldScaleAt(origin.X(), origin.Y(), origin.Z());
    HandleGeom::applyArrowOrientation(vectorDialogArrowTransform_, origin, dir, arrowWorldLen);

    // 已确认方向：用规格中 confirmed 导引色；拾取中仍用 direction_arrow 默认/悬浮色
    const bool picking =
        currentSelectionMode == VectorDialogPickDirection
        || currentSelectionMode == VectorDialogPickStartPoint
        || currentSelectionMode == VectorDialogPickEndPoint;
    HandleStateStyle style = HandleGeom::styleForControl(
        QStringLiteral("vector_direction"),
        QStringLiteral("direction_arrow"),
        picking ? ControlState::Hover : ControlState::Selected);
    if (!picking) {
        // 已确认：贴近规格导引色（蓝）
        style.color = QColor(64, 160, 242);
        style.opacity = 0.90;
    }
    HandleGeom::applyStateStyle(vectorDialogArrowActor_->GetProperty(), style);

    vectorDialogArrowActor_->SetVisibility(true);
    if (vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::updateVectorArrowPreviewWithAxisReversed(bool axisReversed)
{
    if (!hasCustomVectorDir_) return;
    if (!renderer) return;

    gp_Dir d = customVectorDir_;
    if (axisReversed) d.Reverse();

    // 如果还没有起点，就退化到原点（至少保证箭头方向能刷新）
    gp_Pnt origin(0, 0, 0);
    if (hasVectorDialogArrowOrigin_) origin = vectorDialogArrowOrigin_;

    updateVectorDialogArrow(d, origin);
}

void Widget::clearVectorDialogHoverShape()
{
    if (renderer && vectorDialogHoverShapeActor_) {
        removeSceneActor(vectorDialogHoverShapeActor_);
        vectorDialogHoverShapeActor_ = nullptr;
    }
    if (renderer && vectorDialogHoverOutlineActor_) {
        removeSceneActor(vectorDialogHoverOutlineActor_);
        vectorDialogHoverOutlineActor_ = nullptr;
    }
    if (vectorDialogHoverModelIndex_ >= 0 && vectorDialogHoverModelIndex_ < historyList.size()) {
        auto& rec = historyList[vectorDialogHoverModelIndex_];
        if (rec.highlightActor) {
            rec.highlightActor->SetVisibility(false);
        }
    }

    // 恢复“悬浮变暗”状态（保证透明高亮结束后还原原始显示）
    if (vectorDialogHoverDimModelIndex_ >= 0 && vectorDialogHoverDimModelIndex_ < historyList.size()) {
        auto& dimRec = historyList[vectorDialogHoverDimModelIndex_];
        if (dimRec.actor && dimRec.actor->GetVisibility() != 0) {
            dimRec.actor->GetProperty()->SetColor(
                vectorDialogHoverDimOriginalColor_.redF(),
                vectorDialogHoverDimOriginalColor_.greenF(),
                vectorDialogHoverDimOriginalColor_.blueF()
            );
            dimRec.actor->GetProperty()->SetOpacity(vectorDialogHoverDimOriginalOpacity_);
        }
        vectorDialogHoverDimModelIndex_ = -1;
        vectorDialogHoverDimOriginalOpacity_ = 1.0;
    }

    vectorDialogHoverModelIndex_ = -1;
    vectorDialogHoverSubShapeId_ = static_cast<IVtk_IdType>(-1);
}

void Widget::applyVectorDialogHoverSubShape(int modelIndex, IVtk_IdType subShapeId, bool isFace)
{
    const IVtk_IdType invalidSubShapeId = static_cast<IVtk_IdType>(-1);
    if (subShapeId == invalidSubShapeId) return;
    const long long sid = static_cast<long long>(subShapeId);
    if (sid <= 0 || sid > static_cast<long long>(std::numeric_limits<int>::max())) return;
    if (modelIndex < 0 || modelIndex >= historyList.size()) return;
    auto& rec = historyList[modelIndex];
    if (rec.shapeWrapper.IsNull()) return;

    // 悬浮目标未变化：避免高频重复 SetData 引发不稳定
    if (vectorDialogHoverModelIndex_ == modelIndex && vectorDialogHoverSubShapeId_ == subShapeId) {
        if (vectorDialogHoverShapeActor_ && renderer) {
            vectorDialogHoverShapeActor_->SetVisibility(true);
        }
        if (vectorDialogHoverOutlineActor_ && renderer) {
            vectorDialogHoverOutlineActor_->SetVisibility(true);
        }
        return;
    }

    clearVectorDialogHoverShape();

    // 稳定路径：直接从命中的 subShape 构建悬浮高亮几何
    TopoDS_Shape sh;
    try {
        sh = rec.shapeWrapper->GetSubShape(subShapeId);
    } catch (Standard_Failure&) {
        return;
    } catch (...) {
        return;
    }
    if (sh.IsNull()) return;
    if (isFace && sh.ShapeType() != TopAbs_FACE) return;
    if (!isFace && sh.ShapeType() != TopAbs_EDGE && sh.ShapeType() != TopAbs_WIRE) return;

    // 面高亮：与拉伸模块的面悬停/选中视觉策略一致（半透明面 + 边界轮廓，且轮廓有轻微 Z 偏移）
    if (isFace) {
        // 关键：把“遮挡源”（该面的所属模型）临时变暗，避免半透明高亮在遮挡视角下被不透明面完全挡住
        if (rec.actor && vectorDialogHoverDimModelIndex_ != modelIndex) {
            vectorDialogHoverDimModelIndex_ = modelIndex;

            double rgb[3] = {0, 0, 0};
            rec.actor->GetProperty()->GetColor(rgb);
            vectorDialogHoverDimOriginalColor_ = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
            vectorDialogHoverDimOriginalOpacity_ = rec.actor->GetProperty()->GetOpacity();

            rec.actor->GetProperty()->SetColor(
                vectorDialogHoverDimOriginalColor_.redF() * 0.3,
                vectorDialogHoverDimOriginalColor_.greenF() * 0.3,
                vectorDialogHoverDimOriginalColor_.blueF() * 0.3
            );
            rec.actor->GetProperty()->SetOpacity(0.25);
        }

        try {
            BRepMesh_IncrementalMesh mesh(sh, 0.05, Standard_False, 0.3, Standard_True);
            mesh.Perform();

            Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(sh);
            hlShape->SetId(999998); // 临时高亮ID

            Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
            IVtkOCC_ShapeMesher mesher;
            mesher.Build(hlShape, shapeData);
            vtkPolyData* pd = shapeData->getVtkPolyData();
            if (!pd || pd->GetNumberOfPoints() <= 0) return;

            pd->SetLines(nullptr);
            pd->SetVerts(nullptr);

            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(pd);
            mapper->ScalarVisibilityOff();
            mapper->SetResolveCoincidentTopologyToPolygonOffset();

            vectorDialogHoverShapeActor_ = vtkSmartPointer<vtkActor>::New();
            vectorDialogHoverShapeActor_->SetMapper(mapper);
            vectorDialogHoverShapeActor_->GetProperty()->SetColor(0.0, 1.0, 0.0); // 绿色（和拉伸悬停一致）
            vectorDialogHoverShapeActor_->GetProperty()->SetOpacity(0.6);
            vectorDialogHoverShapeActor_->GetProperty()->SetRepresentationToSurface();
            vectorDialogHoverShapeActor_->GetProperty()->EdgeVisibilityOff();
            vectorDialogHoverShapeActor_->GetProperty()->SetLighting(false);
            vectorDialogHoverShapeActor_->GetProperty()->SetInterpolationToFlat();
            vectorDialogHoverShapeActor_->GetProperty()->SetAmbient(1.0);
            vectorDialogHoverShapeActor_->GetProperty()->SetDiffuse(0.0);
            vectorDialogHoverShapeActor_->GetProperty()->SetSpecular(0.0);
            vectorDialogHoverShapeActor_->SetPickable(false);

            // 边界轮廓：保证在部分视角下不会被遮挡得看不见
            vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
            featureEdges->SetInputData(pd);
            featureEdges->BoundaryEdgesOn();
            featureEdges->FeatureEdgesOff();
            featureEdges->ManifoldEdgesOff();
            featureEdges->NonManifoldEdgesOff();
            featureEdges->Update();

            vtkSmartPointer<vtkPolyDataMapper> edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            edgeMapper->SetInputConnection(featureEdges->GetOutputPort());
            edgeMapper->ScalarVisibilityOff();
            edgeMapper->SetResolveCoincidentTopologyToPolygonOffset();

            vectorDialogHoverOutlineActor_ = vtkSmartPointer<vtkActor>::New();
            vectorDialogHoverOutlineActor_->SetMapper(edgeMapper);
            vectorDialogHoverOutlineActor_->GetProperty()->SetColor(0.0, 1.0, 0.0);
            vectorDialogHoverOutlineActor_->GetProperty()->SetLineWidth(2.0);
            vectorDialogHoverOutlineActor_->GetProperty()->SetLighting(false);
            vectorDialogHoverOutlineActor_->SetPickable(false);
            vectorDialogHoverOutlineActor_->SetPosition(0, 0, 0.001);

            if (renderer) {
                addAppearanceActor(vectorDialogHoverShapeActor_);
                addAppearanceActor(vectorDialogHoverOutlineActor_);
            }
        } catch (...) {
            // 失败则回退到旧的单 actor 高亮
            vectorDialogHoverShapeActor_ = buildSnapShapeHighlightActor(sh, 0.2, 1.0, 1.0, 0.72, 5.0);
            if (vectorDialogHoverShapeActor_ && renderer) {
                addAppearanceActor(vectorDialogHoverShapeActor_);
            }
            vectorDialogHoverOutlineActor_ = nullptr;
        }
    } else {
        vectorDialogHoverShapeActor_ = buildSnapShapeHighlightActor(sh, 0.2, 1.0, 1.0, 0.95, 5.0);
        if (vectorDialogHoverShapeActor_ && renderer) {
            addAppearanceActor(vectorDialogHoverShapeActor_);
        }
        vectorDialogHoverOutlineActor_ = nullptr;
    }

    vectorDialogHoverModelIndex_ = modelIndex;
    vectorDialogHoverSubShapeId_ = subShapeId;
}

void Widget::clearVectorDialogArrowPreview()
{
    if (!renderer) return;
    clearVectorDialogHoverShape();
    clearVectorTwoPointHandles();
    if (vectorDialogArrowActor_) {
        removeSceneActor(vectorDialogArrowActor_);
        vectorDialogArrowActor_ = nullptr;
        vectorDialogArrowTransform_ = nullptr;
    }
    hasVectorDialogArrowOrigin_ = false;
    currentSelectionMode = None;
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::reapplyTwoPointSnapKindFilters(int snapKind)
{
    snap_.nearest = false;
    snap_.endpoint = false;
    snap_.midpoint = false;
    snap_.arcMidpoint = false;
    snap_.intersection = false;
    snap_.center = false;
    snap_.quadrant = false;
    snap_.onCurve = false;
    snap_.onFace = false;

    switch (snapKind) {
    case -1:
        snap_.enabled = false;
        snap_.armed = false;
        return;
    case 1:
        snap_.endpoint = true;
        break;
    case 2:
        snap_.midpoint = true;
        break;
    case 5:
        snap_.quadrant = true;
        break;
    case 6:
        snap_.arcMidpoint = true;
        break;
    case 3:
        snap_.intersection = true;
        break;
    default:
        snap_.endpoint = true;
        break;
    }
    snap_.enabled = true;
    snap_.armed = true;
}

void Widget::applyTwoPointVectorSnapKind(int snapKind, bool clearExistingPoints)
{
    if (clearExistingPoints) {
        clearSnapPersistentPoints();
        clearSnapSelected();
    } else {
        clearSnapSelected();
    }
    clearSnapHover();

    // 重置所有 snap 开关
    snap_.nearest = false;
    snap_.endpoint = false;
    snap_.midpoint = false;
    snap_.arcMidpoint = false;
    snap_.intersection = false;
    snap_.center = false;
    snap_.quadrant = false;

    // 根据 snapKind 打开对应类型
    switch (snapKind) {
    case -1: // 任意点：同时启用多类型捕捉
        // 任意点模式：主界面不会调用 pickSnapAt，而是用 tryPickPointOnModelForVector
        // 因此不需要启用 snap hover/armed，避免出现“未捕捉到可用点”或多余悬停。
        snap_.enabled = false;
        snap_.armed = false;
        // 先把类型全关（避免鼠标移动触发悬停候选）
        snap_.endpoint = false;
        snap_.midpoint = false;
        snap_.arcMidpoint = false;
        snap_.quadrant = false;
        snap_.intersection = false;
        break;
    case 1: // 端点
        snap_.endpoint = true;
        break;
    case 2: // 中点
        snap_.midpoint = true;
        break;
    case 5: // 象限点
        snap_.quadrant = true;
        break;
    case 6: // 圆弧中点（仅对圆曲线）
        snap_.arcMidpoint = true;
        break;
    case 3: // 交点
        snap_.intersection = true;
        break;
    default:
        snap_.endpoint = true;
        break;
    }

    if (snapKind != -1) {
        snap_.enabled = true;
        snap_.armed = true;
    }

    updateSnapPickGhostPresentation();

    if (ui) {
        // 避免 setChecked 触发 UI 的 toggled 槽
        QSignalBlocker bUse(ui->Use_Capture);
        QSignalBlocker bClosed(ui->Capture_Closed);
        QSignalBlocker bEnd(ui->Capture_Endpoint);
        QSignalBlocker bMid(ui->Capture_Midpoint);
        QSignalBlocker bInt(ui->Capture_Insertsectionpoint);
        QSignalBlocker bArcCen(ui->Capture_Arccenterpoint);
        QSignalBlocker bQuad(ui->Capture_Quadrantpoint);

        if (snapKind == -1) {
            ui->Use_Capture->setChecked(false);
            ui->Capture_Closed->setChecked(false);
            ui->Capture_Insertsectionpoint->setChecked(false);
            ui->Capture_Endpoint->setChecked(false);
            ui->Capture_Midpoint->setChecked(false);
            ui->Capture_Arccenterpoint->setChecked(false);
            ui->Capture_Quadrantpoint->setChecked(false);
            clearTabPointSnapToolbarButtons();
            if (ui->pushButton_71) {
                QSignalBlocker b71(ui->pushButton_71);
                ui->pushButton_71->setChecked(false);
            }
            if (ui->pushButton_70) {
                QSignalBlocker b70(ui->pushButton_70);
                ui->pushButton_70->setChecked(false);
            }
        } else {
            ui->Use_Capture->setChecked(true);
            ui->Capture_Closed->setChecked(false);
            ui->Capture_Insertsectionpoint->setChecked(snap_.intersection);
            ui->Capture_Endpoint->setChecked(snap_.endpoint);
            ui->Capture_Midpoint->setChecked(snap_.midpoint || snap_.arcMidpoint);
            ui->Capture_Arccenterpoint->setChecked(false);
            ui->Capture_Quadrantpoint->setChecked(snap_.quadrant);
            if (ui->pushButton_71) {
                QSignalBlocker b71(ui->pushButton_71);
                ui->pushButton_71->setChecked(true);
            }
            if (ui->pushButton_70) {
                QSignalBlocker b70(ui->pushButton_70);
                ui->pushButton_70->setChecked(true);
            }
        }
    }
    syncTabPointSnapToolbarsFromCaptureRow();
    mergeSnapFiltersFromToolbarAndCaptureUi();

    if (snapKind == -1) {
        snap_.enabled = false;
        snap_.armed = false;
        snap_.nearest = false;
        snap_.endpoint = false;
        snap_.midpoint = false;
        snap_.arcMidpoint = false;
        snap_.intersection = false;
        snap_.center = false;
        snap_.quadrant = false;
        snap_.onCurve = false;
        snap_.onFace = false;
    } else {
        reapplyTwoPointSnapKindFilters(snapKind);
    }
}

bool Widget::tryComputeVectorDirUnderCursor(int x, int y, gp_Dir& outDir,
                                            int* outHoverModelIndex,
                                            IVtk_IdType* outHoverSubShapeId)
{
    if (!shapePicker || !renderer) return false;
    const IVtk_IdType invalidSubShapeId = static_cast<IVtk_IdType>(-1);
    if (outHoverModelIndex) *outHoverModelIndex = -1;
    if (outHoverSubShapeId) *outHoverSubShapeId = invalidSubShapeId;

    // IVtk ShapePicker 命中无 ShapeSource 的 Actor 会崩溃：拾取前关闭手柄/预览拾取
    setFeatureGizmoActorsPickable(false);
    if (vectorDialogHoverShapeActor_) vectorDialogHoverShapeActor_->SetPickable(0);
    if (vectorDialogHoverOutlineActor_) vectorDialogHoverOutlineActor_->SetPickable(0);

    // 以当前 vectordialog 模式决定“计算逻辑”
    const int modeIndex = vectorDialogModeIndex_;

    // 悬停点用于箭头起点
    gp_Pnt hoverPoint(0, 0, 0);
    gp_Dir computedDir(0, 0, 1);
    int hoverModelIndex = -1;
    IVtk_IdType hoverSubShapeId = invalidSubShapeId;

    auto pickFaceUnderCursor = [&](vtkActor*& outActor,
                                   Handle(IVtkOCC_Shape)& outWrapper,
                                   IVtk_ShapeIdList& outSubIds) -> bool {
        outActor = nullptr;
        outWrapper.Nullify();
        outSubIds.Clear();

        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        shapePicker->SetSelectionMode(SM_Face);
        shapePicker->Pick(x, y, 0);

        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return false;

        pickedActors->InitTraversal();
        while (vtkActor* a = pickedActors->GetNextActor()) {
            if (!a || a->GetVisibility() == 0 || a->GetPickable() == 0) continue;
            if (!IVtkTools_ShapeObject::GetShapeSource(a)) continue;
            outActor = a;
            break;
        }
        if (!outActor) return false;

        IVtkTools_ShapeDataSource* dataSource = IVtkTools_ShapeObject::GetShapeSource(outActor);
        if (!dataSource) return false;
        outWrapper = dataSource->GetShape();
        if (outWrapper.IsNull()) return false;

        outSubIds = shapePicker->GetPickedSubShapesIds(outWrapper->GetId());
        return !outSubIds.IsEmpty();
    };

    auto safeGetFace = [](const Handle(IVtkOCC_Shape)& shapeWrapper,
                          IVtk_IdType subShapeId,
                          TopoDS_Face& outFace) -> bool {
        if (shapeWrapper.IsNull() || subShapeId <= 0) return false;
        try {
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
            if (subShape.IsNull() || subShape.ShapeType() != TopAbs_FACE) return false;
            outFace = TopoDS::Face(subShape);
            return !outFace.IsNull();
        } catch (Standard_Failure&) {
            return false;
        } catch (...) {
            return false;
        }
    };

    auto orientNormalTowardCamera = [&](gp_Dir& normalDir, const gp_Pnt& p) {
        if (auto* cam = renderer->GetActiveCamera()) {
            double camPosArr[3];
            cam->GetPosition(camPosArr);
            gp_Pnt camPos(camPosArr[0], camPosArr[1], camPosArr[2]);
            gp_Vec toCam(p, camPos);
            if (normalDir.Dot(toCam) < 0) {
                normalDir.Reverse();
            }
        }
    };

    // 面法向：取面参数中点（BRepAdaptor 自动处理 Location）
    auto computeFaceNormalAtMidParam = [&]() -> bool {
        try {
            vtkActor* selectedActor = nullptr;
            Handle(IVtkOCC_Shape) shapeWrapper;
            IVtk_ShapeIdList subShapeIds;
            if (!pickFaceUnderCursor(selectedActor, shapeWrapper, subShapeIds)) return false;

            for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
                const IVtk_IdType subShapeId = sIt.Value();
                TopoDS_Face face;
                if (!safeGetFace(shapeWrapper, subShapeId, face)) continue;

                BRepAdaptor_Surface adaptor(face);
                const Standard_Real uMid = 0.5 * (adaptor.FirstUParameter() + adaptor.LastUParameter());
                const Standard_Real vMid = 0.5 * (adaptor.FirstVParameter() + adaptor.LastVParameter());

                gp_Pnt p;
                gp_Vec d1u, d1v;
                adaptor.D1(uMid, vMid, p, d1u, d1v);

                gp_Vec normalVec = d1u.Crossed(d1v);
                if (normalVec.Magnitude() <= Precision::Confusion()) continue;

                gp_Dir normalDir(normalVec);
                if (face.Orientation() == TopAbs_REVERSED) {
                    normalDir.Reverse();
                }
                orientNormalTowardCamera(normalDir, p);

                computedDir = normalDir;
                hoverPoint = p;
                hoverSubShapeId = subShapeId;
                hoverModelIndex = resolveHistoryIndexByActor(selectedActor);
                return true;
            }
        } catch (Standard_Failure&) {
            return false;
        } catch (...) {
            return false;
        }
        return false;
    };

    // 面上点的矢量：射线与面求交得 UV，再取该点法向（禁止嵌套 Pick，避免 IVtk 状态错乱崩溃）
    auto computeFaceNormalAtHoverPoint = [&]() -> bool {
        try {
            vtkActor* selectedActor = nullptr;
            Handle(IVtkOCC_Shape) shapeWrapper;
            IVtk_ShapeIdList subShapeIds;
            if (!pickFaceUnderCursor(selectedActor, shapeWrapper, subShapeIds)) return false;

            gp_Lin pickRay;
            if (!buildPickRayFromDisplay(renderer, x, y, pickRay)) return false;

            for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
                const IVtk_IdType subShapeId = sIt.Value();
                TopoDS_Face face;
                if (!safeGetFace(shapeWrapper, subShapeId, face)) continue;

                IntCurvesFace_ShapeIntersector intersector;
                intersector.Load(face, Precision::Confusion());
                intersector.Perform(pickRay, 0.0, 1.0e9);
                if (intersector.NbPnt() <= 0) continue;

                Standard_Real bestW = RealLast();
                Standard_Real bestU = 0.0, bestV = 0.0;
                gp_Pnt bestP;
                bool found = false;
                for (int i = 1; i <= intersector.NbPnt(); ++i) {
                    const Standard_Real w = intersector.WParameter(i);
                    if (w < 0.0 || w >= bestW) continue;
                    bestW = w;
                    bestU = intersector.UParameter(i);
                    bestV = intersector.VParameter(i);
                    bestP = intersector.Pnt(i);
                    found = true;
                }
                if (!found) continue;

                BRepAdaptor_Surface adaptor(face);
                gp_Pnt p;
                gp_Vec d1u, d1v;
                adaptor.D1(bestU, bestV, p, d1u, d1v);
                // 交点更贴近鼠标射线命中位置
                p = bestP;

                gp_Vec normalVec = d1u.Crossed(d1v);
                if (normalVec.Magnitude() <= Precision::Confusion()) continue;

                gp_Dir normalDir(normalVec);
                if (face.Orientation() == TopAbs_REVERSED) {
                    normalDir.Reverse();
                }
                orientNormalTowardCamera(normalDir, p);

                computedDir = normalDir;
                hoverPoint = p;
                hoverSubShapeId = subShapeId;
                hoverModelIndex = resolveHistoryIndexByActor(selectedActor);
                return true;
            }
        } catch (Standard_Failure&) {
            return false;
        } catch (...) {
            return false;
        }
        return false;
    };

    auto computeEdgeTangent = [&]() -> bool {
        try {
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        shapePicker->SetSelectionMode(SM_Edge);
        shapePicker->Pick(x, y, 0);

        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return false;

        pickedActors->InitTraversal();
        vtkActor* selectedActor = nullptr;
        while (vtkActor* a = pickedActors->GetNextActor()) {
            if (!a || a->GetVisibility() == 0 || a->GetPickable() == 0) continue;
            if (!IVtkTools_ShapeObject::GetShapeSource(a)) continue;
            selectedActor = a;
            break;
        }
        if (!selectedActor) return false;

        IVtkTools_ShapeDataSource* dataSource = IVtkTools_ShapeObject::GetShapeSource(selectedActor);
        if (!dataSource) return false;

        Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
        if (shapeWrapper.IsNull()) return false;

        IVtk_IdType shapeID = shapeWrapper->GetId();
        IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);

        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            IVtk_IdType subShapeId = sIt.Value();
            TopoDS_Edge edge;
            try {
                const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
                if (subShape.IsNull() || subShape.ShapeType() != TopAbs_EDGE) continue;
                edge = TopoDS::Edge(subShape);
            } catch (...) {
                continue;
            }
            if (edge.IsNull()) continue;

            Standard_Real firstParam, lastParam;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, firstParam, lastParam);
            if (curve.IsNull()) continue;

            // 优先尝试识别圆（用于“曲线/轴矢量”）
            if (modeIndex == 3) {
                Handle(Geom_Circle) circle = SketchGeometry::sketchCircleBasis(curve);
                if (!circle.IsNull()) {
                    // 圆所在平面的法向作为“轴矢量”
                    gp_Ax2 ax2 = circle->Position();
                    gp_Dir axisDir = ax2.Direction();
                    gp_Pnt axisPoint = ax2.Location();

                    // 朝向相机
                    if (auto* cam = renderer->GetActiveCamera()) {
                        double camPosArr[3];
                        cam->GetPosition(camPosArr);
                        gp_Pnt camPos(camPosArr[0], camPosArr[1], camPosArr[2]);
                        gp_Vec toCam(axisPoint, camPos);
                        if (axisDir.Dot(toCam) < 0) axisDir.Reverse();
                    }

                    computedDir = axisDir;
                    hoverPoint = axisPoint;
                    hoverSubShapeId = subShapeId;
                    hoverModelIndex = resolveHistoryIndexByActor(selectedActor);
                    return true;
                }
            }

            // 通用：在曲线参数中点处取切向方向
            // “曲线上矢量”（modeIndex==4）：按弧长/弧长百分比定位到曲线上某点取切向
            gp_Pnt p;
            gp_Vec d1;
            double totalLen = 0.0;
            if (modeIndex == 4) {
                if (!SketchGeometry::edgePointAndTangentAtPosition(
                        edge,
                        vectorDialogCurvePosValue_,
                        vectorDialogCurvePosMode_ == 0,
                        p,
                        d1,
                        &totalLen)) {
                    continue;
                }
                vectorDialogCurveEdge_ = edge;
                hasVectorDialogCurveEdge_ = true;
                vectorDialogCurveTotalLen_ = totalLen;
            } else {
                const Standard_Real evalParam = (firstParam + lastParam) / 2.0;
                curve->D1(evalParam, p, d1);
                if (d1.Magnitude() <= Precision::Confusion()) continue;
            }

            gp_Dir tangentDir(d1);

            if (auto* cam = renderer->GetActiveCamera()) {
                double camPosArr[3];
                cam->GetPosition(camPosArr);
                gp_Pnt camPos(camPosArr[0], camPosArr[1], camPosArr[2]);
                gp_Vec toCam(p, camPos);
                if (tangentDir.Dot(toCam) < 0) tangentDir.Reverse();
            }

            computedDir = tangentDir;
            hoverPoint = p;
            hoverSubShapeId = subShapeId;
            hoverModelIndex = resolveHistoryIndexByActor(selectedActor);
            return true;
        }
        } catch (Standard_Failure&) {
            return false;
        } catch (...) {
            return false;
        }
        return false;
    };

    // 面/边的策略
    bool ok = false;
    if (modeIndex == 5) {
        ok = computeFaceNormalAtMidParam();
    } else if (modeIndex == 6) {
        ok = computeFaceNormalAtHoverPoint();
    } else if (modeIndex == 0) {
        // 自动判断：先面法向，失败则边切向
        ok = computeFaceNormalAtMidParam();
        if (!ok) ok = computeEdgeTangent();
    } else if (modeIndex == 3 || modeIndex == 4) {
        ok = computeEdgeTangent();
    } else if (modeIndex == 2) {
        // 先按“自动判断”
        ok = computeFaceNormalAtMidParam();
        if (!ok) ok = computeEdgeTangent();
    } else {
        // 其它（轴/视图方向）不走拾取
        ok = false;
    }

    if (!ok) return false;

    hasVectorDialogArrowOrigin_ = true;
    vectorDialogArrowOrigin_ = hoverPoint;
    if (outHoverModelIndex) *outHoverModelIndex = hoverModelIndex;
    if (outHoverSubShapeId) *outHoverSubShapeId = hoverSubShapeId;

    outDir = computedDir;
    return true;
}

bool Widget::tryPickPointOnModelForVector(int x, int y, gp_Pnt& outPoint)
{
    if (!shapePicker || !renderer) return false;

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);

    // 统一：先拾取面（用于“任意点”）
    shapePicker->SetSelectionMode(SM_Face);
    shapePicker->Pick(x, y, 0);

    vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
    if (!pickedActors || pickedActors->GetNumberOfItems() == 0) {
        // 退化：顶点
        shapePicker->SetSelectionMode(SM_Vertex);
        shapePicker->Pick(x, y, 0);
        pickedActors = shapePicker->GetPickedActors(true);
        if (!pickedActors || pickedActors->GetNumberOfItems() == 0) {
            shapePicker->SetSelectionMode(SM_Edge);
            shapePicker->Pick(x, y, 0);
            pickedActors = shapePicker->GetPickedActors(true);
            if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return false;
        }
    }

    pickedActors->InitTraversal();
    vtkActor* selectedActor = nullptr;
    while (vtkActor* a = pickedActors->GetNextActor()) {
        if (!a || a->GetVisibility() == 0 || a->GetPickable() == 0) continue;
        if (!IVtkTools_ShapeObject::GetShapeSource(a)) continue;
        selectedActor = a;
        break;
    }
    if (!selectedActor) return false;

    IVtkTools_ShapeDataSource* dataSource = IVtkTools_ShapeObject::GetShapeSource(selectedActor);
    if (!dataSource) return false;

    Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
    if (shapeWrapper.IsNull()) return false;

    IVtk_IdType shapeID = shapeWrapper->GetId();
    IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subShapeIds.IsEmpty()) return false;

    // 构造鼠标点对应的世界射线
    gp_Pnt rayOrigin;
    gp_Dir rayDir(0, 0, 1);
    bool hasRay = false;
    {
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

        gp_Pnt p0(worldNear[0], worldNear[1], worldNear[2]);
        gp_Pnt p1(worldFar[0], worldFar[1], worldFar[2]);
        gp_Vec v(p0, p1);
        if (v.Magnitude() > Precision::Confusion()) {
            rayOrigin = p0;
            rayDir = gp_Dir(v);
            hasRay = true;
        }
    }

    // 1) 若拾取到面，射线-面求交，得到点击点
    if (hasRay) {
        Standard_Real globalBestW = RealLast();
        gp_Pnt globalBestP;
        bool foundPoint = false;

        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            const IVtk_IdType subShapeId = sIt.Value();
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
            if (subShape.ShapeType() != TopAbs_FACE) continue;

            TopoDS_Face face = TopoDS::Face(subShape);
            IntCurvesFace_ShapeIntersector intersector;
            intersector.Load(face, Precision::Confusion());
            intersector.Perform(gp_Lin(rayOrigin, rayDir), 0.0, 1.0e9);
            if (intersector.NbPnt() <= 0) continue;

            for (int i = 1; i <= intersector.NbPnt(); ++i) {
                Standard_Real w = intersector.WParameter(i);
                if (w >= 0.0 && w < globalBestW) {
                    globalBestW = w;
                    globalBestP = intersector.Pnt(i);
                    foundPoint = true;
                }
            }
        }

        if (foundPoint) {
            outPoint = globalBestP;
            return true;
        }
    }

    // 2) 顶点/边退化
    for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
        const IVtk_IdType subShapeId = sIt.Value();
        const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
        if (subShape.ShapeType() == TopAbs_VERTEX) {
            outPoint = BRep_Tool::Pnt(TopoDS::Vertex(subShape));
            return true;
        } else if (subShape.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge edge = TopoDS::Edge(subShape);
            if (SketchGeometry::edgeMidPoint(edge, outPoint)) {
                return true;
            }
        }
    }

    return false;
}

void Widget::openVectorDialog(int desiredModeIndex)
{
    if (!shapePicker || !renderer || !vtkWidget) return;

    const bool patternVectorSession =
        (patternDialog_ != nullptr && patternVectorPick_ != PatternVectorPick::None);

    // 先清理上一轮的状态（确保取消时不会残留）
    hasVectorDialogBaseDir_ = false;
    hasCustomVectorDir_ = false;
    vectorDialogReverse_ = false;
    vectorDialogModeIndex_ = 0;
    hasVectorDialogArrowOrigin_ = false;
    if (!patternVectorSession) {
        currentSelectionMode = None;
    }
    hasVectorDialogCurveEdge_ = false;
    vectorDialogCurveTotalLen_ = 0.0;
    vectorDialogCurvePosMode_ = 0;   // 默认：弧长百分比
    vectorDialogCurvePosValue_ = 0.0;

    ensureVectorDialogArrowActor();
    if (vectorDialogArrowActor_) {
        vectorDialogArrowActor_->SetVisibility(false);
        vtkWidget->renderWindow()->Render();
    }

    auto* dlg = new vectordialog(dialogParentWidget());
    vectorDialog_ = dlg;
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QObject::destroyed, this, [this]() {
        vectorDialog_ = nullptr;
    });

    auto applyAxisFixedMode = [&](int modeIndex) {
        // 这些模式不需要鼠标拾取
        gp_Dir base(0, 0, 1);
        bool ok = true;
        if (modeIndex == 7) base = gp_Dir(1, 0, 0);
        else if (modeIndex == 8) base = gp_Dir(0, 1, 0);
        else if (modeIndex == 9) base = gp_Dir(0, 0, 1);
        else if (modeIndex == 10) base = gp_Dir(-1, 0, 0);
        else if (modeIndex == 11) base = gp_Dir(0, -1, 0);
        else if (modeIndex == 12) base = gp_Dir(0, 0, -1);
        else if (modeIndex == 13) {
            // 视图方向：用相机方向作为基准
            if (auto* cam = renderer->GetActiveCamera()) {
                double dirArr[3];
                cam->GetDirectionOfProjection(dirArr);
                base = gp_Dir(dirArr[0], dirArr[1], dirArr[2]);
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        if (!ok) return;

        hasVectorDialogBaseDir_ = true;
        vectorDialogBaseDir_ = base;
        hasCustomVectorDir_ = true;
        gp_Dir d = base;
        if (vectorDialogReverse_) d.Reverse();
        customVectorDir_ = d;

        if (patternVectorSession) {
            applyPatternVectorPick(customVectorDir_);
            return;
        }

        gp_Pnt origin(0, 0, 0);
        hasVectorDialogArrowOrigin_ = true;
        vectorDialogArrowOrigin_ = origin;
        updateVectorDialogArrow(customVectorDir_, origin);
        currentSelectionMode = None;
    };

    connect(dlg, &vectordialog::vectorModeChanged, this, [this, dlg, applyAxisFixedMode](int modeIndex) mutable {
        vectorDialogModeIndex_ = modeIndex;
        clearVectorDialogHoverShape();

        // 轴/视图方向：立即确定
        if (modeIndex >= 7 && modeIndex <= 13) {
            applyAxisFixedMode(modeIndex);
            return;
        }

        // 自动判断：进入悬停拾取
        if (modeIndex == 0 || modeIndex == 2) {
            currentSelectionMode = VectorDialogPickDirection;
        } else if (modeIndex == 3 || modeIndex == 4 || modeIndex == 5 || modeIndex == 6) {
            // 曲线/面法向：等待“选择对象”按钮信号
            currentSelectionMode = None;
            // 模式切换到曲线相关时，清空曲线缓存（重新选择）
            hasVectorDialogCurveEdge_ = false;
            vectorDialogCurveTotalLen_ = 0.0;
            if (modeIndex == 4 && vectorDialog_) {
                vectorDialog_->setCurvePicked(false);
                vectorDialog_->setCurveTotalLength(0.0);
            }
        } else if (modeIndex == 1) {
            QTimer::singleShot(0, this, [this]() {
                if (!vectorDialog_ || vectorDialogModeIndex_ != 1) return;
                if (hasVectorEndPoint_) {
                    currentSelectionMode = VectorTwoPointInteractive;
                    updateVectorTwoPointHandles();
                } else {
                    beginVectorTwoPointPickStart();
                }
            });
        } else {
            currentSelectionMode = None;
        }
    });

    connect(dlg, &vectordialog::vectorReverseToggled, this, [this](bool reversed) {
        vectorDialogReverse_ = reversed;
        if (hasVectorDialogBaseDir_) {
            gp_Dir base = vectorDialogBaseDir_;
            setCustomVectorDirFromDialog(base);
            if (hasVectorDialogArrowOrigin_) {
                updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
            }
            if (vectorDialog_) {
                vectorDialog_->setVectorDirDisplay(customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
            }
        }
    });

    connect(dlg, &vectordialog::twoPointStartRequestedWithSnap, this, [this](int startSnapKind, int endSnapKind) {
        vectorTwoPointStartSnapKind_ = startSnapKind;
        vectorTwoPointEndSnapKind_ = endSnapKind;
        beginVectorTwoPointPickStart(false);
    });

    connect(dlg, &vectordialog::twoPointEndRequestedWithSnap, this, [this](int endSnapKind) {
        vectorTwoPointEndSnapKind_ = endSnapKind;
        if (!hasVectorStartPoint_) return;

        vectorTwoPointAwaitingEndPick_ = false;
        currentSelectionMode = VectorDialogPickEndPoint;
        applyTwoPointVectorSnapKind(endSnapKind, false);
    });

    connect(dlg, &vectordialog::curveSelectionRequested, this, [this]() {
        // 曲线相关：进入悬停拾取（点击确定）
        currentSelectionMode = VectorDialogPickDirection;
        if (vtkWidget) vtkWidget->setFocus();
    });

    auto recomputeCurveVectorIfPossible = [this]() {
        if (!hasVectorDialogCurveEdge_) return;
        gp_Pnt p;
        gp_Vec d1;
        if (!SketchGeometry::edgePointAndTangentAtPosition(
                vectorDialogCurveEdge_,
                vectorDialogCurvePosValue_,
                vectorDialogCurvePosMode_ == 0,
                p,
                d1,
                &vectorDialogCurveTotalLen_)) {
            return;
        }

        gp_Dir tangentDir(d1);
        if (auto* cam = renderer->GetActiveCamera()) {
            double camPosArr[3];
            cam->GetPosition(camPosArr);
            gp_Pnt camPos(camPosArr[0], camPosArr[1], camPosArr[2]);
            gp_Vec toCam(p, camPos);
            if (tangentDir.Dot(toCam) < 0) tangentDir.Reverse();
        }

        vectorDialogBaseDir_ = tangentDir;
        hasVectorDialogBaseDir_ = true;
        setCustomVectorDirFromDialog(tangentDir);

        hasVectorDialogArrowOrigin_ = true;
        vectorDialogArrowOrigin_ = p;
        updateVectorDialogArrow(customVectorDir_, p);

        if (vectorDialog_) {
            vectorDialog_->setCurvePicked(true);
            vectorDialog_->setCurveTotalLength(vectorDialogCurveTotalLen_);
            vectorDialog_->setVectorDirDisplay(customVectorDir_.X(), customVectorDir_.Y(), customVectorDir_.Z());
        }
    };

    connect(dlg, &vectordialog::curveVectorPositionModeChanged, this, [this, recomputeCurveVectorIfPossible](int mode) mutable {
        vectorDialogCurvePosMode_ = mode;
        if (vectorDialogModeIndex_ == 4) {
            recomputeCurveVectorIfPossible();
        }
    });
    connect(dlg, &vectordialog::curveVectorPositionValueChanged, this, [this, recomputeCurveVectorIfPossible](double v) mutable {
        vectorDialogCurvePosValue_ = v;
        if (vectorDialogModeIndex_ == 4) {
            recomputeCurveVectorIfPossible();
        }
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        // 由拾取流程已经写入 hasCustomVectorDir_ / customVectorDir_
        if (patternDialog_ && patternVectorPick_ != PatternVectorPick::None && hasCustomVectorDir_) {
            applyPatternVectorPick(customVectorDir_);
        }
        if (patternDialog_) {
            patternVectorPick_ = PatternVectorPick::None;
            if (patternDialog_->layoutType() == PatternLayoutType::Linear) {
                clearVectorDialogArrowPreview();
            }
            if (patternDialog_->hasDirection1()) {
                const int axis = (patternDialog_->useDirection2() && patternDialog_->hasDirection2()
                                      && patternActivePitchAxis_ == 1)
                                     ? 1
                                     : 0;
                startPatternPitchInteractive(axis);
            } else {
                currentSelectionMode = PatternBodySelection;
            }
        } else if (cuboidInteractiveActive_) {
            // 确认矢量后必须回到长方体 Gizmo 模式，否则手柄无法拖拽
            currentSelectionMode = CuboidInteractive;
            syncCuboidInteractiveFromDialog();
            updateCuboidInteractivePreview();
        } else {
            currentSelectionMode = None;
        }
        snap_.enabled = false;
        setSnapArmed(false);
        clearSnapPersistentPoints();
        clearSnapSelected();
        clearSnapHover();
        clearSelectedPoint();
        clearPointSelectionHover();
        if (ui) {
            ui->Use_Capture->setChecked(false);
            ui->Capture_Closed->setChecked(false);
            ui->Capture_Endpoint->setChecked(false);
            ui->Capture_Midpoint->setChecked(false);
            ui->Capture_Insertsectionpoint->setChecked(false);
            ui->Capture_Arccenterpoint->setChecked(false);
            ui->Capture_Quadrantpoint->setChecked(false);
            clearTabPointSnapToolbarButtons();
            if (ui->pushButton_71) {
                QSignalBlocker b71(ui->pushButton_71);
                ui->pushButton_71->setChecked(false);
            }
            if (ui->pushButton_70) {
                QSignalBlocker b70(ui->pushButton_70);
                ui->pushButton_70->setChecked(false);
            }
        }
        mergeSnapFiltersFromToolbarAndCaptureUi();
        resetReferenceAxisHighlight();
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        // 确认后刷新箭头为已确认样式，并再次清点悬停残留
        if (hasCustomVectorDir_ && hasVectorDialogArrowOrigin_) {
            updateVectorDialogArrow(customVectorDir_, vectorDialogArrowOrigin_);
        }
        clearPointSelectionHover();
        dlg->hide();
    });

    connect(dlg, &QDialog::rejected, this, [this, dlg]() {
        if (patternDialog_) {
            patternVectorPick_ = PatternVectorPick::None;
            clearVectorDialogArrowPreview();
            if (patternDialog_->hasDirection1()) {
                const int axis = (patternDialog_->useDirection2() && patternDialog_->hasDirection2()
                                      && patternActivePitchAxis_ == 1)
                                     ? 1
                                     : 0;
                startPatternPitchInteractive(axis);
            } else {
                currentSelectionMode = PatternBodySelection;
            }
        } else if (cuboidInteractiveActive_) {
            currentSelectionMode = CuboidInteractive;
            updateCuboidInteractivePreview();
        } else {
            currentSelectionMode = None;
        }
        hasCustomVectorDir_ = false;
        hasVectorStartPoint_ = false;
        hasVectorEndPoint_ = false;
        vectorDialogModeIndex_ = 0;
        clearVectorTwoPointHandles();
        if (vectorDialogArrowActor_) vectorDialogArrowActor_->SetVisibility(false);
        snap_.enabled = false;
        setSnapArmed(false);
        clearSnapPersistentPoints();
        clearSnapSelected();
        clearSnapHover();
        clearSelectedPoint();
        clearPointSelectionHover();
        if (ui) {
            ui->Use_Capture->setChecked(false);
            ui->Capture_Closed->setChecked(false);
            ui->Capture_Endpoint->setChecked(false);
            ui->Capture_Midpoint->setChecked(false);
            ui->Capture_Insertsectionpoint->setChecked(false);
            ui->Capture_Arccenterpoint->setChecked(false);
            ui->Capture_Quadrantpoint->setChecked(false);
            clearTabPointSnapToolbarButtons();
            if (ui->pushButton_71) {
                QSignalBlocker b71(ui->pushButton_71);
                ui->pushButton_71->setChecked(false);
            }
            if (ui->pushButton_70) {
                QSignalBlocker b70(ui->pushButton_70);
                ui->pushButton_70->setChecked(false);
            }
        }
        mergeSnapFiltersFromToolbarAndCaptureUi();
        resetReferenceAxisHighlight();
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
        dlg->hide();
    });

    if (desiredModeIndex >= 0) {
        dlg->setModeIndex(desiredModeIndex);
    }

    if (patternVectorSession) {
        currentSelectionMode = VectorDialogPickDirection;
    }

    dlg->show();
    vtkWidget->setFocus();
}


// 更新点选择的悬停提示
void Widget::updatePointSelectionHover(int x, int y)
{
    if (!shapePicker || !renderer) {
        return;
    }

    // 拾取前更新所有 ShapeDataSource
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    // 更新 shapePicker
    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    
    
    bool found = false;
    gp_Pnt hoverPoint;
    QString hoverLabel;
    vtkSmartPointer<vtkActorCollection> pickedActors;

    // 1) 顶点优先：靠近角点时不要被平面 Location(常为原点)抢占
    shapePicker->SetSelectionMode(SM_Vertex);
    shapePicker->Pick(x, y, 0);
    pickedActors = shapePicker->GetPickedActors();
    if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
        pickedActors->InitTraversal();
        vtkActor* selectedActor = pickedActors->GetNextActor();
        IVtkTools_ShapeDataSource* dataSource =
            IVtkTools_ShapeObject::GetShapeSource(selectedActor);
        if (dataSource) {
            Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
            if (!shapeWrapper.IsNull()) {
                IVtk_IdType shapeID = shapeWrapper->GetId();
                IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
                for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
                    const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(sIt.Value());
                    if (subShape.ShapeType() == TopAbs_VERTEX) {
                        hoverPoint = BRep_Tool::Pnt(TopoDS::Vertex(subShape));
                        hoverLabel = QString("顶点\n(%1, %2, %3)")
                                         .arg(hoverPoint.X(), 0, 'f', 2)
                                         .arg(hoverPoint.Y(), 0, 'f', 2)
                                         .arg(hoverPoint.Z(), 0, 'f', 2);
                        found = true;
                        break;
                    }
                }
            }
        }
    }

    // 2) 面上真实交点（任意点悬停）
    if (!found) {
        gp_Pnt pOnFace;
        if (tryPickPointOnModelForVector(x, y, pOnFace)) {
            hoverPoint = pOnFace;
            hoverLabel = QString("面上点\n(%1, %2, %3)")
                             .arg(hoverPoint.X(), 0, 'f', 2)
                             .arg(hoverPoint.Y(), 0, 'f', 2)
                             .arg(hoverPoint.Z(), 0, 'f', 2);
            found = true;
        }
    }

    // 3) 边中点兜底
    if (!found) {
        shapePicker->SetSelectionMode(SM_Edge);
        shapePicker->Pick(x, y, 0);
        pickedActors = shapePicker->GetPickedActors();
        if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
            pickedActors->InitTraversal();
            vtkActor* selectedActor = pickedActors->GetNextActor();
            IVtkTools_ShapeDataSource* dataSource =
                IVtkTools_ShapeObject::GetShapeSource(selectedActor);
            if (dataSource) {
                Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
                if (!shapeWrapper.IsNull()) {
                    IVtk_IdType shapeID = shapeWrapper->GetId();
                    IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
                    for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
                        const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(sIt.Value());
                        if (subShape.ShapeType() == TopAbs_EDGE) {
                            TopoDS_Edge edge = TopoDS::Edge(subShape);
                            Standard_Real firstParam = 0.0, lastParam = 0.0;
                            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, firstParam, lastParam);
                            if (!curve.IsNull()) {
                                hoverPoint = curve->Value(0.5 * (firstParam + lastParam));
                                hoverLabel = QString("中点\n(%1, %2, %3)")
                                                 .arg(hoverPoint.X(), 0, 'f', 2)
                                                 .arg(hoverPoint.Y(), 0, 'f', 2)
                                                 .arg(hoverPoint.Z(), 0, 'f', 2);
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

if (found) {
        // 清除旧的悬停提示
        clearPointSelectionHover();
        
        // 创建新的悬停点
        vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
        configureMarkerSphereSource(sphere, 0.05);
        sphere->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());

        hoverPointActor = vtkSmartPointer<vtkActor>::New();
        hoverPointActor->SetMapper(mapper);
        hoverPointActor->SetPosition(hoverPoint.X(), hoverPoint.Y(), hoverPoint.Z());
        applyMarkerSphereMaterial(hoverPointActor->GetProperty(), MarkerSphereStyle::HoverYellow);
        {
            const double s = overlayWorldScaleAt(hoverPoint.X(), hoverPoint.Y(), hoverPoint.Z());
            hoverPointActor->SetScale(s, s, s);
        }

        addReferenceActor(hoverPointActor);
        
        // 创建文字标签
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(hoverLabel.toStdString().c_str());
        
        vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        textMapper->SetInputConnection(textSource->GetOutputPort());
        
        hoverTextActor = vtkSmartPointer<vtkFollower>::New();
        hoverTextActor->SetMapper(textMapper);
        hoverTextActor->SetCamera(renderer->GetActiveCamera());
        {
            const double s = overlayWorldScaleAt(hoverPoint.X(), hoverPoint.Y(), hoverPoint.Z());
            hoverTextActor->SetPosition(hoverPoint.X() + 0.1 * s, hoverPoint.Y() + 0.1 * s, hoverPoint.Z() + 0.1 * s);
            hoverTextActor->SetScale(0.05 * s);
        }
        hoverTextActor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 黄色文字
        
        addReferenceActor(hoverTextActor);
        
        vtkWidget->renderWindow()->Render();
    } else {
        clearPointSelectionHover();
    }
}

// 清除点选择的悬停提示
void Widget::clearPointSelectionHover()
{
    if (hoverPointActor) {
        removeSceneActor(hoverPointActor);
        hoverPointActor = nullptr;
    }
    if (hoverTextActor) {
        removeSceneActor(hoverTextActor);
        hoverTextActor = nullptr;
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 显示选中的点（放大显示）
void Widget::showSelectedPoint(const gp_Pnt& point, const QString& label)
{
    if (!renderer) {
        return;
    }
    
    // 清除之前的选中点（但不重置指针，因为我们要重新赋值）
    if (selectedPointActor) {
        removeSceneActor(selectedPointActor);
        selectedPointActor = nullptr;
    }
    if (selectedTextActor) {
        removeSceneActor(selectedTextActor);
        selectedTextActor = nullptr;
    }
    
    // 创建放大的点（选中状态）：几何在局部原点，用 Position+Scale 保证缩放绕点中心
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    configureMarkerSphereSource(sphere, 0.08);
    sphere->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());

    selectedPointActor = vtkSmartPointer<vtkActor>::New();
    selectedPointActor->SetMapper(mapper);
    selectedPointActor->SetPosition(point.X(), point.Y(), point.Z());
    applyMarkerSphereMaterial(selectedPointActor->GetProperty(), MarkerSphereStyle::ConfirmedGreen);
    {
        const double s = overlayWorldScaleAt(point.X(), point.Y(), point.Z());
        selectedPointActor->SetScale(s, s, s);
    }

    addReferenceActor(selectedPointActor);

    // 可选：创建文字标签（捕捉点模式下允许只显示小球）
    if (!label.trimmed().isEmpty()) {
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(label.toStdString().c_str());
        textSource->Update();

        vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        textMapper->SetInputConnection(textSource->GetOutputPort());

        selectedTextActor = vtkSmartPointer<vtkFollower>::New();
        selectedTextActor->SetMapper(textMapper);
        selectedTextActor->SetCamera(renderer->GetActiveCamera());
        const double s = overlayWorldScaleAt(point.X(), point.Y(), point.Z());
        selectedTextActor->SetPosition(point.X() + 0.15 * s, point.Y() + 0.15 * s, point.Z() + 0.15 * s);
        selectedTextActor->GetProperty()->SetColor(0.10, 0.62, 0.36);
        selectedTextActor->GetProperty()->SetAmbient(0.55);
        selectedTextActor->GetProperty()->SetDiffuse(0.45);
        selectedTextActor->SetScale(0.06 * s);

        addReferenceActor(selectedTextActor);
    }
    
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// 清除选中的点显示
void Widget::clearSelectedPoint()
{
    if (!renderer) {
        return;
    }
    
    if (selectedPointActor) {
        removeSceneActor(selectedPointActor);
        selectedPointActor = nullptr;
    }
    if (selectedTextActor) {
        removeSceneActor(selectedTextActor);
        selectedTextActor = nullptr;
    }
    
    hasSelectedOriginPoint = false;
    
    // 强制渲染更新
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

// -----------------------------
// 捕捉点（Snap Point）
// -----------------------------

void Widget::mergeSnapFiltersFromToolbarAndCaptureUi()
{
    if (!ui) return;

    const bool oldArcMid = snap_.arcMidpoint;
    const bool oldMid = snap_.midpoint;
    const bool arcMidOnlyPick = oldArcMid && !oldMid;

    const auto checked = [](const QPushButton* b) { return b && b->isChecked(); };

    // Tab 点类型：仅在 Tab「启用捕捉点」打开时参与合并（建模/草图两页总开关镜像）
    const bool tabSnapMaster = checked(ui->pushButton_71) || checked(ui->pushButton_70);

    const bool mEp = tabSnapMaster && checked(ui->pushButton_139);
    const bool mMid = tabSnapMaster && checked(ui->pushButton_143);
    const bool mCtl = tabSnapMaster && checked(ui->pushButton_146);
    const bool mPol = tabSnapMaster && checked(ui->pushButton_150);
    const bool mDef = tabSnapMaster && checked(ui->pushButton_151);
    const bool mInt = tabSnapMaster && checked(ui->pushButton_147);
    const bool mCen = tabSnapMaster && checked(ui->pushButton_141);
    const bool mQuad = tabSnapMaster && checked(ui->pushButton_135);
    const bool mExist = tabSnapMaster && checked(ui->pushButton_136);
    const bool mOnCrv = tabSnapMaster && checked(ui->pushButton_133);
    const bool mOnFace = tabSnapMaster && checked(ui->pushButton_149);
    const bool mGrid = tabSnapMaster && checked(ui->pushButton_138);

    const bool tbEp = mEp || mExist || mDef || mPol;
    const bool tbMid = mMid || mCtl || mDef;
    const bool tbArcLine = mCtl;
    const bool tbInt = mInt;
    const bool tbCen = mCen;
    const bool tbQuad = mQuad || mPol;
    const bool tbNear = mGrid;

    const bool capNear = checked(ui->Capture_Closed);
    const bool capEp = checked(ui->Capture_Endpoint);
    const bool capMidBtn = checked(ui->Capture_Midpoint);
    const bool capInt = checked(ui->Capture_Insertsectionpoint);
    const bool capCen = checked(ui->Capture_Arccenterpoint);
    const bool capQuad = checked(ui->Capture_Quadrantpoint);

    snap_.nearest = capNear || tbNear;
    snap_.endpoint = capEp || tbEp || tbNear || capNear;
    snap_.intersection = capInt || tbInt;
    snap_.center = capCen || tbCen;
    snap_.quadrant = capQuad || tbQuad;
    snap_.onCurve = mOnCrv;
    snap_.onFace = mOnFace;

    snap_.midpoint = tbMid || (capMidBtn && !arcMidOnlyPick);
    snap_.arcMidpoint = tbArcLine || (capMidBtn && arcMidOnlyPick);

    snap_.enabled = checked(ui->Use_Capture) || tabSnapMaster;
    snap_.armed = snap_.enabled;

    updateSnapTypeFilterButtonsEnabled();
    updateTabSnapTypeFilterButtonsEnabled();
    updateSnapPickGhostPresentation();
}

void Widget::clearTabPointSnapToolbarButtons()
{
    if (!ui) return;
    QPushButton* const all[] = {
        ui->pushButton_139, ui->pushButton_143, ui->pushButton_146, ui->pushButton_150, ui->pushButton_151,
        ui->pushButton_147, ui->pushButton_141, ui->pushButton_135, ui->pushButton_136, ui->pushButton_133,
        ui->pushButton_149, ui->pushButton_138,
        ui->pushButton_49,  ui->pushButton_50,  ui->pushButton_51,  ui->pushButton_52,  ui->pushButton_53,
        ui->pushButton_54,  ui->pushButton_55,  ui->pushButton_56,  ui->pushButton_57,  ui->pushButton_58,
        ui->pushButton_59,  ui->pushButton_60,
    };
    for (QPushButton* b : all) {
        if (b) {
            QSignalBlocker blk(b);
            b->setChecked(false);
        }
    }
}

void Widget::syncTabPointSnapToolbarsFromCaptureRow()
{
    if (!ui) return;
    auto pair = [](QPushButton* m, QPushButton* s, bool on) {
        if (m) {
            QSignalBlocker b(m);
            m->setChecked(on);
        }
        if (s) {
            QSignalBlocker b(s);
            s->setChecked(on);
        }
    };
    pair(ui->pushButton_139, ui->pushButton_49, ui->Capture_Endpoint->isChecked());
    pair(ui->pushButton_143, ui->pushButton_50, ui->Capture_Midpoint->isChecked());
    pair(ui->pushButton_147, ui->pushButton_54, ui->Capture_Insertsectionpoint->isChecked());
    pair(ui->pushButton_141, ui->pushButton_55, ui->Capture_Arccenterpoint->isChecked());
    pair(ui->pushButton_135, ui->pushButton_56, ui->Capture_Quadrantpoint->isChecked());
    // 「点在曲线上 / 面上的点」仅 Tab 点选择器有独立按钮，不与 Capture_Closed 绑定
}

void Widget::setupTabPointSnapToolbars()
{
    if (!ui) return;

    static const QString kTabSnapToolbarStyle = QStringLiteral(
        "QPushButton:checked {"
        "  background-color: #2c5f8f;"
        "  color: #ffffff;"
        "  border: 1px solid #1c4266;"
        "}"
        "QPushButton#pushButton_71:checked,"
        "QPushButton#pushButton_70:checked {"
        "  background-color: #257347;"
        "  color: #ffffff;"
        "  border: 1px solid #1a5230;"
        "}");
    if (ui->groupBox_18)
        ui->groupBox_18->setStyleSheet(kTabSnapToolbarStyle);
    if (ui->groupBox_9)
        ui->groupBox_9->setStyleSheet(kTabSnapToolbarStyle);

    auto makeToggle = [](QPushButton* btn) {
        if (!btn) return;
        btn->setCheckable(true);
    };

    QPushButton* const modeling[] = {
        ui->pushButton_139, ui->pushButton_143, ui->pushButton_146, ui->pushButton_150, ui->pushButton_151,
        ui->pushButton_147, ui->pushButton_141, ui->pushButton_135, ui->pushButton_136, ui->pushButton_133,
        ui->pushButton_149, ui->pushButton_138,
    };
    QPushButton* const sketch[] = {
        ui->pushButton_49, ui->pushButton_50, ui->pushButton_51, ui->pushButton_52, ui->pushButton_53,
        ui->pushButton_54, ui->pushButton_55, ui->pushButton_56, ui->pushButton_57, ui->pushButton_58,
        ui->pushButton_59, ui->pushButton_60,
    };
    for (QPushButton* b : modeling)
        makeToggle(b);
    for (QPushButton* b : sketch)
        makeToggle(b);
    makeToggle(ui->pushButton_71);
    makeToggle(ui->pushButton_70);
    if (ui->pushButton_71) {
        QSignalBlocker z71(ui->pushButton_71);
        ui->pushButton_71->setChecked(false);
    }
    if (ui->pushButton_70) {
        QSignalBlocker z70(ui->pushButton_70);
        ui->pushButton_70->setChecked(false);
    }

    static bool s_tabSnapToolbarWired = false;
    if (!s_tabSnapToolbarWired) {
        s_tabSnapToolbarWired = true;

    auto wirePair = [this](QPushButton* modelingBtn, QPushButton* sketchBtn) {
        if (!modelingBtn || !sketchBtn) return;
        auto mirror = [this, modelingBtn, sketchBtn](bool on, QPushButton* src) {
            if (src != modelingBtn) {
                QSignalBlocker b(modelingBtn);
                modelingBtn->setChecked(on);
            }
            if (src != sketchBtn) {
                QSignalBlocker b(sketchBtn);
                sketchBtn->setChecked(on);
            }
            mergeSnapFiltersFromToolbarAndCaptureUi();
        };
        connect(modelingBtn, &QPushButton::toggled, this,
                [mirror, modelingBtn](bool on) { mirror(on, modelingBtn); });
        connect(sketchBtn, &QPushButton::toggled, this, [mirror, sketchBtn](bool on) { mirror(on, sketchBtn); });
    };

    wirePair(ui->pushButton_139, ui->pushButton_49);
    wirePair(ui->pushButton_143, ui->pushButton_50);
    wirePair(ui->pushButton_146, ui->pushButton_51);
    wirePair(ui->pushButton_150, ui->pushButton_52);
    wirePair(ui->pushButton_151, ui->pushButton_53);
    wirePair(ui->pushButton_147, ui->pushButton_54);
    wirePair(ui->pushButton_141, ui->pushButton_55);
    wirePair(ui->pushButton_135, ui->pushButton_56);
    wirePair(ui->pushButton_136, ui->pushButton_57);
    wirePair(ui->pushButton_133, ui->pushButton_58);
    wirePair(ui->pushButton_149, ui->pushButton_59);
    wirePair(ui->pushButton_138, ui->pushButton_60);

        QPushButton* m71 = ui->pushButton_71;
        QPushButton* s70 = ui->pushButton_70;
        if (m71 && s70) {
            auto mirrorTabSnapMaster = [this](bool on, QPushButton* src) {
                QPushButton* a = ui->pushButton_71;
                QPushButton* b = ui->pushButton_70;
                if (!a || !b) return;
                if (src != a) {
                    QSignalBlocker ba(a);
                    a->setChecked(on);
                }
                if (src != b) {
                    QSignalBlocker bb(b);
                    b->setChecked(on);
                }
                if (!on) {
                    clearTabPointSnapToolbarButtons();
                    clearSnapPersistentPoints();
                    clearSnapHover();
                    clearSnapSelected();
                }
                mergeSnapFiltersFromToolbarAndCaptureUi();
            };
            connect(m71, &QPushButton::toggled, this,
                    [mirrorTabSnapMaster, m71](bool on) { mirrorTabSnapMaster(on, m71); });
            connect(s70, &QPushButton::toggled, this,
                    [mirrorTabSnapMaster, s70](bool on) { mirrorTabSnapMaster(on, s70); });
        }

    } // s_tabSnapToolbarWired

    mergeSnapFiltersFromToolbarAndCaptureUi();
}

void Widget::setupSnapPointUI()
{
    // UI 上的捕捉点按钮来自 main_window.ui（点选择过滤器区域）
    // 将它们设置为可切换，并绑定到内部状态。
    if (!ui) return;

    auto makeToggle = [](QPushButton* btn) {
        if (!btn) return;
        btn->setCheckable(true);
        btn->setChecked(false);
    };

    makeToggle(ui->Use_Capture);
    makeToggle(ui->Capture_Closed);
    makeToggle(ui->Capture_Endpoint);
    makeToggle(ui->Capture_Midpoint);
    makeToggle(ui->Capture_Insertsectionpoint);
    makeToggle(ui->Capture_Arccenterpoint);
    makeToggle(ui->Capture_Quadrantpoint);

    // 选中态加深底纹（默认可勾选按钮在 Windows 主题下对比度不足）
    if (ui->groupBox_15) {
        ui->groupBox_15->setStyleSheet(QStringLiteral(
            "QPushButton#Use_Capture:checked {"
            "  background-color: #257347;"
            "  color: #ffffff;"
            "  border: 1px solid #1a5230;"
            "}"
            "QPushButton#Capture_Closed:checked,"
            "QPushButton#Capture_Endpoint:checked,"
            "QPushButton#Capture_Midpoint:checked,"
            "QPushButton#Capture_Insertsectionpoint:checked,"
            "QPushButton#Capture_Arccenterpoint:checked,"
            "QPushButton#Capture_Quadrantpoint:checked {"
            "  background-color: #2c5f8f;"
            "  color: #ffffff;"
            "  border: 1px solid #1c4266;"
            "}"));
    }

    // 初始状态：全部关闭
    snap_ = SnapSettings{};
    ui->Use_Capture->setChecked(false);
    ui->Capture_Closed->setChecked(false);
    ui->Capture_Endpoint->setChecked(false);
    ui->Capture_Midpoint->setChecked(false);
    ui->Capture_Insertsectionpoint->setChecked(false);
    ui->Capture_Arccenterpoint->setChecked(false);
    ui->Capture_Quadrantpoint->setChecked(false);

    // 用 toggled：Ribbon 通过 setChecked 同步时不会发 clicked，否则 armed 状态与 UI 脱节
    connect(ui->Use_Capture, &QPushButton::toggled, this, [this](bool on) {
        if (!on) {
            clearSnapSettings();
            return;
        }
        statusBar()->showMessage(tr("捕捉点已启用：请先选择需要的捕捉类型（端点/中点/交点/圆心/象限点），再移动鼠标预览。"), 5000);
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });

    connect(ui->Capture_Closed, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });
    connect(ui->Capture_Endpoint, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });
    connect(ui->Capture_Midpoint, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });
    connect(ui->Capture_Insertsectionpoint, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });
    connect(ui->Capture_Arccenterpoint, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });
    connect(ui->Capture_Quadrantpoint, &QPushButton::toggled, this, [this](bool) {
        mergeSnapFiltersFromToolbarAndCaptureUi();
    });

    connect(ui->Capture_Clear, &QPushButton::clicked, this, [this]() {
        clearSnapSettings();
    });

    setupTabPointSnapToolbars();
}

void Widget::updateSnapTypeFilterButtonsEnabled()
{
    if (!ui) return;
    const bool on = ui->Use_Capture->isChecked();
    QPushButton* const filters[] = {
        ui->Capture_Closed,
        ui->Capture_Endpoint,
        ui->Capture_Midpoint,
        ui->Capture_Insertsectionpoint,
        ui->Capture_Arccenterpoint,
        ui->Capture_Quadrantpoint,
    };
    for (QPushButton* b : filters) {
        if (b) b->setEnabled(on);
    }
}

void Widget::updateTabSnapTypeFilterButtonsEnabled()
{
    if (!ui) return;
    const bool tabOn = (ui->pushButton_71 && ui->pushButton_71->isChecked())
                       || (ui->pushButton_70 && ui->pushButton_70->isChecked());
    QPushButton* const tabTypes[] = {
        ui->pushButton_139, ui->pushButton_143, ui->pushButton_146, ui->pushButton_150, ui->pushButton_151,
        ui->pushButton_147, ui->pushButton_141, ui->pushButton_135, ui->pushButton_136, ui->pushButton_133,
        ui->pushButton_149, ui->pushButton_138,
        ui->pushButton_49,  ui->pushButton_50,  ui->pushButton_51,  ui->pushButton_52,  ui->pushButton_53,
        ui->pushButton_54,  ui->pushButton_55,  ui->pushButton_56,  ui->pushButton_57,  ui->pushButton_58,
        ui->pushButton_59,  ui->pushButton_60,
    };
    for (QPushButton* b : tabTypes) {
        if (b) b->setEnabled(tabOn);
    }
}

void Widget::setSnapArmed(bool armed)
{
    snap_.armed = armed;
    if (!armed) {
        clearSnapHover();
        clearSnapSelected();
    }
    updateSnapPickGhostPresentation();
}

void Widget::updateSnapPickGhostPresentation()
{
    // 拉伸/旋转进行中时不抢幽灵模式所有权
    const bool featureDialogBusy =
        (extrusionDialog != nullptr) || (revolveDialog != nullptr)
        || currentSelectionMode == ExtrusionSelection
        || currentSelectionMode == EdgeSelection
        || currentSelectionMode == FaceSelection
        || currentSelectionMode == ExtrusionHandleDrag
        || currentSelectionMode == RevolveHandleDrag;

    const bool inPointPickUi =
        originSnapSelectionActive_
        || currentSelectionMode == PointSelection
        || currentSelectionMode == VectorDialogPickStartPoint
        || currentSelectionMode == VectorDialogPickEndPoint
        || currentSelectionMode == VectorTwoPointHandleDrag;

    const bool wantSnapGhost = (snap_.armed || inPointPickUi) && !featureDialogBusy;

    if (wantSnapGhost) {
        if (currentSelectedIndex >= 0) {
            currentSelectedIndex = -1;
            clearSubShapeHighlight();
            updateHistoryListSelection();
        }
        if (!featureOperationGhostMode_) {
            setFeatureOperationGhostMode(true);
            snapPickGhostOwned_ = true;
        }
        return;
    }

    if (snapPickGhostOwned_) {
        snapPickGhostOwned_ = false;
        setFeatureOperationGhostMode(false);
    }
}

void Widget::startOriginSnapSelection(OriginDialogKind kind, int snapKind)
{
    pendingOriginDialogKind_ = kind;
    pendingOriginSnapKind_ = snapKind;
    originSnapSelectionActive_ = true;
    currentSelectionMode = None; // 只让点击触发 snap 捕捉

    // 清空原有捕捉状态与 UI 勾选
    clearSnapSettings();

    if (!ui) return;

    // 开启捕捉总开关（用于保持交互一致）
    snap_.enabled = true;
    snap_.armed = true;
    ui->Use_Capture->setChecked(true);

    // 设置捕捉类型（最近点）
    snap_.nearest = (snapKind == 0);
    snap_.endpoint = (snapKind == 1);
    snap_.midpoint = (snapKind == 2);
    snap_.intersection = (snapKind == 3);
    snap_.center = (snapKind == 4);
    snap_.quadrant = (snapKind == 5);

    // “最近点”语义：在所有类型中找离鼠标最近的点（不是只找端点）
    if (snapKind == 0) {
        // 稳定优先：为“原点捕捉”的最近点，先只扫描端点+中点
        // 这样避免在 nearest 分支里额外做交点/圆心/象限相关更复杂计算而导致崩溃。
        snap_.endpoint = true;
        snap_.midpoint = true;
        snap_.intersection = false;
        // 仍允许圆心/象限点（来自有限边扫描里的圆曲线），但不做交点计算
        snap_.center = true;
        snap_.quadrant = true;
    }

    ui->Capture_Closed->setChecked(snap_.nearest);
    ui->Capture_Endpoint->setChecked(snap_.endpoint);
    ui->Capture_Midpoint->setChecked(snap_.midpoint);
    ui->Capture_Insertsectionpoint->setChecked(snap_.intersection);
    ui->Capture_Arccenterpoint->setChecked(snap_.center);
    ui->Capture_Quadrantpoint->setChecked(snap_.quadrant);

    syncTabPointSnapToolbarsFromCaptureRow();
    mergeSnapFiltersFromToolbarAndCaptureUi();

    if (vtkWidget) {
        vtkWidget->setFocus();
    }
    if (statusBar()) {
        statusBar()->showMessage(tr("捕捉原点：请在模型上点击对应点。"), 3000);
    }
}

void Widget::applyOriginFromSnap(const gp_Pnt& p, const QString& chosenLabel)
{
    // 用于“最近点”按钮时：即使候选来自端点/中点，仍按需求显示为“最近点”
    QString displayLabel = chosenLabel;
    if (pendingOriginSnapKind_ == 0) {
        displayLabel = tr("最近点");
    }

    // 记录到你现有的“旋转用中心点”逻辑（getRevolutionAxis 依赖）
    selectedOriginPoint = p;
    hasSelectedOriginPoint = true;

    // 同时写回各个创建对话框
    if (pendingOriginDialogKind_ == OriginDialogKind::Cuboid && cuboidDialog) {
        cuboidDialog->setOriginPoint(p.X(), p.Y(), p.Z());
        if (cuboidInteractiveActive_) {
            applyCuboidInteractiveOrigin(p);
        }
    } else if (pendingOriginDialogKind_ == OriginDialogKind::Cylinder && cylinderDialog) {
        cylinderDialog->setOriginPoint(p.X(), p.Y(), p.Z());
    } else if (pendingOriginDialogKind_ == OriginDialogKind::Cone && coneDialog) {
        coneDialog->setOriginPoint(p.X(), p.Y(), p.Z());
    } else if (pendingOriginDialogKind_ == OriginDialogKind::Sphere && sphereDialog) {
        sphereDialog->setOriginPoint(p.X(), p.Y(), p.Z());
    } else if (pendingOriginDialogKind_ == OriginDialogKind::Pattern && patternDialog_) {
        patternDialog_->setRotationCenter(p, true);
        if (patternDialog_->hasDirection1()) {
            updatePatternRotationAxisArrow();
        }
    } else {
        // Revolve：不需要对话框内部 setOriginPoint（旋转轴通过 hasSelectedOriginPoint 读取）
    }

    // 显示选中的点（复用你原来的点选择显示逻辑）
    QString label = QString("%1\n(%2, %3, %4)")
                        .arg(displayLabel)
                        .arg(p.X(), 0, 'f', 2)
                        .arg(p.Y(), 0, 'f', 2)
                        .arg(p.Z(), 0, 'f', 2);
    showSelectedPoint(p, label);
    if (renderer) {
        clearPointSelectionHover();
    }

    originSnapSelectionActive_ = false;
    pendingOriginDialogKind_ = OriginDialogKind::None;
    pendingOriginSnapKind_ = -1;
    if (cuboidInteractiveActive_) {
        currentSelectionMode = CuboidInteractive;
    } else if (patternDialog_) {
        restorePatternPitchInteractiveAfterOriginPick();
    } else {
        currentSelectionMode = None;
    }

    // 用你现有的捕捉点逻辑清理掉 snap UI 与“常驻捕捉点”
    // 轻量化清理：避免在 pick 回调里执行 clearSnapSettings()（其中包含拾取绑定重建）
    // 这可能导致 VTK/Picker 状态在回调栈内被破坏从而崩溃。
    snap_.enabled = false;
    setSnapArmed(false); // 内部会清 snapHover / snapSelected

    // 清空捕捉类型 UI 勾选（不做 pick 绑定重建）
    if (ui) {
        ui->Use_Capture->setChecked(false);
        ui->Capture_Closed->setChecked(false);
        ui->Capture_Endpoint->setChecked(false);
        ui->Capture_Midpoint->setChecked(false);
        ui->Capture_Insertsectionpoint->setChecked(false);
        ui->Capture_Arccenterpoint->setChecked(false);
        ui->Capture_Quadrantpoint->setChecked(false);
        clearTabPointSnapToolbarButtons();
        if (ui->pushButton_71) {
            QSignalBlocker b71(ui->pushButton_71);
            ui->pushButton_71->setChecked(false);
        }
        if (ui->pushButton_70) {
            QSignalBlocker b70(ui->pushButton_70);
            ui->pushButton_70->setChecked(false);
        }
    }
    mergeSnapFiltersFromToolbarAndCaptureUi();

    // 删除已经加入场景的“常驻捕捉点”（避免在 VTK 事件回调栈内强制 Render）
    if (renderer) {
        for (const auto& a : snapPersistentPointActors_) {
            if (a) removeSceneActor(a);
        }
        snapPersistentPointActors_.clear();
        snapPersistentPoints_.clear();
    } else {
        // 退化：renderer 不存在时再走原逻辑
        clearSnapPersistentPoints();
    }
}

void Widget::clearSnapSettings()
{
    snap_ = SnapSettings{};
    if (ui) {
        {
            QSignalBlocker b(ui->Use_Capture);
            ui->Use_Capture->setChecked(false);
        }
        ui->Capture_Closed->setChecked(false);
        ui->Capture_Endpoint->setChecked(false);
        ui->Capture_Midpoint->setChecked(false);
        ui->Capture_Insertsectionpoint->setChecked(false);
        ui->Capture_Arccenterpoint->setChecked(false);
        ui->Capture_Quadrantpoint->setChecked(false);
        clearTabPointSnapToolbarButtons();
        if (ui->pushButton_71) {
            QSignalBlocker b71(ui->pushButton_71);
            ui->pushButton_71->setChecked(false);
        }
        if (ui->pushButton_70) {
            QSignalBlocker b70(ui->pushButton_70);
            ui->pushButton_70->setChecked(false);
        }
    }
    setSnapArmed(false);
    // 清除捕捉点设置时，同时清除所有已捕捉的常驻点
    clearSnapPersistentPoints();

    // 重要：恢复所有可见模型的拾取绑定（避免捕捉模式退出后无法拾取）
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible && historyList[i].type != DATUM_PLANE
                                            && historyList[i].type != DATUM_AXIS
                                            && historyList[i].type != WORK_CSYS
                                            && historyList[i].type != REFERENCE_CSYS);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }
    if (shapePicker && renderer) {
        shapePicker->SetRenderer(renderer);
    }
    mergeSnapFiltersFromToolbarAndCaptureUi();
    statusBar()->showMessage(tr("已清除所有捕捉点设置。"), 3000);
}

void Widget::clearVectorTwoPointSnapGhosts()
{
    if (!renderer) return;
    for (const auto& a : vectorTwoPointSnapGhostActors_) {
        if (a) removeSceneActor(a);
    }
    vectorTwoPointSnapGhostActors_.clear();
}

void Widget::clearSnapHover()
{
    if (!renderer) return;
    hasSnapHoverBestPoint_ = false;
    clearVectorTwoPointSnapGhosts();
    if (snapHoverPointActor_) {
        removeSceneActor(snapHoverPointActor_);
        snapHoverPointActor_ = nullptr;
    }
    if (snapHoverTextActor_) {
        removeSceneActor(snapHoverTextActor_);
        snapHoverTextActor_ = nullptr;
    }
    if (snapHoverShapeActor_) {
        removeSceneActor(snapHoverShapeActor_);
        snapHoverShapeActor_ = nullptr;
    }
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearSnapSelected()
{
    if (!renderer) return;
    if (snapSelectedPointActor_) {
        removeSceneActor(snapSelectedPointActor_);
        snapSelectedPointActor_ = nullptr;
    }
    if (snapSelectedShapeActor_) {
        removeSceneActor(snapSelectedShapeActor_);
        snapSelectedShapeActor_ = nullptr;
    }
    hasSnapSelectedPoint_ = false;
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::addSnapPersistentPoint(const gp_Pnt& p)
{
    if (!renderer) return;

    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    configureMarkerSphereSource(sphere, 0.09);
    sphere->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPosition(p.X(), p.Y(), p.Z());
    applyMarkerSphereMaterial(actor->GetProperty(), MarkerSphereStyle::ConfirmedGreen);
    actor->SetPickable(false);
    {
        const double s = overlayWorldScaleAt(p.X(), p.Y(), p.Z());
        actor->SetScale(s, s, s);
    }

    addReferenceActor(actor);
    snapPersistentPoints_.append(p);
    snapPersistentPointActors_.append(actor);

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearSnapPersistentPoints()
{
    if (!renderer) return;
    for (const auto& a : snapPersistentPointActors_) {
        if (a) {
            removeSceneActor(a);
        }
    }
    snapPersistentPointActors_.clear();
    snapPersistentPoints_.clear();
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

vtkSmartPointer<vtkActor> Widget::buildSnapShapeHighlightActor(const TopoDS_Shape& shape,
                                                              const double r, const double g, const double b,
                                                              const double opacity,
                                                              const double lineWidth)
{
    if (shape.IsNull() || !renderer) {
        return nullptr;
    }

    // 与拉伸高亮一致：先离散化，再用 IVtkOCC_ShapeMesher 构建 vtkPolyData
    try {
        BRepMesh_IncrementalMesh mesh(shape, 0.05, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(shape);
        hlShape->SetId(888888); // 捕捉点高亮专用ID

        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(hlShape, shapeData);
        vtkPolyData* pd = shapeData->getVtkPolyData();
        if (!pd || pd->GetNumberOfPoints() <= 0) {
            return nullptr;
        }

        // 关键：pd 由局部 shapeData 持有，函数返回后可能销毁导致悬空指针。
        // 必须拷贝一份由 VTK 智能指针长期持有的数据，避免下一次渲染/交互崩溃。
        vtkSmartPointer<vtkPolyData> ownedPd = vtkSmartPointer<vtkPolyData>::New();
        ownedPd->ShallowCopy(pd);

        // 对 EDGE：线高亮；对 FACE：半透明面高亮（避免显示三角剖分条纹）
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(ownedPd);
        mapper->ScalarVisibilityOff();
        mapper->SetResolveCoincidentTopologyToPolygonOffset();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetOpacity(opacity);
        if (shape.ShapeType() == TopAbs_FACE) {
            actor->GetProperty()->SetRepresentationToSurface();
            actor->GetProperty()->EdgeVisibilityOff();
            actor->GetProperty()->SetLighting(false);
            actor->GetProperty()->SetInterpolationToFlat();
            actor->GetProperty()->SetAmbient(1.0);
            actor->GetProperty()->SetDiffuse(0.0);
            actor->GetProperty()->SetSpecular(0.0);
            // 面高亮在某些视角不明显时，提升覆盖强度
            actor->GetProperty()->SetOpacity(qMax(0.72, opacity));
        } else {
            actor->GetProperty()->SetRepresentationToWireframe();
            actor->GetProperty()->SetLineWidth(lineWidth);
        }
        actor->SetPickable(false);
        return actor;
    } catch (...) {
        return nullptr;
    }
}

static double snapScreenDist2(vtkRenderer* renderer, const gp_Pnt& p, int sx, int sy)
{
    if (!renderer) return 1e100;
    renderer->SetWorldPoint(p.X(), p.Y(), p.Z(), 1.0);
    renderer->WorldToDisplay();
    double d[3];
    renderer->GetDisplayPoint(d);
    const double dx = d[0] - static_cast<double>(sx);
    const double dy = d[1] - static_cast<double>(sy);
    return dx * dx + dy * dy;
}

/** 由屏幕坐标构造拾取射线（Display→World near/far） */
static bool buildPickRayFromDisplay(vtkRenderer* renderer, int x, int y, gp_Lin& outRay)
{
    if (!renderer) return false;
    double worldNear[4] = {0, 0, 0, 1};
    double worldFar[4] = {0, 0, 0, 1};
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
    gp_Pnt p0(worldNear[0], worldNear[1], worldNear[2]);
    gp_Pnt p1(worldFar[0], worldFar[1], worldFar[2]);
    gp_Vec v(p0, p1);
    if (v.Magnitude() <= Precision::Confusion()) return false;
    outRay = gp_Lin(p0, gp_Dir(v));
    return true;
}

/** 射线与边曲线最近点 →「点在曲线上」 */
static bool snapProjectRayOntoEdge(const TopoDS_Edge& edge, const gp_Lin& ray, gp_Pnt& outPoint)
{
    Standard_Real f = 0.0, l = 0.0;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
    if (curve.IsNull()) return false;
    try {
        Handle(Geom_Line) rayCurve = new Geom_Line(ray);
        GeomAPI_ExtremaCurveCurve extrema(rayCurve, curve, -1.0e6, 1.0e6, f, l);
        if (extrema.NbExtrema() < 1) return false;
        gp_Pnt onRay, onCurve;
        extrema.NearestPoints(onRay, onCurve);
        // 再投影一次并钳制到边参数域，避免极值落在修剪外
        gp_Pnt projected;
        if (SketchGeometry::projectPointOntoEdge(onCurve, edge, projected)) {
            outPoint = projected;
        } else {
            outPoint = onCurve;
        }
        return true;
    } catch (...) {
        return false;
    }
}

/** 射线与面求交（最近交点）→「面上的点」 */
static bool snapProjectRayOntoFace(const TopoDS_Face& face, const gp_Lin& ray, gp_Pnt& outPoint)
{
    try {
        IntCurvesFace_ShapeIntersector intersector;
        intersector.Load(face, Precision::Confusion());
        intersector.Perform(ray, 0.0, 1.0e9);
        if (intersector.NbPnt() <= 0) return false;
        Standard_Real bestW = RealLast();
        bool found = false;
        gp_Pnt bestP;
        for (int i = 1; i <= intersector.NbPnt(); ++i) {
            const Standard_Real w = intersector.WParameter(i);
            if (w >= 0.0 && w < bestW) {
                bestW = w;
                bestP = intersector.Pnt(i);
                found = true;
            }
        }
        if (!found) return false;
        outPoint = bestP;
        return true;
    } catch (...) {
        return false;
    }
}

static constexpr double kSketchSnapScreenPixelRadius = 110.0;
/** 曲线上/面上点：屏幕像素容差（比特征点略宽，便于沿边/面滑动） */
static constexpr double kOnCurveFaceSnapScreenPixelRadius = 48.0;

void Widget::appendActiveSketchSnapScreenCandidates(int x, int y, double maxScreenDist2,
                                                    QList<SketchSnapScreenCandidate>& out) const
{
    if (!renderer) return;
    if (!snap_.endpoint && !snap_.midpoint && !snap_.arcMidpoint && !snap_.intersection && !snap_.center
        && !snap_.quadrant && !snap_.onCurve) {
        return;
    }

    const auto addIfNear = [&](const QString& lab, const gp_Pnt& pt, const TopoDS_Shape& ref = TopoDS_Shape(),
                               bool hasRef = false) {
        const double d2 = snapScreenDist2(renderer, pt, x, y);
        if (d2 <= maxScreenDist2)
            out.append({lab, pt, d2, ref, hasRef});
    };

    auto appendEdgesFromShape = [](const TopoDS_Shape& g, QList<TopoDS_Edge>& edges) {
        if (g.IsNull()) return;
        if (g.ShapeType() == TopAbs_EDGE) {
            edges.append(TopoDS::Edge(g));
            return;
        }
        for (TopExp_Explorer ex(g, TopAbs_EDGE); ex.More(); ex.Next()) {
            edges.append(TopoDS::Edge(ex.Current()));
        }
    };

    gp_Lin pickRay;
    const bool hasPickRay = snap_.onCurve && buildPickRayFromDisplay(renderer, x, y, pickRay);
    const double onCurveMaxD2 = kOnCurveFaceSnapScreenPixelRadius * kOnCurveFaceSnapScreenPixelRadius;

    QList<TopoDS_Edge> edges;
    // 当前活动草图
    if (hasActiveSketch_) {
        for (const TopoDS_Shape& g : activeSketch_.getGeometries()) {
            appendEdgesFromShape(g, edges);
        }
    }
    // 历史中已落盘的草图（点拾取器也需支持）
    for (int hi = 0; hi < historyList.size(); ++hi) {
        const ModelingHistory& rec = historyList[hi];
        if (rec.type != SKETCH) continue;
        if (!rec.actor || rec.actor->GetVisibility() == 0) continue;
        if (rec.occShape.IsNull()) continue;
        // 活动草图对应的历史项已由 activeSketch_ 覆盖，避免重复
        if (hasActiveSketch_ && hi == activeSketchHistoryIndex_) continue;
        appendEdgesFromShape(rec.occShape, edges);
    }

    if (edges.isEmpty()) return;

    for (const TopoDS_Edge& edge : edges) {
        Standard_Real f = 0.0, l = 0.0;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
        if (curve.IsNull()) continue;
        const Handle(Geom_Circle) circ = SketchGeometry::sketchCircleBasis(curve);

        if (snap_.endpoint) {
            const gp_Pnt p0 = curve->Value(f);
            const gp_Pnt p1 = curve->Value(l);
            addIfNear(tr("端点"), p0, edge, true);
            addIfNear(tr("端点"), p1, edge, true);
        }

        if (snap_.midpoint) {
            addIfNear(tr("中点"), curve->Value((f + l) * 0.5), edge, true);
        }

        if (snap_.onCurve && hasPickRay) {
            gp_Pnt pOn;
            if (snapProjectRayOntoEdge(edge, pickRay, pOn)) {
                const double d2 = snapScreenDist2(renderer, pOn, x, y);
                if (d2 <= onCurveMaxD2) {
                    out.append({tr("点在曲线上"), pOn, d2, edge, true});
                }
            }
        }

        if (snap_.arcMidpoint) {
            if (!circ.IsNull()) {
                addIfNear(tr("圆弧中点"), curve->Value((f + l) * 0.5), edge, true);
            }
        }

        if (!circ.IsNull()) {
            const gp_Pnt c = circ->Location();
            if (snap_.center) {
                addIfNear(tr("圆心"), c, edge, true);
            }
            if (snap_.quadrant) {
                const gp_Ax2 ax = circ->Position();
                const gp_Dir xd = ax.XDirection();
                const gp_Dir yd = ax.YDirection();
                const double r = circ->Radius();
                addIfNear(tr("象限点"), gp_Pnt(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r), edge, true);
                addIfNear(tr("象限点"), gp_Pnt(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r), edge, true);
                addIfNear(tr("象限点"), gp_Pnt(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r), edge, true);
                addIfNear(tr("象限点"), gp_Pnt(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r), edge, true);
            }
        }
    }

    if (snap_.intersection && edges.size() >= 2) {
        const double tol = 1e-3;
        for (int i = 0; i < edges.size(); ++i) {
            for (int j = i + 1; j < edges.size(); ++j) {
                gp_Pnt pi;
                if (SketchGeometry::edgeIntersectionPoint(edges[i], edges[j], pi, tol)) {
                    addIfNear(tr("交点"), pi, TopoDS_Shape(), false);
                }
            }
        }
    }

    if (snap_.endpoint) {
        auto addVertices = [&](const TopoDS_Shape& g) {
            if (g.IsNull()) return;
            for (TopExp_Explorer ex(g, TopAbs_VERTEX); ex.More(); ex.Next()) {
                const gp_Pnt pv = BRep_Tool::Pnt(TopoDS::Vertex(ex.Current()));
                addIfNear(tr("端点"), pv, TopoDS_Shape(), false);
            }
        };
        if (hasActiveSketch_) {
            for (const TopoDS_Shape& g : activeSketch_.getGeometries()) {
                addVertices(g);
            }
        }
        for (int hi = 0; hi < historyList.size(); ++hi) {
            const ModelingHistory& rec = historyList[hi];
            if (rec.type != SKETCH) continue;
            if (!rec.actor || rec.actor->GetVisibility() == 0) continue;
            if (rec.occShape.IsNull()) continue;
            if (hasActiveSketch_ && hi == activeSketchHistoryIndex_) continue;
            addVertices(rec.occShape);
        }
    }
}

void Widget::refreshSnapHoverAfterSketchMouseMove(int x, int y)
{
    if (snap_.armed)
        updateSnapHover(x, y);
    else
        clearSnapHover();
}

void Widget::updateSnapHover(int x, int y, SnapHoverOptions options)
{
    if (!snap_.armed || !shapePicker || !renderer) return;

    // 捕捉范围：拖拽/扩大半径拾取时用更大容差
    const double snapTolerance =
        (options.expandScreenPixelRadius > 0.0) ? 0.18
        : (snap_.nearest ? 0.25 : 0.05);

    // 拾取前准备：确保可见模型绑定 ShapeSource，且 ShapeDataSource 已更新
    // 否则在一些模型（例如长方体的棱）上悬停时可能取不到 EDGE 子形状
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
                // 仅解除 ShapeSource 绑定即可避免隐藏遮挡；
                // 若从 picker 内部映射移除，恢复可见时可能出现高亮/子形状拾取异常
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    // 没有任何捕捉类型开启：不显示
    if (!snap_.endpoint && !snap_.midpoint && !snap_.arcMidpoint && !snap_.intersection && !snap_.center && !snap_.quadrant
        && !snap_.nearest && !snap_.onCurve && !snap_.onFace) {
        clearSnapHover();
        return;
    }

    // 为了性能：只对“当前鼠标附近”的可选子形状计算候选点
    struct Candidate {
        QString label;
        gp_Pnt p;
        double d2 = 0.0;
        TopoDS_Shape refShape;   // 用于高亮的关联形状（一般为 EDGE）
        bool hasRef = false;
    };
    QList<Candidate> candidates;

    gp_Lin pickRay;
    const bool hasPickRay = (snap_.onCurve || snap_.onFace) && buildPickRayFromDisplay(renderer, x, y, pickRay);
    const double onGeomMaxD2 = kOnCurveFaceSnapScreenPixelRadius * kOnCurveFaceSnapScreenPixelRadius;

    auto collectFromPickedEdges = [&](const IVtk_ShapeIdList& subShapeIds, const Handle(IVtkOCC_Shape)& shapeWrapper) {
        if (shapeWrapper.IsNull()) return;
        QList<TopoDS_Edge> edges;

        for (IVtk_ShapeIdList::Iterator it(subShapeIds); it.More(); it.Next()) {
            const TopoDS_Shape& sh = shapeWrapper->GetSubShape(it.Value());
            if (sh.ShapeType() != TopAbs_EDGE) continue;
            edges.append(TopoDS::Edge(sh));
        }

        // 端点/中点/圆心/象限点/点在曲线上
        for (const TopoDS_Edge& edge : edges) {
            Standard_Real f = 0.0, l = 0.0;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, f, l);
            if (curve.IsNull()) continue;
            const Handle(Geom_Circle) circ = SketchGeometry::sketchCircleBasis(curve);

            if (snap_.endpoint) {
                gp_Pnt p0 = curve->Value(f);
                gp_Pnt p1 = curve->Value(l);
                candidates.append({tr("端点"), p0, snapScreenDist2(renderer, p0, x, y), edge, true});
                candidates.append({tr("端点"), p1, snapScreenDist2(renderer, p1, x, y), edge, true});
            }

            // 中点：普通边中点；圆弧中点：仅对圆曲线显示
            if (snap_.midpoint) {
                gp_Pnt pm = curve->Value((f + l) * 0.5);
                candidates.append({tr("中点"), pm, snapScreenDist2(renderer, pm, x, y), edge, true});
            }

            if (snap_.onCurve && hasPickRay) {
                gp_Pnt pOn;
                if (snapProjectRayOntoEdge(edge, pickRay, pOn)) {
                    const double d2 = snapScreenDist2(renderer, pOn, x, y);
                    if (d2 <= onGeomMaxD2) {
                        candidates.append({tr("点在曲线上"), pOn, d2, edge, true});
                    }
                }
            }

            if (snap_.arcMidpoint && !circ.IsNull()) {
                const gp_Pnt pm = curve->Value((f + l) * 0.5);
                candidates.append({tr("圆弧中点"), pm, snapScreenDist2(renderer, pm, x, y), edge, true});
            }

            // 圆心/象限点：仅对圆（含圆弧/修剪圆）支持
            if (!circ.IsNull()) {
                const gp_Pnt c = circ->Location();
                if (snap_.center) {
                    candidates.append({tr("圆心"), c, snapScreenDist2(renderer, c, x, y), edge, true});
                }
                if (snap_.quadrant) {
                    const gp_Ax2 ax = circ->Position();
                    const gp_Dir xd = ax.XDirection();
                    const gp_Dir yd = ax.YDirection();
                    const double r = circ->Radius();
                    const gp_Pnt q0(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r);
                    const gp_Pnt q1(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r);
                    const gp_Pnt q2(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r);
                    const gp_Pnt q3(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r);
                    candidates.append({tr("象限点"), q0, snapScreenDist2(renderer, q0, x, y), edge, true});
                    candidates.append({tr("象限点"), q1, snapScreenDist2(renderer, q1, x, y), edge, true});
                    candidates.append({tr("象限点"), q2, snapScreenDist2(renderer, q2, x, y), edge, true});
                    candidates.append({tr("象限点"), q3, snapScreenDist2(renderer, q3, x, y), edge, true});
                }
            }
        }

        // 交点：在本次拾取命中的“边集合”里两两求最近距离，接近 0 时认为相交
        if (snap_.intersection && edges.size() >= 2) {
            const double tol = 1e-3;
            for (int i = 0; i < edges.size(); ++i) {
                for (int j = i + 1; j < edges.size(); ++j) {
                    gp_Pnt pi;
                    if (SketchGeometry::edgeIntersectionPoint(edges[i], edges[j], pi, tol)) {
                        candidates.append({tr("交点"), pi, snapScreenDist2(renderer, pi, x, y), TopoDS_Shape(), false});
                    }
                }
            }
        }
    };

    auto collectPicked = [&](const IVtk_SelectionMode mode) {
        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(snapTolerance);
        shapePicker->SetSelectionMode(mode);
        shapePicker->Pick(x, y, 0);

        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (!pickedActors || pickedActors->GetNumberOfItems() == 0) return;

        pickedActors->InitTraversal();
        vtkActor* actor = pickedActors->GetNextActor();
        if (!actor) return;

        IVtkTools_ShapeDataSource* ds = IVtkTools_ShapeObject::GetShapeSource(actor);
        if (!ds) return;
        Handle(IVtkOCC_Shape) sw = ds->GetShape();
        if (sw.IsNull()) return;
        IVtk_IdType shapeID = sw->GetId();
        IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
        if (subIds.IsEmpty()) return;

        // 顶点直接作为端点候选（当 endpoint 开启时）
        if (mode == SM_Vertex && snap_.endpoint) {
            for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
                const TopoDS_Shape& sh = sw->GetSubShape(it.Value());
                if (sh.ShapeType() != TopAbs_VERTEX) continue;
                const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(sh));
                candidates.append({tr("端点"), p, snapScreenDist2(renderer, p, x, y), TopoDS_Shape(), false});
            }
        }

        if (mode == SM_Edge) {
            collectFromPickedEdges(subIds, sw);
        }

        if (mode == SM_Face && snap_.onFace && hasPickRay) {
            for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
                const TopoDS_Shape& sh = sw->GetSubShape(it.Value());
                if (sh.ShapeType() != TopAbs_FACE) continue;
                const TopoDS_Face face = TopoDS::Face(sh);
                gp_Pnt pOn;
                if (!snapProjectRayOntoFace(face, pickRay, pOn)) continue;
                const double d2 = snapScreenDist2(renderer, pOn, x, y);
                if (d2 <= onGeomMaxD2) {
                    candidates.append({tr("面上的点"), pOn, d2, face, true});
                }
            }
        }
    };

    // 采样：顶点 + 边；「面上的点」额外拾取面
    if (snap_.endpoint) collectPicked(SM_Vertex);
    if (snap_.endpoint || snap_.midpoint || snap_.arcMidpoint || snap_.intersection
        || snap_.center || snap_.quadrant || snap_.onCurve || snap_.nearest) {
        collectPicked(SM_Edge);
    }
    if (snap_.onFace) {
        collectPicked(SM_Face);
    }

    {
        const double sketchMaxD2 = kSketchSnapScreenPixelRadius * kSketchSnapScreenPixelRadius;
        QList<SketchSnapScreenCandidate> sketchCand;
        appendActiveSketchSnapScreenCandidates(x, y, sketchMaxD2, sketchCand);
        for (const auto& sc : sketchCand) {
            candidates.append({sc.label, sc.p, sc.d2, sc.refShape, sc.hasRef});
        }
    }

    // 两点矢量拖拽/拾取：在屏幕像素半径内扫描候选，避免必须精确命中边/顶点
    if (options.expandScreenPixelRadius > 0.0) {
        const double maxD2 = options.expandScreenPixelRadius * options.expandScreenPixelRadius;
        auto addIfNear = [&](const QString& t, const gp_Pnt& p, const TopoDS_Shape& ref = TopoDS_Shape(),
                             const bool hasRef = false) {
            const double d2 = snapScreenDist2(renderer, p, x, y);
            if (d2 <= maxD2) {
                candidates.append({t, p, d2, ref, hasRef});
            }
        };

        int edgeCount = 0;
        const int maxEdgesToScan = 800;
        for (const auto& rec : historyList) {
            if (!rec.actor || rec.actor->GetVisibility() == 0) continue;
            if (rec.type == DATUM_PLANE || rec.type == DATUM_AXIS || rec.type == WORK_CSYS
                || rec.type == REFERENCE_CSYS) {
                continue;
            }
            if (rec.occShape.IsNull()) continue;

            if (snap_.endpoint) {
                for (TopExp_Explorer exV(rec.occShape, TopAbs_VERTEX); exV.More(); exV.Next()) {
                    addIfNear(tr("端点"), BRep_Tool::Pnt(TopoDS::Vertex(exV.Current())));
                }
            }

            if (snap_.midpoint || snap_.arcMidpoint || snap_.center || snap_.quadrant || snap_.intersection) {
                QList<TopoDS_Edge> edges;
                for (TopExp_Explorer exE(rec.occShape, TopAbs_EDGE); exE.More(); exE.Next()) {
                    if (edgeCount++ > maxEdgesToScan) break;
                    edges.append(TopoDS::Edge(exE.Current()));
                }

                for (const TopoDS_Edge& e : edges) {
                    Standard_Real f = 0.0, l = 0.0;
                    Handle(Geom_Curve) curve = BRep_Tool::Curve(e, f, l);
                    if (curve.IsNull()) continue;
                    const Handle(Geom_Circle) circ = SketchGeometry::sketchCircleBasis(curve);

                    if (snap_.midpoint) {
                        addIfNear(tr("中点"), curve->Value((f + l) * 0.5), e, true);
                    }
                    if (snap_.arcMidpoint) {
                        if (!circ.IsNull()) {
                            addIfNear(tr("圆弧中点"), curve->Value((f + l) * 0.5), e, true);
                        }
                    }
                    if (!circ.IsNull()) {
                        const gp_Pnt c = circ->Location();
                        if (snap_.center) addIfNear(tr("圆心"), c, e, true);
                        if (snap_.quadrant) {
                            const gp_Ax2 ax = circ->Position();
                            const gp_Dir xd = ax.XDirection();
                            const gp_Dir yd = ax.YDirection();
                            const double r = circ->Radius();
                            addIfNear(tr("象限点"),
                                        gp_Pnt(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r), e, true);
                            addIfNear(tr("象限点"),
                                        gp_Pnt(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r), e, true);
                            addIfNear(tr("象限点"),
                                        gp_Pnt(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r), e, true);
                            addIfNear(tr("象限点"),
                                        gp_Pnt(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r), e, true);
                        }
                    }
                }

                if (snap_.intersection && edges.size() >= 2) {
                    const double tol = 1e-3;
                    for (int i = 0; i < edges.size(); ++i) {
                        for (int j = i + 1; j < edges.size(); ++j) {
                            gp_Pnt pi;
                            if (SketchGeometry::edgeIntersectionPoint(edges[i], edges[j], pi, tol)) {
                                addIfNear(tr("交点"), pi);
                            }
                        }
                    }
                }
            }
        }
    }

    if (candidates.isEmpty()) {
        clearSnapHover();
        return;
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.d2 < b.d2;
    });

    const Candidate best = candidates.front();
    const QString label = QString("%1\n(%2, %3, %4)")
                              .arg(best.label)
                              .arg(best.p.X(), 0, 'f', 2)
                              .arg(best.p.Y(), 0, 'f', 2)
                              .arg(best.p.Z(), 0, 'f', 2);

    // 清除旧悬停（点 + 关联形状）；best 点坐标在清除后再写入，避免 clearSnapHover 重置标志
    clearSnapHover();

    snapHoverBestPoint_ = best.p;
    hasSnapHoverBestPoint_ = true;

    // 1) 悬停点：琥珀高光小球（两点矢量拾取/拖拽时改由手柄球或半透明候选球显示）
    if (!options.suppressHoverBall) {
        vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
        configureMarkerSphereSource(sphere, 0.06);
        sphere->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());

        snapHoverPointActor_ = vtkSmartPointer<vtkActor>::New();
        snapHoverPointActor_->SetMapper(mapper);
        snapHoverPointActor_->SetPosition(best.p.X(), best.p.Y(), best.p.Z());
        applyMarkerSphereMaterial(snapHoverPointActor_->GetProperty(), MarkerSphereStyle::HoverYellow);
        {
            const double s = overlayWorldScaleAt(best.p.X(), best.p.Y(), best.p.Z());
            snapHoverPointActor_->SetScale(s, s, s);
        }
        addReferenceActor(snapHoverPointActor_);
    }

    // 2) 悬停文字
    if (!options.suppressHoverText) {
        vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
        textSource->SetText(label.toStdString().c_str());
        vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        textMapper->SetInputConnection(textSource->GetOutputPort());

        snapHoverTextActor_ = vtkSmartPointer<vtkFollower>::New();
        snapHoverTextActor_->SetMapper(textMapper);
        snapHoverTextActor_->SetCamera(renderer->GetActiveCamera());
        {
            const double s = overlayWorldScaleAt(best.p.X(), best.p.Y(), best.p.Z());
            snapHoverTextActor_->SetPosition(best.p.X() + 0.1 * s, best.p.Y() + 0.1 * s, best.p.Z() + 0.1 * s);
            snapHoverTextActor_->SetScale(0.05 * s);
        }
        snapHoverTextActor_->GetProperty()->SetColor(0.92, 0.72, 0.08);
        snapHoverTextActor_->GetProperty()->SetAmbient(0.55);
        snapHoverTextActor_->GetProperty()->SetDiffuse(0.45);
        addReferenceActor(snapHoverTextActor_);
    }

    // 3) 悬停关联形状高亮（中点/端点/圆心/象限点：高亮那条边/曲线）
    if (best.hasRef && !best.refShape.IsNull()) {
        snapHoverShapeActor_ = buildSnapShapeHighlightActor(best.refShape, 0.0, 0.9, 1.0, 0.8, 4.0);
        if (snapHoverShapeActor_) {
            addAppearanceActor(snapHoverShapeActor_);
        }
    }

    // 4) 拖拽：在高亮边上显示全部可吸附候选点（半透明球）
    if (options.showCandidateGhosts && best.hasRef && !best.refShape.IsNull()) {
        QSet<QString> drawnKeys;
        for (const Candidate& c : candidates) {
            if (!c.hasRef || c.refShape.IsNull() || !c.refShape.IsSame(best.refShape)) continue;

            const QString key = QString("%1,%2,%3")
                                    .arg(c.p.X(), 0, 'f', 4)
                                    .arg(c.p.Y(), 0, 'f', 4)
                                    .arg(c.p.Z(), 0, 'f', 4);
            if (drawnKeys.contains(key)) continue;
            drawnKeys.insert(key);

            vtkSmartPointer<vtkSphereSource> gSphere = vtkSmartPointer<vtkSphereSource>::New();
            configureMarkerSphereSource(gSphere, 0.055);
            gSphere->Update();

            vtkSmartPointer<vtkPolyDataMapper> gMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            gMapper->SetInputConnection(gSphere->GetOutputPort());

            vtkSmartPointer<vtkActor> ghost = vtkSmartPointer<vtkActor>::New();
            ghost->SetMapper(gMapper);
            ghost->SetPosition(c.p.X(), c.p.Y(), c.p.Z());
            ghost->SetPickable(0);
            applyMarkerSphereMaterial(ghost->GetProperty(), MarkerSphereStyle::HoverYellow);
            const bool isBest = c.p.Distance(best.p) < Precision::Confusion();
            ghost->GetProperty()->SetOpacity(isBest ? 0.72 : 0.38);
            {
                const double s = overlayWorldScaleAt(c.p.X(), c.p.Y(), c.p.Z());
                ghost->SetScale(s, s, s);
            }
            addReferenceActor(ghost);
            vectorTwoPointSnapGhostActors_.append(ghost);
        }
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::pickSnapAt(int x, int y, SnapPickContext ctx)
{
    if (!snap_.armed || !shapePicker || !renderer) return;

    // 捕捉范围：开启“捕捉最近点”时原本会扩大拾取容差。
    // 但“指定点/原点捕捉”流程为了稳定性，强制使用较小容差，避免 shapePicker 命中异常子形状导致崩溃。
    const double snapTolerance = (snap_.nearest && originSnapSelectionActive_) ? 0.05 : (snap_.nearest ? 0.25 : 0.05);

    // 拾取前准备：与悬停一致，确保可见模型可拾取且数据已更新
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor && historyList[i].shapeDataSource) {
            const bool visible = (historyList[i].actor->GetVisibility() != 0);
            historyList[i].actor->SetPickable(visible);
            if (visible) {
                rebindHistoryShapeSource(historyList[i]);
            } else {
                IVtkTools_ShapeObject::SetShapeSource(nullptr, historyList[i].actor);
                // 仅解除 ShapeSource 绑定即可避免隐藏遮挡；
                // 若从 picker 内部映射移除，恢复可见时可能出现高亮/子形状拾取异常
            }
        }
    }
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].shapeDataSource) {
            historyList[i].shapeDataSource->Modified();
            historyList[i].shapeDataSource->Update();
        }
    }

    // 候选点与 hover 逻辑一致，但在 nearest 模式下会弹出列表（需求 2）
    struct Candidate {
        QString label;
        gp_Pnt p;
        double d2 = 0.0;
        TopoDS_Shape refShape;
        bool hasRef = false;
    };
    QList<Candidate> candidates;

    // 复用 updateSnapHover 的同一套候选生成逻辑（简化：调用一次 edge/vertex pick）
    auto addCandidate = [&](const QString& t, const gp_Pnt& p, const TopoDS_Shape& refShape = TopoDS_Shape(), const bool hasRef = false) {
        candidates.append({t, p, snapScreenDist2(renderer, p, x, y), refShape, hasRef});
    };

    gp_Lin pickRay;
    const bool hasPickRay = (snap_.onCurve || snap_.onFace) && buildPickRayFromDisplay(renderer, x, y, pickRay);
    const double onGeomMaxD2 = kOnCurveFaceSnapScreenPixelRadius * kOnCurveFaceSnapScreenPixelRadius;

    // 最近点：扩大“有效范围”为屏幕像素半径，避免必须点中边/顶点才出候选
    // 需求：点击在长方体角附近也能列出周边可选点
    // “用于指定点(原点/中心点)”的最近点：跳过“大范围像素半径扫描”
    // 只依赖 shapePicker 的局部拾取，显著降低崩溃风险。
    if (snap_.nearest && !originSnapSelectionActive_) {
        const double pixelRadius = 110.0; // 最近点范围（屏幕像素半径）
        const double maxD2 = pixelRadius * pixelRadius;
        auto addCandidateIfNear = [&](const QString& t, const gp_Pnt& p, const TopoDS_Shape& refShape = TopoDS_Shape(), const bool hasRef = false) {
            const double d2 = snapScreenDist2(renderer, p, x, y);
            if (d2 <= maxD2) {
                candidates.append({t, p, d2, refShape, hasRef});
            }
        };

        int edgeCount = 0;
        const int maxEdgesToScan = 800; // 保护性能（一般远小于该值）

        for (const auto& rec : historyList) {
            if (!rec.actor || rec.actor->GetVisibility() == 0) continue;
            if (rec.type == DATUM_PLANE || rec.type == DATUM_AXIS || rec.type == WORK_CSYS || rec.type == REFERENCE_CSYS) continue;
            if (rec.occShape.IsNull()) continue;

            // 端点：直接扫描顶点（比从边取端点更直接）
            if (snap_.endpoint) {
                for (TopExp_Explorer exV(rec.occShape, TopAbs_VERTEX); exV.More(); exV.Next()) {
                    const TopoDS_Vertex v = TopoDS::Vertex(exV.Current());
                    addCandidateIfNear(tr("端点"), BRep_Tool::Pnt(v));
                }
            }

            // 其它类型需要扫描边
            if (snap_.midpoint || snap_.arcMidpoint || snap_.center || snap_.quadrant || snap_.intersection) {
                QList<TopoDS_Edge> edges;
                for (TopExp_Explorer exE(rec.occShape, TopAbs_EDGE); exE.More(); exE.Next()) {
                    if (edgeCount++ > maxEdgesToScan) break;
                    edges.append(TopoDS::Edge(exE.Current()));
                }

                for (const TopoDS_Edge& e : edges) {
                    Standard_Real f = 0.0, l = 0.0;
                    Handle(Geom_Curve) curve = BRep_Tool::Curve(e, f, l);
                    if (curve.IsNull()) continue;
                    const Handle(Geom_Circle) circ = SketchGeometry::sketchCircleBasis(curve);

                    if (snap_.midpoint) {
                        addCandidateIfNear(tr("中点"), curve->Value((f + l) * 0.5), e, true);
                    }

                    if (snap_.arcMidpoint) {
                        if (!circ.IsNull()) {
                            addCandidateIfNear(tr("圆弧中点"), curve->Value((f + l) * 0.5), e, true);
                        }
                    }

                    if (!circ.IsNull()) {
                        const gp_Pnt c = circ->Location();
                        if (snap_.center) addCandidateIfNear(tr("圆心"), c, e, true);
                        if (snap_.quadrant) {
                            const gp_Ax2 ax = circ->Position();
                            const gp_Dir xd = ax.XDirection();
                            const gp_Dir yd = ax.YDirection();
                            const double r = circ->Radius();
                            addCandidateIfNear(tr("象限点"), gp_Pnt(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r), e, true);
                            addCandidateIfNear(tr("象限点"), gp_Pnt(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r), e, true);
                            addCandidateIfNear(tr("象限点"), gp_Pnt(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r), e, true);
                            addCandidateIfNear(tr("象限点"), gp_Pnt(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r), e, true);
                        }
                    }
                }

                if (snap_.intersection && edges.size() >= 2) {
                    const double tol = 1e-3;
                    for (int i = 0; i < edges.size(); ++i) {
                        for (int j = i + 1; j < edges.size(); ++j) {
                            gp_Pnt pi;
                            if (SketchGeometry::edgeIntersectionPoint(edges[i], edges[j], pi, tol)) {
                                addCandidateIfNear(tr("交点"), pi);
                            }
                        }
                    }
                }
            }
        }

        {
            QList<SketchSnapScreenCandidate> sketchCand;
            appendActiveSketchSnapScreenCandidates(x, y, maxD2, sketchCand);
            for (const auto& sc : sketchCand) {
                candidates.append({sc.label, sc.p, sc.d2, sc.refShape, sc.hasRef});
            }
        }

        // 若已经在大范围内找到候选点，则直接进入排序与弹窗，不再依赖 shapePicker 的命中
        if (!candidates.isEmpty()) {
            std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
                return a.d2 < b.d2;
            });
            // 继续走下面的“chosen/弹窗”逻辑
            goto SNAP_PICK_CHOSEN;
        }
    }

    // originSnapSelectionActive_：为“指定点(原点/中心点)-最近点”准备轻量候选。
    // 只扫端点/中点并按 d2 排序，避免大范围扫描导致的不稳定。
    if (snap_.nearest && originSnapSelectionActive_ && candidates.isEmpty()) {
        const int maxEdgesToScan = 300; // 性能保护
        int edgeCount = 0;

        for (const auto& rec : historyList) {
            if (!rec.actor || rec.actor->GetVisibility() == 0) continue;
            if (rec.type == DATUM_PLANE || rec.type == DATUM_AXIS || rec.type == WORK_CSYS || rec.type == REFERENCE_CSYS) continue;
            if (rec.occShape.IsNull()) continue;

            // 端点
            if (snap_.endpoint) {
                for (TopExp_Explorer exV(rec.occShape, TopAbs_VERTEX); exV.More(); exV.Next()) {
                    const TopoDS_Vertex v = TopoDS::Vertex(exV.Current());
                    const gp_Pnt p = BRep_Tool::Pnt(v);
                    candidates.append({tr("端点"), p, snapScreenDist2(renderer, p, x, y), TopoDS_Shape(), false});
                }
            }

            // 中点/圆弧中点（扫描部分边）
            if (snap_.midpoint) {
                for (TopExp_Explorer exE(rec.occShape, TopAbs_EDGE); exE.More(); exE.Next()) {
                    if (edgeCount++ > maxEdgesToScan) break;
                    const TopoDS_Edge e = TopoDS::Edge(exE.Current());
                    const QList<gp_Pnt> snapPts = SketchGeometry::edgeSnapCandidates(e, true, snap_.center, snap_.quadrant);
                    for (int i = 0; i < snapPts.size(); ++i) {
                        const gp_Pnt& p = snapPts[i];
                        QString label = tr("中点");
                        if (snap_.center && i == 1) {
                            label = tr("圆心");
                        } else if ((snap_.center && i >= 2) || (!snap_.center && i >= 1)) {
                            label = tr("象限点");
                        }
                        candidates.append({label, p, snapScreenDist2(renderer, p, x, y), e, true});
                    }
                }
            }

            // 圆弧中点：仅对圆曲线
            if (snap_.arcMidpoint) {
                for (TopExp_Explorer exE(rec.occShape, TopAbs_EDGE); exE.More(); exE.Next()) {
                    if (edgeCount++ > maxEdgesToScan) break;
                    const TopoDS_Edge e = TopoDS::Edge(exE.Current());
                    gp_Pnt mid;
                    if (!SketchGeometry::circularEdgeMidPoint(e, mid)) continue;
                    candidates.append({tr("圆弧中点"), mid, snapScreenDist2(renderer, mid, x, y), e, true});
                }
            }
        }

        {
            const double sketchMaxD2 = kSketchSnapScreenPixelRadius * kSketchSnapScreenPixelRadius;
            QList<SketchSnapScreenCandidate> sketchCand;
            appendActiveSketchSnapScreenCandidates(x, y, sketchMaxD2, sketchCand);
            for (const auto& sc : sketchCand) {
                candidates.append({sc.label, sc.p, sc.d2, sc.refShape, sc.hasRef});
            }
        }

        if (!candidates.isEmpty()) {
            std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
                return a.d2 < b.d2;
            });
            const int maxCandidates = 60;
            if (candidates.size() > maxCandidates) {
                candidates = candidates.mid(0, maxCandidates);
            }
            goto SNAP_PICK_CHOSEN;
        }
    }

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(snapTolerance);

    // 顶点（端点）
    if (snap_.endpoint) {
        shapePicker->SetSelectionMode(SM_Vertex);
        shapePicker->Pick(x, y, 0);
        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
            pickedActors->InitTraversal();
            if (vtkActor* actor = pickedActors->GetNextActor()) {
                vtkSmartPointer<IVtkTools_ShapeDataSource> ds = IVtkTools_ShapeObject::GetShapeSource(actor);
                if (ds) {
                    Handle(IVtkOCC_Shape) sw = ds->GetShape();
                    if (!sw.IsNull()) {
                        IVtk_IdType shapeID = sw->GetId();
                        IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
                        for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
                            const TopoDS_Shape& sh = sw->GetSubShape(it.Value());
                            if (sh.ShapeType() != TopAbs_VERTEX) continue;
                            addCandidate(tr("端点"), BRep_Tool::Pnt(TopoDS::Vertex(sh)));
                        }
                    }
                }
            }
        }
    }

    // 边（端点/中点/圆心/象限/交点）
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);
    {
        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
            pickedActors->InitTraversal();
            if (vtkActor* actor = pickedActors->GetNextActor()) {
                vtkSmartPointer<IVtkTools_ShapeDataSource> ds = IVtkTools_ShapeObject::GetShapeSource(actor);
                if (ds) {
                    Handle(IVtkOCC_Shape) sw = ds->GetShape();
                    if (!sw.IsNull()) {
                        IVtk_IdType shapeID = sw->GetId();
                        IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
                        QList<TopoDS_Edge> edges;
                        for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
                            const TopoDS_Shape& sh = sw->GetSubShape(it.Value());
                            if (sh.ShapeType() == TopAbs_EDGE) edges.append(TopoDS::Edge(sh));
                        }

                        for (const TopoDS_Edge& e : edges) {
                            Standard_Real f = 0.0, l = 0.0;
                            Handle(Geom_Curve) curve = BRep_Tool::Curve(e, f, l);
                            if (curve.IsNull()) continue;
                            const Handle(Geom_Circle) circ = SketchGeometry::sketchCircleBasis(curve);

                            if (snap_.endpoint) {
                                addCandidate(tr("端点"), curve->Value(f), e, true);
                                addCandidate(tr("端点"), curve->Value(l), e, true);
                            }
                            if (snap_.midpoint) {
                                addCandidate(tr("中点"), curve->Value((f + l) * 0.5), e, true);
                            }

                            if (snap_.onCurve && hasPickRay) {
                                gp_Pnt pOn;
                                if (snapProjectRayOntoEdge(e, pickRay, pOn)) {
                                    const double d2 = snapScreenDist2(renderer, pOn, x, y);
                                    if (d2 <= onGeomMaxD2) {
                                        candidates.append({tr("点在曲线上"), pOn, d2, e, true});
                                    }
                                }
                            }

                            if (snap_.arcMidpoint) {
                                if (!circ.IsNull()) {
                                    addCandidate(tr("圆弧中点"), curve->Value((f + l) * 0.5), e, true);
                                }
                            }

                            if (!circ.IsNull()) {
                                const gp_Pnt c = circ->Location();
                                if (snap_.center) addCandidate(tr("圆心"), c, e, true);
                                if (snap_.quadrant) {
                                    const gp_Ax2 ax = circ->Position();
                                    const gp_Dir xd = ax.XDirection();
                                    const gp_Dir yd = ax.YDirection();
                                    const double r = circ->Radius();
                                    addCandidate(tr("象限点"), gp_Pnt(c.X() + xd.X() * r, c.Y() + xd.Y() * r, c.Z() + xd.Z() * r), e, true);
                                    addCandidate(tr("象限点"), gp_Pnt(c.X() - xd.X() * r, c.Y() - xd.Y() * r, c.Z() - xd.Z() * r), e, true);
                                    addCandidate(tr("象限点"), gp_Pnt(c.X() + yd.X() * r, c.Y() + yd.Y() * r, c.Z() + yd.Z() * r), e, true);
                                    addCandidate(tr("象限点"), gp_Pnt(c.X() - yd.X() * r, c.Y() - yd.Y() * r, c.Z() - yd.Z() * r), e, true);
                                }
                            }
                        }

                                if (snap_.intersection && edges.size() >= 2) {
                                    const double tol = 1e-3;
                                    for (int i = 0; i < edges.size(); ++i) {
                                        for (int j = i + 1; j < edges.size(); ++j) {
                                            gp_Pnt pi;
                                            if (SketchGeometry::edgeIntersectionPoint(edges[i], edges[j], pi, tol)) {
                                                addCandidate(tr("交点"), pi);
                                            }
                                        }
                                    }
                                }
                    }
                }
            }
        }
    }

    // 面（面上的点）：射线与面求交
    if (snap_.onFace && hasPickRay) {
        shapePicker->SetSelectionMode(SM_Face);
        shapePicker->Pick(x, y, 0);
        vtkSmartPointer<vtkActorCollection> pickedActors = shapePicker->GetPickedActors(true);
        if (pickedActors && pickedActors->GetNumberOfItems() > 0) {
            pickedActors->InitTraversal();
            if (vtkActor* actor = pickedActors->GetNextActor()) {
                vtkSmartPointer<IVtkTools_ShapeDataSource> ds = IVtkTools_ShapeObject::GetShapeSource(actor);
                if (ds) {
                    Handle(IVtkOCC_Shape) sw = ds->GetShape();
                    if (!sw.IsNull()) {
                        IVtk_IdType shapeID = sw->GetId();
                        IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
                        for (IVtk_ShapeIdList::Iterator it(subIds); it.More(); it.Next()) {
                            const TopoDS_Shape& sh = sw->GetSubShape(it.Value());
                            if (sh.ShapeType() != TopAbs_FACE) continue;
                            const TopoDS_Face face = TopoDS::Face(sh);
                            gp_Pnt pOn;
                            if (!snapProjectRayOntoFace(face, pickRay, pOn)) continue;
                            const double d2 = snapScreenDist2(renderer, pOn, x, y);
                            if (d2 <= onGeomMaxD2) {
                                candidates.append({tr("面上的点"), pOn, d2, face, true});
                            }
                        }
                    }
                }
            }
        }
    }

    {
        const double sketchMaxD2 = kSketchSnapScreenPixelRadius * kSketchSnapScreenPixelRadius;
        QList<SketchSnapScreenCandidate> sketchCand;
        appendActiveSketchSnapScreenCandidates(x, y, sketchMaxD2, sketchCand);
        for (const auto& sc : sketchCand) {
            candidates.append({sc.label, sc.p, sc.d2, sc.refShape, sc.hasRef});
        }
    }

    if (candidates.isEmpty()) {
        if (ctx == SnapPickContext::SketchTool) {
            hasSnapSelectedPoint_ = false;
            return;
        }
        statusBar()->showMessage(tr("未捕捉到可用点。"), 1500);
        return;
    }

SNAP_PICK_CHOSEN:
    // 去重：同一空间点可能由不同来源重复加入（例如：顶点扫描 + 边端点扫描）
    // 以坐标量化作为 key，并用“类型优先级 + 距离”选择保留项
    auto coordKey = [&](const gp_Pnt& p) -> QString {
        // 0.001 精度足够抑制重复，又不会误合并相邻点
        return QString("%1,%2,%3")
            .arg(p.X(), 0, 'f', 3)
            .arg(p.Y(), 0, 'f', 3)
            .arg(p.Z(), 0, 'f', 3);
    };
    auto labelPriority = [&](const QString& label) -> int {
        // 数字越小优先级越高
        if (label == tr("交点")) return 0;
        if (label == tr("端点")) return 1;
        if (label == tr("中点")) return 2;
        if (label == tr("圆弧中点")) return 2;
        if (label == tr("圆心")) return 3;
        if (label == tr("象限点")) return 4;
        if (label == tr("点在曲线上")) return 5;
        if (label == tr("面上的点")) return 6;
        return 10;
    };
    {
        QHash<QString, int> bestIndexByKey;
        QList<Candidate> unique;
        unique.reserve(candidates.size());

        for (const auto& c : candidates) {
            const QString key = coordKey(c.p);
            if (!bestIndexByKey.contains(key)) {
                bestIndexByKey.insert(key, unique.size());
                unique.append(c);
                continue;
            }
            const int idx = bestIndexByKey.value(key);
            Candidate& best = unique[idx];
            const int pBest = labelPriority(best.label);
            const int pCur  = labelPriority(c.label);
            if (pCur < pBest || (pCur == pBest && c.d2 < best.d2)) {
                best = c;
            }
        }
        candidates = unique;
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.d2 < b.d2;
    });

    gp_Pnt chosen = candidates.front().p;
    QString chosenLabel = candidates.front().label;
    TopoDS_Shape chosenRefShape = candidates.front().refShape;
    bool chosenHasRef = candidates.front().hasRef;

    // 最近点：弹出快速选取列表（需求 2）；草图工具内点击捕捉时跳过模态列表
    if (snap_.nearest && candidates.size() >= 1 && ctx == SnapPickContext::Normal) {
        // 关键修复：弹出 Qt 模态对话框前，强制结束一次 VTK 左键交互
        // 否则 VTK 可能卡在 Rotate 状态，导致关闭对话框后“鼠标一动就旋转”
        if (vtkWidget && vtkWidget->renderWindow() && vtkWidget->renderWindow()->GetInteractor()) {
            vtkRenderWindowInteractor* iren = vtkWidget->renderWindow()->GetInteractor();
            if (iren->GetInteractorStyle()) {
                if (vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(iren->GetInteractorStyle())) {
                    // VTK 9.4 的 vtkInteractorStyle 没有公开 SetState()；使用 TrackballCamera 的 End* 来清理交互状态
                    if (vtkInteractorStyleTrackballCamera* tbc = vtkInteractorStyleTrackballCamera::SafeDownCast(style)) {
                        tbc->EndRotate();
                        tbc->EndPan();
                        tbc->EndDolly();
                        tbc->EndSpin();
                    }
                    style->OnLeftButtonUp();
                }
            }
        }
        resetVtkMouseDragTracking();

        QStringList items;
        const int maxItems = std::min<int>(20, static_cast<int>(candidates.size()));
        items.reserve(maxItems);
        for (int i = 0; i < maxItems; ++i) {
            const auto& c = candidates[i];
            items << QString("%1  (%2, %3, %4)")
                         .arg(c.label)
                         .arg(c.p.X(), 0, 'f', 3)
                         .arg(c.p.Y(), 0, 'f', 3)
                         .arg(c.p.Z(), 0, 'f', 3);
        }

        bool ok = false;
        const QString picked = QInputDialog::getItem(
            this,
            tr("快速选取"),
            tr("附近可供选择的点："),
            items,
            0,
            false,
            &ok
        );

        // 关闭对话框后：再次确保交互状态干净
        if (vtkWidget && vtkWidget->renderWindow() && vtkWidget->renderWindow()->GetInteractor()) {
            vtkRenderWindowInteractor* iren = vtkWidget->renderWindow()->GetInteractor();
            if (iren->GetInteractorStyle()) {
                if (vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(iren->GetInteractorStyle())) {
                    if (vtkInteractorStyleTrackballCamera* tbc = vtkInteractorStyleTrackballCamera::SafeDownCast(style)) {
                        tbc->EndRotate();
                        tbc->EndPan();
                        tbc->EndDolly();
                        tbc->EndSpin();
                    }
                    style->OnLeftButtonUp();
                }
            }
        }
        resetVtkMouseDragTracking();

        if (!ok) {
            return;
        }

        const int idx = items.indexOf(picked);
        if (idx >= 0 && idx < maxItems) {
            chosen = candidates[idx].p;
            chosenLabel = candidates[idx].label;
            chosenRefShape = candidates[idx].refShape;
            chosenHasRef = candidates[idx].hasRef;
        }
    }

    snapSelectedPoint_ = chosen;
    hasSnapSelectedPoint_ = true;

    if (ctx == SnapPickContext::Normal)
        addSnapPersistentPoint(chosen);

    // 捕捉完成后：清除悬浮提示/棱高亮，只保留常驻点高亮
    clearSnapHover();

    if (ctx == SnapPickContext::Normal)
        statusBar()->showMessage(tr("捕捉成功：%1").arg(chosenLabel), 2000);

    // 如果当前是“用 snap 选原点”流程，则捕捉完成后立刻写回对话框并退出捕捉
    if (originSnapSelectionActive_) {
        applyOriginFromSnap(chosen, chosenLabel);
        return;
    }
}

void Widget::showSnapPointBall(const gp_Pnt& p)
{
    // 已切换为捕捉专用显示（保留接口兼容）
    Q_UNUSED(p);
}
