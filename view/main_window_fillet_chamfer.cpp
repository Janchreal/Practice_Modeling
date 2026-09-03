// 倒圆角 / 倒角 / 抽壳（从 main_window.cpp 拆出）
#include "main_window.h"
#include "ui_main_window.h"
#include "feature_topology.h"
#include "feature_modification_geometry.h"
#include "addmodelcommand.h"
#include "fillet_dialog.h"
#include "chamfer_dialog.h"
#include "handle_geometry.h"

#include <algorithm>
#include <cmath>

#include <QDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <Qt>

#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp_Face.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS_Face.hxx>

#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_ShapePicker.hxx>
#include <IVtk_Types.hxx>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCellArray.h>
#include <vtkMapper.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkVersionMacros.h>

namespace {

void styleFilletChamferEdgeOverlay(vtkActor* actor)
{
    if (!actor) return;
    vtkProperty* p = actor->GetProperty();
    p->SetLighting(false);
    p->SetAmbient(1.0);
    p->SetDiffuse(0.0);
    p->SetSpecular(0.0);
    p->SetOpacity(1.0);
    p->SetRepresentationToWireframe();
    p->RenderLinesAsTubesOff();
#if VTK_MAJOR_VERSION >= 9
    actor->ForceOpaqueOn();
#endif
    if (vtkMapper* mapper = actor->GetMapper()) {
        mapper->SetResolveCoincidentTopologyToPolygonOffset();
        // 略向前推，盖住幽灵橙轮廓，避免共面闪烁斑点
        mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-8.0, -8.0);
    }
}

// 按边曲线采样成折线，避免 IVtk 三角网线框造成“很宽+黑斑”
vtkSmartPointer<vtkActor> buildCleanEdgeHighlightActor(const TopoDS_Edge& edge,
                                                       double r, double g, double b,
                                                       double lineWidth)
{
    if (edge.IsNull()) return nullptr;
    try {
        BRepAdaptor_Curve curve(edge);
        const double u0 = curve.FirstParameter();
        const double u1 = curve.LastParameter();
        if (!(u1 > u0)) return nullptr;

        const int samples = std::clamp(static_cast<int>(std::ceil((u1 - u0) * 32.0)), 16, 128);
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        pts->SetNumberOfPoints(samples + 1);
        for (int i = 0; i <= samples; ++i) {
            const double t = u0 + (u1 - u0) * (static_cast<double>(i) / samples);
            const gp_Pnt p = curve.Value(t);
            pts->SetPoint(i, p.X(), p.Y(), p.Z());
        }

        vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
        lines->InsertNextCell(samples + 1);
        for (int i = 0; i <= samples; ++i) {
            lines->InsertCellPoint(i);
        }

        vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
        pd->SetPoints(pts);
        pd->SetLines(lines);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(pd);
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(r, g, b);
        actor->GetProperty()->SetLineWidth(lineWidth);
        actor->SetPickable(false);
        styleFilletChamferEdgeOverlay(actor);
        return actor;
    } catch (...) {
        return nullptr;
    }
}

void addEdgeHighlightActor(vtkRenderer* target,
                           vtkSmartPointer<vtkActor> actor,
                           QList<vtkSmartPointer<vtkActor>>* outActors,
                           vtkSmartPointer<vtkActor>* hoverOut)
{
    if (!target || !actor) return;
    target->AddActor(actor);
    if (outActors) outActors->append(actor);
    if (hoverOut) *hoverOut = actor;
}

} // namespace

// 倒角按钮点击事件
void Widget::on_fillet_clicked()
{
    // 清理上一次的倒圆角选择/高亮
    clearFilletEdgeHighlight();
    filletSelectedEdges_.clear();
    filletSelectedEdgeSubIds_.clear();
    filletTargetModelIndex = -1; // 由第一次选边时确定目标模型

    filletdialog* dialog = new filletdialog(dialogParentWidget());
    dialog->setModal(false);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    filletDialog = dialog;

    auto enterFilletEdgeSelection = [this]() {
        currentSelectionMode = FilletEdgeSelection;
        setFeatureGizmoActorsPickable(false);
        // 与拉伸/旋转一致：进入选边即全体幽灵，便于透过实体选边
        setFeatureOperationGhostMode(true);
        if (statusBar()) {
            statusBar()->showMessage(tr("倒圆角：请在视图中点击选择要倒圆的边（可多选）"), 5000);
        }
        if (vtkWidget) vtkWidget->setFocus();
    };

    // 点击倒圆角按钮后自动进入边选择模式
    enterFilletEdgeSelection();

    // “已选 N 条”按钮：仅提示当前处于选边模式（不改变选择）
    connect(dialog, &filletdialog::edgeModeHintRequested, this, [this]() {
        if (statusBar()) {
            statusBar()->showMessage(tr("当前为倒圆角-选择边模式"), 2000);
        }
    });
    connect(dialog, &filletdialog::parametersChanged, this, [this]() {
        refreshFilletLivePreview();
        updateFilletRadiusHandles();
    });

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        if (!dialog->continuityText().startsWith("G2") && dialog->shapeText() != "圆") {
            if (statusBar()) statusBar()->showMessage(tr("当前仅支持圆形截面"), 3000);
            return;
        }

        if (filletSelectedEdges_.isEmpty()) {
            if (statusBar()) {
                statusBar()->showMessage(tr("未选择任何边"), 2000);
            }
            return;
        }

        if (filletTargetModelIndex < 0 || filletTargetModelIndex >= historyList.size()) {
            return;
        }

        const double r = dialog->radiusValue();
        if (r <= 0.0) {
            if (statusBar()) {
                statusBar()->showMessage(tr("半径必须大于 0"), 2000);
            }
            return;
        }

        try {
            TopoDS_Shape originalShape = historyList[filletTargetModelIndex].occShape;
            if (originalShape.IsNull()) {
                return;
            }

            // 圆角仅对实体更稳定；如果不是实体，直接提示
            {
                TopExp_Explorer ex(originalShape, TopAbs_SOLID);
                if (!ex.More()) {
                    QMessageBox::warning(this, "错误", "当前仅支持对实体(Solid)执行倒圆角。");
                    return;
                }
            }

            const bool isG2 = dialog->continuityText().startsWith("G2");
            const double rho = dialog->rhoValue();
            TopoDS_Shape resultShape;
            double usedR = r;
            if (!FeatureModificationGeometry::buildFilletShape(
                    originalShape, filletSelectedEdges_, r, isG2, rho, resultShape, &usedR)) {
                const QString msg = isG2
                    ? QString("G2 曲率圆角失败（严格模式，不会降级到 G1）。\n可尝试减小半径或调整 Rho（当前=%1）。").arg(rho, 0, 'f', 3)
                    : QString("倒圆角操作失败（可能半径过大或几何不兼容）。");
                QMessageBox::warning(this, "错误", msg);
                return;
            }
            const QString originalName = historyList[filletTargetModelIndex].name;
            const QString modeTag = isG2 ? "G2" : "G1";
            const QString resultName = QString("倒圆角(%1,r=%2)_%3").arg(modeTag).arg(usedR).arg(originalName);

            FeatureRecipe recipe;
            recipe.hasRecipe = true;
            recipe.parentIndices = { filletTargetModelIndex };
            recipe.fillet.targetIndex = filletTargetModelIndex;
            recipe.fillet.radius = r;
            recipe.fillet.usedRadius = usedR;
            recipe.fillet.isG2 = isG2;
            recipe.fillet.rho = rho;
            const TopoDS_Shape& parentShape = historyList[filletTargetModelIndex].occShape;
            for (const TopoDS_Edge& edge : filletSelectedEdges_) {
                recipe.fillet.edges.append(
                    makeSubShapeRef(filletTargetModelIndex, parentShape, edge, TopAbs_EDGE));
            }

            executeCommand(new AddModelCommand(this, resultShape, resultName, FILLET, QColor(255, 140, 0), usedR,
                                               0.0, 0.0, QList<int>{ filletTargetModelIndex }, true, &recipe));
        } catch (Standard_Failure& e) {
            QMessageBox::critical(this, "错误", QString("倒圆角操作异常: %1").arg(e.GetMessageString()));
        }
    });

    auto cleanupFilletState = [this]() {
        clearFilletRadiusHandles();
        clearFilletEdgeHighlight();
        clearFeatureLivePreview();
        setFeatureOperationGhostMode(false);
        setFeatureGizmoActorsPickable(true);
        filletSelectedEdgeSubIds_.clear();
        filletSelectedEdges_.clear();
        filletTargetModelIndex = -1;
        currentSelectionMode = None;
    };

    connect(dialog, &QDialog::rejected, this, [cleanupFilletState]() {
        cleanupFilletState();
    });

    connect(dialog, &QDialog::finished, this, [this, cleanupFilletState](int) {
        cleanupFilletState();
        filletDialog = nullptr;
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

bool Widget::buildFilletPreviewShape(TopoDS_Shape& outShape) const
{
    outShape = TopoDS_Shape();
    if (!filletDialog || filletSelectedEdges_.isEmpty()) return false;
    if (filletTargetModelIndex < 0 || filletTargetModelIndex >= historyList.size()) return false;

    const TopoDS_Shape originalShape = historyList[filletTargetModelIndex].occShape;
    if (originalShape.IsNull()) return false;
    TopExp_Explorer solidEx(originalShape, TopAbs_SOLID);
    if (!solidEx.More()) return false;

    const double r = filletDialog->radiusValue();
    if (r <= 1e-9) return false;

    try {
        const bool isG2 = filletDialog->continuityText().startsWith(QStringLiteral("G2"));
        const double rho = filletDialog->rhoValue();
        double usedRadius = r;
        if (!FeatureModificationGeometry::buildFilletShape(
                originalShape, filletSelectedEdges_, r, isG2, rho, outShape, &usedRadius)) {
            return false;
        }
        return !outShape.IsNull();
    } catch (Standard_Failure&) {
        return false;
    } catch (...) {
        return false;
    }
}

bool Widget::buildChamferPreviewShape(TopoDS_Shape& outShape) const
{
    outShape = TopoDS_Shape();
    if (!chamferDialog || chamferSelectedEdges_.isEmpty()) return false;
    if (chamferTargetModelIndex_ < 0 || chamferTargetModelIndex_ >= historyList.size()) return false;

    const QString section = chamferDialog->sectionText();
    if (section != QStringLiteral("对称") && section != QStringLiteral("非对称")) return false;

    const TopoDS_Shape originalShape = historyList[chamferTargetModelIndex_].occShape;
    if (originalShape.IsNull()) return false;

    const bool twoDistances = (section == QStringLiteral("非对称"));
    const double d1 = twoDistances ? chamferDialog->distance1Value() : chamferDialog->distanceValue();
    const double d2 = twoDistances ? chamferDialog->distance2Value() : d1;
    if (d1 <= 1e-9 || d2 <= 1e-9) return false;

    try {
        if (!FeatureModificationGeometry::buildChamferShape(
                originalShape, chamferSelectedEdges_, d1, d2, twoDistances, outShape)) {
            return false;
        }
        return !outShape.IsNull();
    } catch (Standard_Failure&) {
        return false;
    } catch (...) {
        return false;
    }
}

void Widget::refreshFilletLivePreview()
{
    if (!filletDialog) {
        clearFeatureLivePreview();
        return;
    }
    TopoDS_Shape shape;
    if (!buildFilletPreviewShape(shape) || shape.IsNull()) {
        clearFeatureLivePreview();
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }
    showFeatureLivePreviewShape(shape);
}

void Widget::refreshChamferLivePreview()
{
    if (!chamferDialog) {
        clearFeatureLivePreview();
        return;
    }
    TopoDS_Shape shape;
    if (!buildChamferPreviewShape(shape) || shape.IsNull()) {
        clearFeatureLivePreview();
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }
    showFeatureLivePreviewShape(shape);
}

void Widget::syncFilletChamferGhostAndPreview(bool isFillet)
{
    const bool dialogOpen = isFillet ? (filletDialog != nullptr) : (chamferDialog != nullptr);
    if (!dialogOpen) {
        clearFeatureLivePreview();
        setFeatureOperationGhostMode(false);
        return;
    }

    // 对话框打开期间保持幽灵（与拉伸/旋转选截面阶段一致），不要因暂无选边而退出
    if (!featureOperationGhostMode_) {
        setFeatureOperationGhostMode(true);
    }

    const bool hasEdges = isFillet ? !filletSelectedEdges_.isEmpty()
                                   : !chamferSelectedEdges_.isEmpty();
    if (hasEdges) {
        if (isFillet) refreshFilletLivePreview();
        else refreshChamferLivePreview();
    } else {
        clearFeatureLivePreview();
    }
}

void Widget::clearFilletEdgeHighlight()
{
    for (const auto& a : filletEdgeHighlightActors_) {
        if (!a) continue;
        removeSceneActor(a);
    }
    filletEdgeHighlightActors_.clear();

    if (filletHoverEdgeActor_) {
        removeSceneActor(filletHoverEdgeActor_);
        filletHoverEdgeActor_ = nullptr;
    }
    hasFilletHoverEdge_ = false;
    filletHoverEdgeSubId_ = -1;
    filletHoverModelIndex_ = -1;
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::clearChamferEdgeHighlight()
{
    for (const auto& a : chamferEdgeHighlightActors_) {
        if (!a) continue;
        removeSceneActor(a);
    }
    chamferEdgeHighlightActors_.clear();

    if (chamferHoverEdgeActor_) {
        removeSceneActor(chamferHoverEdgeActor_);
        chamferHoverEdgeActor_ = nullptr;
    }
    hasChamferHoverEdge_ = false;
    chamferHoverEdgeSubId_ = -1;
    chamferHoverModelIndex_ = -1;
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::updateChamferEdgeHighlight()
{
    clearChamferEdgeHighlight();
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay ? overlay : renderer.GetPointer();
    if (!target) return;
    for (const TopoDS_Edge& e : chamferSelectedEdges_) {
        addEdgeHighlightActor(target,
                              buildCleanEdgeHighlightActor(e, 1.0, 0.15, 0.08, 4.5),
                              &chamferEdgeHighlightActors_,
                              nullptr);
    }
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::handleChamferEdgeClick(int x, int y)
{
    if (!shapePicker || !renderer) return;

    // 优先使用悬浮高亮的边
    if (hasChamferHoverEdge_ && !chamferHoverEdge_.IsNull() && chamferHoverEdgeSubId_ >= 0 && chamferHoverModelIndex_ >= 0) {
        if (chamferTargetModelIndex_ < 0) chamferTargetModelIndex_ = chamferHoverModelIndex_;
        if (chamferTargetModelIndex_ == chamferHoverModelIndex_) {
            const IVtk_IdType subId = chamferHoverEdgeSubId_;
            const TopoDS_Edge edge = chamferHoverEdge_;

            int existing = -1;
            for (int i = 0; i < chamferSelectedEdgeSubIds_.size(); ++i) {
                if (chamferSelectedEdgeSubIds_[i] == subId) { existing = i; break; }
            }
            if (existing >= 0) {
                chamferSelectedEdgeSubIds_.removeAt(existing);
                chamferSelectedEdges_.removeAt(existing);
            } else {
                chamferSelectedEdgeSubIds_.append(subId);
                chamferSelectedEdges_.append(edge);
            }

            updateChamferEdgeHighlight();
            if (chamferDialog) chamferDialog->setSelectedEdgeCount(chamferSelectedEdges_.size());
            syncFilletChamferGhostAndPreview(false);
            updateChamferAsymHandles();
            return;
        }
    }

    // 兜底：即时拾取
    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);

    vtkSmartPointer<vtkActorCollection> picked = shapePicker->GetPickedActors(true);
    if (!picked || picked->GetNumberOfItems() == 0) return;

    vtkActor* pickedActor = nullptr;
    picked->InitTraversal();
    while (vtkActor* a = picked->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) { pickedActor = a; break; }
    }
    if (!pickedActor) return;

    int pickedIndex = resolveHistoryIndexByActor(pickedActor);
    if (pickedIndex < 0) return;
    if (chamferTargetModelIndex_ >= 0 && chamferTargetModelIndex_ != pickedIndex) return;
    if (chamferTargetModelIndex_ < 0) chamferTargetModelIndex_ = pickedIndex;

    IVtkTools_ShapeDataSource* ds = IVtkTools_ShapeObject::GetShapeSource(pickedActor);
    if (!ds) return;
    Handle(IVtkOCC_Shape) wrapper = ds->GetShape();
    if (wrapper.IsNull()) return;
    const IVtk_IdType shapeID = wrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subIds.IsEmpty()) return;
    const IVtk_IdType subId = subIds.First();
    const TopoDS_Shape sub = wrapper->GetSubShape(subId);
    if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) return;
    const TopoDS_Edge edge = TopoDS::Edge(sub);

    int existing = -1;
    for (int i = 0; i < chamferSelectedEdgeSubIds_.size(); ++i) {
        if (chamferSelectedEdgeSubIds_[i] == subId) { existing = i; break; }
    }
    if (existing >= 0) {
        chamferSelectedEdgeSubIds_.removeAt(existing);
        chamferSelectedEdges_.removeAt(existing);
    } else {
        chamferSelectedEdgeSubIds_.append(subId);
        chamferSelectedEdges_.append(edge);
    }
    updateChamferEdgeHighlight();
    if (chamferDialog) chamferDialog->setSelectedEdgeCount(chamferSelectedEdges_.size());
    syncFilletChamferGhostAndPreview(false);
    updateChamferAsymHandles();
}

void Widget::handleChamferEdgeHover(int x, int y)
{
    if (!shapePicker || !renderer) return;

    auto clearHoverOnly = [this]() {
        if (!chamferHoverEdgeActor_ && !hasChamferHoverEdge_) return;
        if (chamferHoverEdgeActor_) {
            removeSceneActor(chamferHoverEdgeActor_);
            chamferHoverEdgeActor_ = nullptr;
        }
        hasChamferHoverEdge_ = false;
        chamferHoverEdgeSubId_ = -1;
        chamferHoverModelIndex_ = -1;
        chamferHoverEdge_ = TopoDS_Edge();
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    };

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.02);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0, renderer);

    vtkSmartPointer<vtkActorCollection> picked = shapePicker->GetPickedActors(true);
    if (!picked || picked->GetNumberOfItems() == 0) {
        clearHoverOnly();
        return;
    }

    vtkActor* pickedActor = nullptr;
    picked->InitTraversal();
    while (vtkActor* a = picked->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) { pickedActor = a; break; }
    }
    if (!pickedActor) {
        clearHoverOnly();
        return;
    }

    int pickedIndex = resolveHistoryIndexByActor(pickedActor);
    if (pickedIndex < 0) {
        clearHoverOnly();
        return;
    }
    if (chamferTargetModelIndex_ >= 0 && chamferTargetModelIndex_ != pickedIndex) {
        clearHoverOnly();
        return;
    }

    IVtkTools_ShapeDataSource* ds = IVtkTools_ShapeObject::GetShapeSource(pickedActor);
    if (!ds) {
        clearHoverOnly();
        return;
    }
    Handle(IVtkOCC_Shape) wrapper = ds->GetShape();
    if (wrapper.IsNull()) {
        clearHoverOnly();
        return;
    }
    const IVtk_IdType shapeID = wrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subIds.IsEmpty()) {
        clearHoverOnly();
        return;
    }
    const IVtk_IdType subId = subIds.First();
    const TopoDS_Shape sub = wrapper->GetSubShape(subId);
    if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
        clearHoverOnly();
        return;
    }
    const TopoDS_Edge edge = TopoDS::Edge(sub);

    // 已选中的边不再叠一层悬停，避免移开后看起来像“残留高亮”
    for (const IVtk_IdType id : chamferSelectedEdgeSubIds_) {
        if (id == subId) {
            clearHoverOnly();
            return;
        }
    }

    if (hasChamferHoverEdge_ && !chamferHoverEdge_.IsNull() && chamferHoverEdge_.IsSame(edge)
        && chamferHoverEdgeSubId_ == subId) {
        return;
    }

    if (chamferHoverEdgeActor_) {
        removeSceneActor(chamferHoverEdgeActor_);
        chamferHoverEdgeActor_ = nullptr;
    }
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay ? overlay : renderer.GetPointer();
    addEdgeHighlightActor(target,
                          buildCleanEdgeHighlightActor(edge, 1.0, 0.25, 0.1, 3.5),
                          nullptr,
                          &chamferHoverEdgeActor_);
    if (!chamferHoverEdgeActor_) {
        clearHoverOnly();
        return;
    }
    chamferHoverEdge_ = edge;
    hasChamferHoverEdge_ = true;
    chamferHoverEdgeSubId_ = subId;
    chamferHoverModelIndex_ = pickedIndex;
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::updateFilletEdgeHighlight()
{
    clearFilletEdgeHighlight();
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay ? overlay : renderer.GetPointer();
    if (!target) return;

    for (const TopoDS_Edge& e : filletSelectedEdges_) {
        addEdgeHighlightActor(target,
                              buildCleanEdgeHighlightActor(e, 1.0, 0.15, 0.08, 4.5),
                              &filletEdgeHighlightActors_,
                              nullptr);
    }

    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}

void Widget::handleFilletEdgeClick(int x, int y)
{
    if (!shapePicker || !renderer) return;

    // 优先使用悬浮高亮的边：点击即选中当前高亮边
    if (hasFilletHoverEdge_ && !filletHoverEdge_.IsNull() && filletHoverEdgeSubId_ >= 0 && filletHoverModelIndex_ >= 0) {
        if (filletTargetModelIndex < 0) {
            filletTargetModelIndex = filletHoverModelIndex_;
        }
        if (filletTargetModelIndex == filletHoverModelIndex_) {
            const IVtk_IdType subId = filletHoverEdgeSubId_;
            const TopoDS_Edge edge = filletHoverEdge_;

            int existing = -1;
            for (int i = 0; i < filletSelectedEdgeSubIds_.size(); ++i) {
                if (filletSelectedEdgeSubIds_[i] == subId) {
                    existing = i;
                    break;
                }
            }
            if (existing >= 0) {
                filletSelectedEdgeSubIds_.removeAt(existing);
                filletSelectedEdges_.removeAt(existing);
            } else {
                filletSelectedEdgeSubIds_.append(subId);
                filletSelectedEdges_.append(edge);
            }

            updateFilletEdgeHighlight();
            if (filletDialog) {
                filletDialog->setSelectedEdgeCount(filletSelectedEdges_.size());
            }
            syncFilletChamferGhostAndPreview(true);
            updateFilletRadiusHandles();
            return;
        }
    }

    // 目标模型由第一次选边确定；后续限制在同一模型
    int targetModelIndex = filletTargetModelIndex;

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.05);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0);

    vtkSmartPointer<vtkActorCollection> picked = shapePicker->GetPickedActors(true);
    if (!picked || picked->GetNumberOfItems() == 0) return;

    vtkActor* pickedActor = nullptr;
    picked->InitTraversal();
    while (vtkActor* a = picked->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) {
            pickedActor = a;
            break;
        }
    }
    if (!pickedActor) return;
    int pickedIndex = resolveHistoryIndexByActor(pickedActor);
    if (pickedIndex < 0) return;
    if (targetModelIndex >= 0 && pickedIndex != targetModelIndex) return;
    if (targetModelIndex < 0) {
        filletTargetModelIndex = pickedIndex;
    }

    IVtkTools_ShapeDataSource* ds = IVtkTools_ShapeObject::GetShapeSource(pickedActor);
    if (!ds) return;
    Handle(IVtkOCC_Shape) wrapper = ds->GetShape();
    if (wrapper.IsNull()) return;

    const IVtk_IdType shapeID = wrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subIds.IsEmpty()) return;

    // 取第一个子形状（最近的边）
    const IVtk_IdType subId = subIds.First();
    const TopoDS_Shape sub = wrapper->GetSubShape(subId);
    if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
        return;
    }
    const TopoDS_Edge edge = TopoDS::Edge(sub);

    // toggle 选择
    int existing = -1;
    for (int i = 0; i < filletSelectedEdgeSubIds_.size(); ++i) {
        if (filletSelectedEdgeSubIds_[i] == subId) {
            existing = i;
            break;
        }
    }
    if (existing >= 0) {
        filletSelectedEdgeSubIds_.removeAt(existing);
        filletSelectedEdges_.removeAt(existing);
    } else {
        filletSelectedEdgeSubIds_.append(subId);
        filletSelectedEdges_.append(edge);
    }

    updateFilletEdgeHighlight();

    if (filletDialog) {
        filletDialog->setSelectedEdgeCount(filletSelectedEdges_.size());
    }
    syncFilletChamferGhostAndPreview(true);
    updateFilletRadiusHandles();
}

void Widget::handleFilletEdgeHover(int x, int y)
{
    if (!shapePicker || !renderer) return;

    auto clearHoverOnly = [this]() {
        if (!filletHoverEdgeActor_ && !hasFilletHoverEdge_) return;
        if (filletHoverEdgeActor_) {
            removeSceneActor(filletHoverEdgeActor_);
            filletHoverEdgeActor_ = nullptr;
        }
        hasFilletHoverEdge_ = false;
        filletHoverEdgeSubId_ = -1;
        filletHoverModelIndex_ = -1;
        filletHoverEdge_ = TopoDS_Edge();
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
    };

    shapePicker->SetRenderer(renderer);
    shapePicker->SetTolerance(0.02);
    shapePicker->SetSelectionMode(SM_Edge);
    shapePicker->Pick(x, y, 0, renderer);

    vtkSmartPointer<vtkActorCollection> picked = shapePicker->GetPickedActors(true);
    if (!picked || picked->GetNumberOfItems() == 0) {
        clearHoverOnly();
        return;
    }

    vtkActor* pickedActor = nullptr;
    picked->InitTraversal();
    while (vtkActor* a = picked->GetNextActor()) {
        if (a && a->GetVisibility() != 0 && a->GetPickable() != 0) {
            pickedActor = a;
            break;
        }
    }
    if (!pickedActor) {
        clearHoverOnly();
        return;
    }

    const int hoverModelIndex = resolveHistoryIndexByActor(pickedActor);
    if (hoverModelIndex < 0) {
        clearHoverOnly();
        return;
    }
    if (filletTargetModelIndex >= 0 && filletTargetModelIndex < historyList.size()
        && hoverModelIndex != filletTargetModelIndex) {
        clearHoverOnly();
        return;
    }

    IVtkTools_ShapeDataSource* ds = IVtkTools_ShapeObject::GetShapeSource(pickedActor);
    if (!ds) {
        clearHoverOnly();
        return;
    }
    Handle(IVtkOCC_Shape) wrapper = ds->GetShape();
    if (wrapper.IsNull()) {
        clearHoverOnly();
        return;
    }

    const IVtk_IdType shapeID = wrapper->GetId();
    IVtk_ShapeIdList subIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subIds.IsEmpty()) {
        clearHoverOnly();
        return;
    }

    const IVtk_IdType subId = subIds.First();
    const TopoDS_Shape sub = wrapper->GetSubShape(subId);
    if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
        clearHoverOnly();
        return;
    }
    const TopoDS_Edge edge = TopoDS::Edge(sub);

    for (const IVtk_IdType id : filletSelectedEdgeSubIds_) {
        if (id == subId) {
            clearHoverOnly();
            return;
        }
    }

    if (hasFilletHoverEdge_ && !filletHoverEdge_.IsNull() && filletHoverEdge_.IsSame(edge)
        && filletHoverEdgeSubId_ == subId) {
        return;
    }

    if (filletHoverEdgeActor_) {
        removeSceneActor(filletHoverEdgeActor_);
        filletHoverEdgeActor_ = nullptr;
    }
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay ? overlay : renderer.GetPointer();
    addEdgeHighlightActor(target,
                          buildCleanEdgeHighlightActor(edge, 1.0, 0.25, 0.1, 3.5),
                          nullptr,
                          &filletHoverEdgeActor_);
    if (!filletHoverEdgeActor_) {
        clearHoverOnly();
        return;
    }
    filletHoverEdge_ = edge;
    hasFilletHoverEdge_ = true;
    filletHoverEdgeSubId_ = subId;
    filletHoverModelIndex_ = hoverModelIndex;
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

// 挖空按钮点击事件
void Widget::handleHollow()
{
    if (currentSelectedIndex < 0 || currentSelectedIndex >= historyList.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个模型进行挖空操作！");
        return;
    }

    if (!isValidShapeForOperation(currentSelectedIndex)) {
        QMessageBox::warning(this, "警告", "选中的模型不支持挖空操作！");
        return;
    }

    // 临时：使用默认参数测试挖空
    performHollow(1.0);
}

void Widget::on_chamfer_clicked()
{
    // 清理上一次倒角选择/高亮
    clearChamferEdgeHighlight();
    chamferSelectedEdgeSubIds_.clear();
    chamferSelectedEdges_.clear();
    chamferTargetModelIndex_ = -1;

    chamferdialog* dialog = new chamferdialog(dialogParentWidget());
    dialog->setModal(false);
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    chamferDialog = dialog;

    // 点击倒角按钮后自动进入边选择模式（与拉伸一致：立刻全体幽灵）
    currentSelectionMode = ChamferEdgeSelection;
    setFeatureGizmoActorsPickable(false);
    setFeatureOperationGhostMode(true);
    if (statusBar()) {
        statusBar()->showMessage(tr("倒角：请在视图中点击选择要倒角的边（可多选）"), 5000);
    }
    if (vtkWidget) vtkWidget->setFocus();

    // “选择边”按钮（显示已选数量）：仅提示当前处于选边模式
    connect(dialog, &chamferdialog::edgeModeHintRequested, this, [this]() {
        if (statusBar()) statusBar()->showMessage(tr("当前为倒角-选择边模式"), 2000);
    });
    connect(dialog, &chamferdialog::parametersChanged, this, [this]() {
        refreshChamferLivePreview();
        updateChamferAsymHandles();
    });

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        if (chamferSelectedEdges_.isEmpty()) {
            if (statusBar()) statusBar()->showMessage(tr("未选择任何边"), 2000);
            return;
        }
        if (chamferTargetModelIndex_ < 0 || chamferTargetModelIndex_ >= historyList.size()) return;

        const QString section = dialog->sectionText();
        const bool twoDistances = (section == QStringLiteral("非对称"));

        const double d1 = twoDistances ? dialog->distance1Value() : dialog->distanceValue();
        const double d2 = twoDistances ? dialog->distance2Value() : d1;
        if (d1 <= 0.0 || d2 <= 0.0) {
            if (statusBar()) statusBar()->showMessage(tr("距离必须大于 0"), 2000);
            return;
        }

        if (section != QStringLiteral("对称") && !twoDistances) {
            if (statusBar()) statusBar()->showMessage(tr("当前仅实现“对称”倒角"), 3000);
            return;
        }

        try {
            const TopoDS_Shape originalShape = historyList[chamferTargetModelIndex_].occShape;
            if (originalShape.IsNull()) return;

            TopoDS_Shape res;
            if (!FeatureModificationGeometry::buildChamferShape(
                    originalShape, chamferSelectedEdges_, d1, d2, twoDistances, res)) {
                QMessageBox::warning(this, "错误", "倒角操作失败！");
                return;
            }
            const QString originalName = historyList[chamferTargetModelIndex_].name;
            const QString resultName = twoDistances
                ? QString("倒角(非对称,d1=%1,d2=%2)_%3").arg(d1).arg(d2).arg(originalName)
                : QString("倒角(对称,d=%1)_%2").arg(d1).arg(originalName);

            FeatureRecipe recipe;
            recipe.hasRecipe = true;
            recipe.parentIndices = { chamferTargetModelIndex_ };
            recipe.fillet.targetIndex = chamferTargetModelIndex_;
            recipe.fillet.isChamfer = true;
            recipe.fillet.chamferDistance = d1;
            recipe.fillet.chamferDistance2 = d2;
            recipe.fillet.isChamferTwoDistances = twoDistances;
            const TopoDS_Shape& parentShape = historyList[chamferTargetModelIndex_].occShape;
            for (const TopoDS_Edge& edge : chamferSelectedEdges_) {
                recipe.fillet.edges.append(
                    makeSubShapeRef(chamferTargetModelIndex_, parentShape, edge, TopAbs_EDGE));
            }

            executeCommand(new AddModelCommand(this, res, resultName, FILLET, QColor(255, 140, 0), d1,
                                               0.0, 0.0, QList<int>{ chamferTargetModelIndex_ }, true, &recipe));
        } catch (Standard_Failure& e) {
            QMessageBox::critical(this, "错误", QString("倒角操作异常: %1").arg(e.GetMessageString()));
        }
    });

    auto cleanupChamferState = [this]() {
        clearChamferAsymHandles();
        clearChamferEdgeHighlight();
        clearFeatureLivePreview();
        setFeatureOperationGhostMode(false);
        chamferSelectedEdgeSubIds_.clear();
        chamferSelectedEdges_.clear();
        chamferTargetModelIndex_ = -1;
        currentSelectionMode = None;
    };

    connect(dialog, &QDialog::rejected, this, [cleanupChamferState]() {
        cleanupChamferState();
    });

    connect(dialog, &QDialog::finished, this, [this, cleanupChamferState](int) {
        cleanupChamferState();
        chamferDialog = nullptr;
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// 执行挖空操作
void Widget::performHollow(double thickness)
{
    try {
        TopoDS_Shape originalShape = getSelectedOccShape();
        if (originalShape.IsNull()) {
            QMessageBox::warning(this, "错误", "无法获取原始形状！");
            return;
        }

        TopTools_ListOfShape facesToRemove;
        TopExp_Explorer faceExplorer;

        const int targetIndex = currentSelectedIndex;
        TopoDS_Face removedFace;
        for (faceExplorer.Init(originalShape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next()) {
            facesToRemove.Append(faceExplorer.Current());
            removedFace = TopoDS::Face(faceExplorer.Current());
            break; // 只选择第一个面
        }

        if (facesToRemove.IsEmpty() || removedFace.IsNull()) {
            QMessageBox::warning(this, "警告", "未找到合适的面进行挖空！");
            return;
        }

        TopoDS_Shape resultShape;
        if (!FeatureModificationGeometry::buildHollowShape(originalShape, facesToRemove, thickness, resultShape)) {
            QMessageBox::warning(this, "错误", "挖空操作失败！");
            return;
        }

        QString originalName = historyList[targetIndex].name;
        QString resultName = QString("挖空(厚度=%1)_%2").arg(thickness).arg(originalName);

        FeatureRecipe recipe;
        recipe.hasRecipe = true;
        recipe.parentIndices = { targetIndex };
        recipe.hollow.targetIndex = targetIndex;
        recipe.hollow.thickness = thickness;
        recipe.hollow.faces.append(makeSubShapeRef(targetIndex, originalShape, removedFace, TopAbs_FACE));

        executeCommand(new AddModelCommand(this, resultShape, resultName, HOLLOW, QColor(255, 140, 0), thickness,
                                           0.0, 0.0, QList<int>{ targetIndex }, true, &recipe));


    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("挖空操作异常: %1").arg(e.GetMessageString()));
    }
}

// ─── 倒角拖拽手柄（细线 + 小箭头，交互对齐拉伸手柄） ────────────────

namespace {

constexpr double kChamferArrowLen = 0.55; // 与拉伸手柄同量级（屏幕恒定尺寸）

constexpr double kChamferBlueR = 0.25;
constexpr double kChamferBlueG = 0.55;
constexpr double kChamferBlueB = 0.95;
constexpr double kChamferOrangeR = 1.00;
constexpr double kChamferOrangeG = 0.55;
constexpr double kChamferOrangeB = 0.15;
constexpr double kChamferHoverR = 1.0;
constexpr double kChamferHoverG = 0.85;
constexpr double kChamferHoverB = 0.2;
constexpr double kChamferDragR = 1.0;
constexpr double kChamferDragG = 0.25;
constexpr double kChamferDragB = 0.1;

constexpr double kChamferArrowHoverLenFactor = 1.08;
constexpr double kChamferArrowDragLenFactor = 1.22;
constexpr double kChamferLineDefaultWidth = 2.0;
constexpr double kChamferLineHoverWidth = 2.5;
constexpr double kChamferLineDragWidth = 3.3;

void setChamferArrowGeometry(vtkActor* actor, const gp_Pnt& tip, const gp_Dir& dir, double arrowLen)
{
    if (!actor) return;
    auto tf = HandleGeom::makeOrientedArrow(
        ControlShape::ArrowWithShaft, tip, dir, arrowLen,
        HandleGeom::defaultShaftArrowParams());
    if (auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())) {
        mapper->SetInputConnection(tf->GetOutputPort());
        mapper->Modified();
    }
}

void setChamferLineGeometry(vtkActor* actor, const gp_Pnt& a, const gp_Pnt& b)
{
    if (!actor) return;
    auto line = vtkSmartPointer<vtkLineSource>::New();
    line->SetPoint1(a.X(), a.Y(), a.Z());
    line->SetPoint2(b.X(), b.Y(), b.Z());
    if (auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())) {
        mapper->SetInputConnection(line->GetOutputPort());
        mapper->Modified();
    }
}

vtkSmartPointer<vtkActor> makeChamferArrowActor()
{
    auto arrow = HandleGeom::makeArrowSource(
        ControlShape::ArrowWithShaft, HandleGeom::defaultShaftArrowParams());
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(arrow->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetLighting(false);
    actor->GetProperty()->SetAmbient(1.0);
    actor->SetPickable(true);
    return actor;
}

vtkSmartPointer<vtkActor> makeChamferLineActor()
{
    auto line = vtkSmartPointer<vtkLineSource>::New();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(line->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetLighting(false);
    actor->GetProperty()->SetAmbient(1.0);
    actor->SetPickable(false);
    return actor;
}

void styleChamferHandlePair(vtkActor* arrow, vtkActor* line, bool active, bool hover,
                            double baseR, double baseG, double baseB)
{
    // 与全局操作柄四状态对齐：悬浮琥珀黄 / 拖拽橙红
    const double r = active ? 1.00 : hover ? 1.00 : baseR;
    const double g = active ? 0.25 : hover ? 0.85 : baseG;
    const double b = active ? 0.10 : hover ? 0.20 : baseB;
    const double lw = active ? kChamferLineDragWidth
                             : hover ? kChamferLineHoverWidth
                                     : kChamferLineDefaultWidth;
    auto paint = [&](vtkActor* a, bool isLine) {
        if (!a) return;
        vtkProperty* p = a->GetProperty();
        p->SetColor(r, g, b);
        p->SetAmbient(0.88);
        p->SetDiffuse(0.35);
        p->SetSpecular(0.05);
        if (isLine) p->SetLineWidth(lw);
    };
    paint(arrow, false);
    paint(line, true);
}

gp_Dir faceNormalDir(const TopoDS_Face& face)
{
    BRepGProp_Face prop(face);
    double u1 = 0, u2 = 0, v1 = 0, v2 = 0;
    prop.Bounds(u1, u2, v1, v2);
    gp_Pnt center;
    gp_Vec normal;
    prop.Normal((u1 + u2) * 0.5, (v1 + v2) * 0.5, center, normal);
    if (normal.Magnitude() < 1e-12) return gp_Dir(0, 0, 1);
    return gp_Dir(normal);
}

gp_Dir inFaceDirFromEdge(const gp_Vec& tangent, const gp_Dir& faceNormal, const gp_Vec& outwardHint)
{
    gp_Vec t = tangent;
    if (t.Magnitude() < 1e-12) return faceNormal;
    t.Normalize();
    gp_Vec d = gp_Vec(faceNormal).Crossed(t);
    if (d.Magnitude() < 1e-12) d = t.Crossed(gp_Vec(faceNormal));
    if (d.Magnitude() < 1e-12) return faceNormal;
    d.Normalize();
    if (outwardHint.Magnitude() > 1e-12 && d.Dot(outwardHint) > 0.0) d.Reverse();
    return gp_Dir(d);
}

bool chamferRayPlaneHit(vtkRenderer* ren, int x, int y, const gp_Pnt& planeOrigin,
                        const gp_Dir& planeNormal, gp_Pnt& outHit)
{
    if (!ren) return false;
    double nearPt[4] = {0, 0, 0, 1};
    double farPt[4] = {0, 0, 0, 1};
    ren->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 0.0);
    ren->DisplayToWorld();
    ren->GetWorldPoint(nearPt);
    ren->SetDisplayPoint(static_cast<double>(x), static_cast<double>(y), 1.0);
    ren->DisplayToWorld();
    ren->GetWorldPoint(farPt);
    if (std::abs(nearPt[3]) < 1e-12 || std::abs(farPt[3]) < 1e-12) return false;
    gp_Pnt p0(nearPt[0] / nearPt[3], nearPt[1] / nearPt[3], nearPt[2] / nearPt[3]);
    gp_Pnt p1(farPt[0] / farPt[3], farPt[1] / farPt[3], farPt[2] / farPt[3]);
    gp_Vec ray(p0, p1);
    if (ray.Magnitude() < 1e-12) return false;
    const double denom = ray.Dot(gp_Vec(planeNormal));
    if (std::abs(denom) < 1e-12) return false;
    const double t = gp_Vec(p0, planeOrigin).Dot(gp_Vec(planeNormal)) / denom;
    outHit = p0.Translated(ray.Multiplied(t));
    return true;
}

} // namespace

void Widget::updateChamferAsymHandles()
{
    if (!chamferDialog || chamferSelectedEdges_.isEmpty()
        || chamferTargetModelIndex_ < 0 || chamferTargetModelIndex_ >= historyList.size()) {
        clearChamferAsymHandles();
        return;
    }

    const QString section = chamferDialog->sectionText();
    const bool isAsym = (section == QStringLiteral("非对称"));
    const bool isSym = (section == QStringLiteral("对称"));
    if (!isAsym && !isSym) {
        clearChamferAsymHandles();
        return;
    }

    const TopoDS_Shape& parentShape = historyList[chamferTargetModelIndex_].occShape;
    if (parentShape.IsNull()) {
        clearChamferAsymHandles();
        return;
    }

    const TopoDS_Edge& edge = chamferSelectedEdges_.first();
    if (edge.IsNull()) {
        clearChamferAsymHandles();
        return;
    }

    BRepAdaptor_Curve curve(edge);
    const double uMid = (curve.FirstParameter() + curve.LastParameter()) * 0.5;
    gp_Pnt midPt;
    gp_Vec tangent;
    curve.D1(uMid, midPt, tangent);
    chamferAsymEdgeMid_ = midPt;
    if (tangent.Magnitude() < 1e-12) {
        clearChamferAsymHandles();
        return;
    }

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(parentShape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    if (!edgeFaceMap.Contains(edge)) {
        clearChamferAsymHandles();
        return;
    }

    const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
    if (faces.IsEmpty()) {
        clearChamferAsymHandles();
        return;
    }

    const TopoDS_Face& face1 = TopoDS::Face(faces.First());
    gp_Dir n1 = faceNormalDir(face1);
    gp_Dir n2 = n1.Reversed();
    bool hasFace2 = false;
    {
        TopTools_ListIteratorOfListOfShape it(faces);
        it.Next();
        if (it.More()) {
            n2 = faceNormalDir(TopoDS::Face(it.Value()));
            hasFace2 = true;
        }
    }
    gp_Vec outwardHint = gp_Vec(n1);
    if (hasFace2) outwardHint += gp_Vec(n2);
    chamferAsymDir1_ = inFaceDirFromEdge(tangent, n1, outwardHint);
    chamferAsymDir2_ = hasFace2 ? inFaceDirFromEdge(tangent, n2, outwardHint) : chamferAsymDir1_.Reversed();

    const double d1 = std::max(0.01, isSym ? chamferDialog->distanceValue() : chamferDialog->distance1Value());
    const double d2 = std::max(0.01, chamferDialog->distance2Value());
    const gp_Pnt tip1 = midPt.Translated(gp_Vec(chamferAsymDir1_) * d1);
    const gp_Pnt tip2 = midPt.Translated(gp_Vec(chamferAsymDir2_) * d2);

    const bool side1Active = chamferAsymHandleDragging_ && chamferAsymActiveSide_ == 0;
    const bool side2Active = chamferAsymHandleDragging_ && chamferAsymActiveSide_ == 1;
    const bool side1Hover = !chamferAsymHandleDragging_ && chamferAsymHoverSide_ == 0;
    const bool side2Hover = !chamferAsymHandleDragging_ && chamferAsymHoverSide_ == 1;

    auto arrowLenAt = [&](const gp_Pnt& tip, bool active, bool hover) {
        const double factor = active ? kChamferArrowDragLenFactor
                                     : hover ? kChamferArrowHoverLenFactor
                                             : 1.0;
        return kChamferArrowLen * factor * overlayWorldScaleAt(tip.X(), tip.Y(), tip.Z());
    };

    // 拖拽中：只更新几何，不销毁 Actor
    const bool canUpdateInPlace = chamferAsymHandleDragging_
        && chamferAsymHandleSide1Actor_ && chamferAsymHandleLine1Actor_
        && (!isAsym || (chamferAsymHandleSide2Actor_ && chamferAsymHandleLine2Actor_));

    if (canUpdateInPlace) {
        setChamferLineGeometry(chamferAsymHandleLine1Actor_, midPt, tip1);
        setChamferArrowGeometry(chamferAsymHandleSide1Actor_, tip1, chamferAsymDir1_,
                                arrowLenAt(tip1, side1Active, false));
        styleChamferHandlePair(chamferAsymHandleSide1Actor_, chamferAsymHandleLine1Actor_,
                               side1Active, false, kChamferBlueR, kChamferBlueG, kChamferBlueB);
        if (isAsym) {
            setChamferLineGeometry(chamferAsymHandleLine2Actor_, midPt, tip2);
            setChamferArrowGeometry(chamferAsymHandleSide2Actor_, tip2, chamferAsymDir2_,
                                    arrowLenAt(tip2, side2Active, false));
            styleChamferHandlePair(chamferAsymHandleSide2Actor_, chamferAsymHandleLine2Actor_,
                                   side2Active, false, kChamferOrangeR, kChamferOrangeG, kChamferOrangeB);
        }
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }

    // 重建（保留拖拽状态）
    const bool keepDragging = chamferAsymHandleDragging_;
    const int keepSide = chamferAsymActiveSide_;
    const int keepHover = chamferAsymHoverSide_;
    clearChamferAsymHandles();
    chamferAsymHandleDragging_ = keepDragging;
    chamferAsymActiveSide_ = keepSide;
    chamferAsymHoverSide_ = keepHover;

    chamferAsymHandleLine1Actor_ = makeChamferLineActor();
    chamferAsymHandleSide1Actor_ = makeChamferArrowActor();
    setChamferLineGeometry(chamferAsymHandleLine1Actor_, midPt, tip1);
    setChamferArrowGeometry(chamferAsymHandleSide1Actor_, tip1, chamferAsymDir1_,
                            arrowLenAt(tip1, side1Active, side1Hover));
    styleChamferHandlePair(chamferAsymHandleSide1Actor_, chamferAsymHandleLine1Actor_,
                           side1Active, side1Hover, kChamferBlueR, kChamferBlueG, kChamferBlueB);
    addReferenceActor(chamferAsymHandleLine1Actor_);
    addReferenceActor(chamferAsymHandleSide1Actor_);

    if (isAsym) {
        chamferAsymHandleLine2Actor_ = makeChamferLineActor();
        chamferAsymHandleSide2Actor_ = makeChamferArrowActor();
        setChamferLineGeometry(chamferAsymHandleLine2Actor_, midPt, tip2);
        setChamferArrowGeometry(chamferAsymHandleSide2Actor_, tip2, chamferAsymDir2_,
                                arrowLenAt(tip2, side2Active, side2Hover));
        styleChamferHandlePair(chamferAsymHandleSide2Actor_, chamferAsymHandleLine2Actor_,
                               side2Active, side2Hover, kChamferOrangeR, kChamferOrangeG, kChamferOrangeB);
        addReferenceActor(chamferAsymHandleLine2Actor_);
        addReferenceActor(chamferAsymHandleSide2Actor_);
    }

    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::clearChamferAsymHandles()
{
    auto remove = [&](vtkSmartPointer<vtkActor>& a) {
        if (a) removeSceneActor(a);
        a = nullptr;
    };
    remove(chamferAsymHandleSide1Actor_);
    remove(chamferAsymHandleSide2Actor_);
    remove(chamferAsymHandleLine1Actor_);
    remove(chamferAsymHandleLine2Actor_);
    chamferAsymHandleDragging_ = false;
    chamferAsymActiveSide_ = -1;
    chamferAsymHoverSide_ = -1;
}

void Widget::handleChamferAsymHandleMouseDown(int x, int y)
{
    if (!chamferAsymHandleSide1Actor_ && !chamferAsymHandleSide2Actor_) return;
    if (!renderer) return;

    auto picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->PickFromListOn();
    picker->InitializePickList();
    if (chamferAsymHandleSide1Actor_) picker->AddPickList(chamferAsymHandleSide1Actor_);
    if (chamferAsymHandleSide2Actor_) picker->AddPickList(chamferAsymHandleSide2Actor_);
    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* picked = picker->GetActor();

    int side = -1;
    if (picked && chamferAsymHandleSide1Actor_ && picked == chamferAsymHandleSide1Actor_.GetPointer()) {
        side = 0;
    } else if (picked && chamferAsymHandleSide2Actor_ && picked == chamferAsymHandleSide2Actor_.GetPointer()) {
        side = 1;
    }
    if (side < 0) return;

    chamferAsymHandleDragging_ = true;
    chamferAsymActiveSide_ = side;
    chamferAsymHoverSide_ = -1;
    chamferAsymDragStartX_ = x;
    chamferAsymDragStartY_ = y;
    chamferAsymStartD1_ = chamferDialog ? chamferDialog->distance1Value() : 1.0;
    chamferAsymStartD2_ = chamferDialog ? chamferDialog->distance2Value() : 1.0;
    currentSelectionMode = ChamferAsymHandleDrag;
    updateChamferAsymHandles();
}

void Widget::handleChamferAsymHandleMouseMove(int x, int y)
{
    if (!chamferDialog || !renderer) return;

    // 非拖拽：悬浮高亮
    if (!chamferAsymHandleDragging_) {
        if (!chamferAsymHandleSide1Actor_ && !chamferAsymHandleSide2Actor_) return;
        auto picker = vtkSmartPointer<vtkPropPicker>::New();
        picker->PickFromListOn();
        picker->InitializePickList();
        if (chamferAsymHandleSide1Actor_) picker->AddPickList(chamferAsymHandleSide1Actor_);
        if (chamferAsymHandleSide2Actor_) picker->AddPickList(chamferAsymHandleSide2Actor_);
        picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
        vtkActor* hit = picker->GetActor();
        int hover = -1;
        if (hit && chamferAsymHandleSide1Actor_ && hit == chamferAsymHandleSide1Actor_.GetPointer()) hover = 0;
        else if (hit && chamferAsymHandleSide2Actor_ && hit == chamferAsymHandleSide2Actor_.GetPointer()) hover = 1;
        if (hover != chamferAsymHoverSide_) {
            chamferAsymHoverSide_ = hover;
            updateChamferAsymHandles();
        }
        return;
    }

    const bool isSym = (chamferDialog->sectionText() == QStringLiteral("对称"));
    const gp_Dir& dir = (chamferAsymActiveSide_ == 1) ? chamferAsymDir2_ : chamferAsymDir1_;

    gp_Dir planeN(0, 0, 1);
    if (auto* cam = renderer->GetActiveCamera()) {
        double fp[3] = {0, 0, 0};
        cam->GetDirectionOfProjection(fp);
        const double mag = std::sqrt(fp[0] * fp[0] + fp[1] * fp[1] + fp[2] * fp[2]);
        if (mag > 1e-12) planeN = gp_Dir(fp[0] / mag, fp[1] / mag, fp[2] / mag);
    }

    gp_Pnt hit;
    if (!chamferRayPlaneHit(renderer, x, y, chamferAsymEdgeMid_, planeN, hit)) return;
    const double dist = std::max(0.01, gp_Vec(chamferAsymEdgeMid_, hit).Dot(gp_Vec(dir)));

    if (isSym || chamferAsymActiveSide_ == 0) {
        chamferDialog->setDistance1Value(dist);
    } else {
        chamferDialog->setDistance2Value(dist);
    }
    refreshChamferLivePreview();
    updateChamferAsymHandles();
}

void Widget::handleChamferAsymHandleMouseUp(int /*x*/, int /*y*/)
{
    chamferAsymHandleDragging_ = false;
    chamferAsymActiveSide_ = -1;
    currentSelectionMode = ChamferEdgeSelection;
    updateChamferAsymHandles();
}

void Widget::updateFilletRadiusHandles()
{
    if (!filletDialog || filletSelectedEdges_.isEmpty()
        || filletTargetModelIndex < 0 || filletTargetModelIndex >= historyList.size()) {
        clearFilletRadiusHandles();
        return;
    }

    const TopoDS_Shape& parentShape = historyList[filletTargetModelIndex].occShape;
    if (parentShape.IsNull()) {
        clearFilletRadiusHandles();
        return;
    }

    const TopoDS_Edge& edge = filletSelectedEdges_.first();
    if (edge.IsNull()) {
        clearFilletRadiusHandles();
        return;
    }

    BRepAdaptor_Curve curve(edge);
    const double uMid = (curve.FirstParameter() + curve.LastParameter()) * 0.5;
    gp_Pnt midPt;
    gp_Vec tangent;
    curve.D1(uMid, midPt, tangent);
    filletRadiusEdgeMid_ = midPt;
    if (tangent.Magnitude() < 1e-12) {
        clearFilletRadiusHandles();
        return;
    }

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(parentShape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    if (!edgeFaceMap.Contains(edge)) {
        clearFilletRadiusHandles();
        return;
    }

    const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
    if (faces.IsEmpty()) {
        clearFilletRadiusHandles();
        return;
    }

    const TopoDS_Face& face1 = TopoDS::Face(faces.First());
    gp_Dir n1 = faceNormalDir(face1);
    gp_Dir n2 = n1.Reversed();
    bool hasFace2 = false;
    {
        TopTools_ListIteratorOfListOfShape it(faces);
        it.Next();
        if (it.More()) {
            n2 = faceNormalDir(TopoDS::Face(it.Value()));
            hasFace2 = true;
        }
    }
    gp_Vec outwardHint = gp_Vec(n1);
    if (hasFace2) outwardHint += gp_Vec(n2);
    filletRadiusDir1_ = inFaceDirFromEdge(tangent, n1, outwardHint);
    filletRadiusDir2_ = hasFace2 ? inFaceDirFromEdge(tangent, n2, outwardHint) : filletRadiusDir1_.Reversed();

    const double radius = std::max(0.01, filletDialog->radiusValue());
    const gp_Pnt tip1 = midPt.Translated(gp_Vec(filletRadiusDir1_) * radius);
    const gp_Pnt tip2 = midPt.Translated(gp_Vec(filletRadiusDir2_) * radius);

    const bool side1Active = filletRadiusHandleDragging_ && filletRadiusActiveSide_ == 0;
    const bool side2Active = filletRadiusHandleDragging_ && filletRadiusActiveSide_ == 1;
    const bool side1Hover = !filletRadiusHandleDragging_ && filletRadiusHoverSide_ == 0;
    const bool side2Hover = !filletRadiusHandleDragging_ && filletRadiusHoverSide_ == 1;

    auto arrowLenAt = [&](const gp_Pnt& tip, bool active, bool hover) {
        const double factor = active ? kChamferArrowDragLenFactor
                                     : hover ? kChamferArrowHoverLenFactor
                                             : 1.0;
        return kChamferArrowLen * factor * overlayWorldScaleAt(tip.X(), tip.Y(), tip.Z());
    };

    const bool canUpdateInPlace = filletRadiusHandleDragging_
        && filletRadiusHandleSide1Actor_ && filletRadiusHandleLine1Actor_
        && filletRadiusHandleSide2Actor_ && filletRadiusHandleLine2Actor_;

    if (canUpdateInPlace) {
        setChamferLineGeometry(filletRadiusHandleLine1Actor_, midPt, tip1);
        setChamferArrowGeometry(filletRadiusHandleSide1Actor_, tip1, filletRadiusDir1_,
                                arrowLenAt(tip1, side1Active, false));
        styleChamferHandlePair(filletRadiusHandleSide1Actor_, filletRadiusHandleLine1Actor_,
                               side1Active, false, kChamferBlueR, kChamferBlueG, kChamferBlueB);
        setChamferLineGeometry(filletRadiusHandleLine2Actor_, midPt, tip2);
        setChamferArrowGeometry(filletRadiusHandleSide2Actor_, tip2, filletRadiusDir2_,
                                arrowLenAt(tip2, side2Active, false));
        styleChamferHandlePair(filletRadiusHandleSide2Actor_, filletRadiusHandleLine2Actor_,
                               side2Active, false, kChamferOrangeR, kChamferOrangeG, kChamferOrangeB);
        if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
        return;
    }

    const bool keepDragging = filletRadiusHandleDragging_;
    const int keepSide = filletRadiusActiveSide_;
    const int keepHover = filletRadiusHoverSide_;
    clearFilletRadiusHandles();
    filletRadiusHandleDragging_ = keepDragging;
    filletRadiusActiveSide_ = keepSide;
    filletRadiusHoverSide_ = keepHover;

    filletRadiusHandleLine1Actor_ = makeChamferLineActor();
    filletRadiusHandleSide1Actor_ = makeChamferArrowActor();
    setChamferLineGeometry(filletRadiusHandleLine1Actor_, midPt, tip1);
    setChamferArrowGeometry(filletRadiusHandleSide1Actor_, tip1, filletRadiusDir1_,
                            arrowLenAt(tip1, side1Active, side1Hover));
    styleChamferHandlePair(filletRadiusHandleSide1Actor_, filletRadiusHandleLine1Actor_,
                           side1Active, side1Hover, kChamferBlueR, kChamferBlueG, kChamferBlueB);
    addReferenceActor(filletRadiusHandleLine1Actor_);
    addReferenceActor(filletRadiusHandleSide1Actor_);

    filletRadiusHandleLine2Actor_ = makeChamferLineActor();
    filletRadiusHandleSide2Actor_ = makeChamferArrowActor();
    setChamferLineGeometry(filletRadiusHandleLine2Actor_, midPt, tip2);
    setChamferArrowGeometry(filletRadiusHandleSide2Actor_, tip2, filletRadiusDir2_,
                            arrowLenAt(tip2, side2Active, side2Hover));
    styleChamferHandlePair(filletRadiusHandleSide2Actor_, filletRadiusHandleLine2Actor_,
                           side2Active, side2Hover, kChamferOrangeR, kChamferOrangeG, kChamferOrangeB);
    addReferenceActor(filletRadiusHandleLine2Actor_);
    addReferenceActor(filletRadiusHandleSide2Actor_);

    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

void Widget::clearFilletRadiusHandles()
{
    auto remove = [&](vtkSmartPointer<vtkActor>& a) {
        if (a) removeSceneActor(a);
        a = nullptr;
    };
    remove(filletRadiusHandleSide1Actor_);
    remove(filletRadiusHandleSide2Actor_);
    remove(filletRadiusHandleLine1Actor_);
    remove(filletRadiusHandleLine2Actor_);
    filletRadiusHandleDragging_ = false;
    filletRadiusActiveSide_ = -1;
    filletRadiusHoverSide_ = -1;
}

void Widget::handleFilletRadiusHandleMouseDown(int x, int y)
{
    if (!filletRadiusHandleSide1Actor_ && !filletRadiusHandleSide2Actor_) return;
    if (!renderer) return;

    auto picker = vtkSmartPointer<vtkPropPicker>::New();
    picker->PickFromListOn();
    picker->InitializePickList();
    if (filletRadiusHandleSide1Actor_) picker->AddPickList(filletRadiusHandleSide1Actor_);
    if (filletRadiusHandleSide2Actor_) picker->AddPickList(filletRadiusHandleSide2Actor_);
    picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
    vtkActor* picked = picker->GetActor();

    int side = -1;
    if (picked && filletRadiusHandleSide1Actor_ && picked == filletRadiusHandleSide1Actor_.GetPointer()) {
        side = 0;
    } else if (picked && filletRadiusHandleSide2Actor_ && picked == filletRadiusHandleSide2Actor_.GetPointer()) {
        side = 1;
    }
    if (side < 0) return;

    filletRadiusHandleDragging_ = true;
    filletRadiusActiveSide_ = side;
    filletRadiusHoverSide_ = -1;
    currentSelectionMode = FilletRadiusHandleDrag;
    updateFilletRadiusHandles();
}

void Widget::handleFilletRadiusHandleMouseMove(int x, int y)
{
    if (!filletDialog || !renderer) return;

    if (!filletRadiusHandleDragging_) {
        if (!filletRadiusHandleSide1Actor_ && !filletRadiusHandleSide2Actor_) return;
        auto picker = vtkSmartPointer<vtkPropPicker>::New();
        picker->PickFromListOn();
        picker->InitializePickList();
        if (filletRadiusHandleSide1Actor_) picker->AddPickList(filletRadiusHandleSide1Actor_);
        if (filletRadiusHandleSide2Actor_) picker->AddPickList(filletRadiusHandleSide2Actor_);
        picker->Pick(x, y, 0, referenceOverlay() ? referenceOverlay() : renderer.GetPointer());
        vtkActor* hit = picker->GetActor();
        int hover = -1;
        if (hit && filletRadiusHandleSide1Actor_ && hit == filletRadiusHandleSide1Actor_.GetPointer()) hover = 0;
        else if (hit && filletRadiusHandleSide2Actor_ && hit == filletRadiusHandleSide2Actor_.GetPointer()) hover = 1;
        if (hover != filletRadiusHoverSide_) {
            filletRadiusHoverSide_ = hover;
            updateFilletRadiusHandles();
        }
        return;
    }

    const gp_Dir& dir = (filletRadiusActiveSide_ == 1) ? filletRadiusDir2_ : filletRadiusDir1_;

    gp_Dir planeN(0, 0, 1);
    if (auto* cam = renderer->GetActiveCamera()) {
        double fp[3] = {0, 0, 0};
        cam->GetDirectionOfProjection(fp);
        const double mag = std::sqrt(fp[0] * fp[0] + fp[1] * fp[1] + fp[2] * fp[2]);
        if (mag > 1e-12) planeN = gp_Dir(fp[0] / mag, fp[1] / mag, fp[2] / mag);
    }

    gp_Pnt hit;
    if (!chamferRayPlaneHit(renderer, x, y, filletRadiusEdgeMid_, planeN, hit)) return;
    const double radius = std::max(0.01, gp_Vec(filletRadiusEdgeMid_, hit).Dot(gp_Vec(dir)));

    filletDialog->setRadiusValue(radius);
    refreshFilletLivePreview();
    updateFilletRadiusHandles();
}

void Widget::handleFilletRadiusHandleMouseUp(int /*x*/, int /*y*/)
{
    filletRadiusHandleDragging_ = false;
    filletRadiusActiveSide_ = -1;
    currentSelectionMode = FilletEdgeSelection;
    updateFilletRadiusHandles();
}
