// 坐标与点选相关：工作坐标系放置、模型点选择、最近点击坐标
#include "main_window.h"
#include "ui_main_window.h"

#include "presentation/dialogs/primitives/cuboid_params_dialog.h"
#include "presentation/dialogs/primitives/cylinder_dialog.h"
#include "presentation/dialogs/primitives/cone_params_dialog.h"
#include "presentation/dialogs/primitives/sphere_params_dialog.h"
#include "presentation/dialogs/extrude_revolve/revolve_dialog.h"
#include "presentation/dialogs/pattern/pattern_feature_dialog.h"
#include "geometry/sketch/sketch_geometry.h"

#include <cmath>

#include <QMessageBox>
#include <QStatusBar>

#include <Standard_Real.hxx>

#include <BRep_Tool.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtk_Types.hxx>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTransform.h>

void Widget::on_workAxisButton_clicked()
{
    if (hasSnapSelectedPoint_) {
        if (!renderer || !vtkWidget) return;
        const gp_Pnt& p = snapSelectedPoint_;
        if (!ensureWorkCsysActorsCreated()) return;
        const bool needCreate = (workCsysHistoryIndex_ < 0);

        workCsysTransform->Identity();
        workCsysTransform->Translate(p.X(), p.Y(), p.Z());
        hasWorkCsys = true;
        workCsysDragActive = false;
        setWorkCsysVisible(true);
        applyAxisDirectionHighlight(currentAxisDirection);
        currentSelectionMode = None;

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }

        if (needCreate) {
            addToHistory(WORK_CSYS, tr("工作坐标系"),
                         workCsysActor, QColor(255, 230, 80),
                         p.X(), p.Y(), p.Z(),
                         nullptr, TopoDS_Shape(), nullptr, nullptr);
            workCsysHistoryIndex_ = historyList.size() - 1;
        } else if (workCsysHistoryIndex_ >= 0 && workCsysHistoryIndex_ < historyList.size()
                   && historyList[workCsysHistoryIndex_].type == WORK_CSYS) {
            historyList[workCsysHistoryIndex_].param1 = p.X();
            historyList[workCsysHistoryIndex_].param2 = p.Y();
            historyList[workCsysHistoryIndex_].param3 = p.Z();
        }

        if (statusBar()) {
            statusBar()->showMessage(tr("已在捕捉点处创建/移动工作坐标系。"), 2000);
        }
        return;
    }

    currentSelectionMode = WorkCsysPlacement;
    if (statusBar()) {
        statusBar()->showMessage(tr("工作坐标系：请在 3D 视图中点击一个位置来创建/移动。"), 4000);
    }
    if (vtkWidget) {
        vtkWidget->setFocus();
    }
}

void Widget::handlePointSelection(vtkActor* selectedActor, int x, int y)
{
    if (!selectedActor || !shapePicker) {
        QMessageBox::warning(this, "提示", "请点击模型上的点/边/面来选择原点（当前未拾取到模型）！");
        return;
    }

    const bool hasActiveDialog =
        cuboidDialog || cylinderDialog || coneDialog || sphereDialog || revolveDialog || patternDialog_;
    if (!hasActiveDialog) {
        return;
    }

    IVtkTools_ShapeDataSource* dataSource =
        IVtkTools_ShapeObject::GetShapeSource(selectedActor);
    if (!dataSource) {
        QMessageBox::warning(this, "错误", "无法获取形状数据源！");
        return;
    }

    Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
    if (shapeWrapper.IsNull()) {
        QMessageBox::warning(this, "错误", "无法获取形状包装器！");
        return;
    }

    const IVtk_IdType shapeID = shapeWrapper->GetId();
    const IVtk_ShapeIdList subShapeIds = shapePicker->GetPickedSubShapesIds(shapeID);
    if (subShapeIds.IsEmpty()) {
        QMessageBox::warning(this, "提示", "未拾取到有效的点或边，请点击模型的顶点或棱！");
        return;
    }

    gp_Pnt selectedPoint;
    bool pointFound = false;
    QString pointType = "点";

    gp_Pnt rayOrigin;
    gp_Dir rayDir(0, 0, 1);
    bool hasRay = false;
    if (renderer) {
        double worldNear[4] = {0, 0, 0, 1};
        double worldFar[4]  = {0, 0, 1, 1};

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

    if (hasRay) {
        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            const IVtk_IdType subShapeId = sIt.Value();
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
            if (subShape.ShapeType() != TopAbs_FACE) continue;

            TopoDS_Face face = TopoDS::Face(subShape);
            gp_Lin ray(rayOrigin, rayDir);

            IntCurvesFace_ShapeIntersector intersector;
            intersector.Load(face, Precision::Confusion());
            intersector.Perform(ray, 0.0, 1.0e9);

            if (intersector.NbPnt() <= 0) {
                continue;
            }

            Standard_Real bestW = RealLast();
            gp_Pnt bestP;
            bool hasBest = false;
            for (int i = 1; i <= intersector.NbPnt(); ++i) {
                const Standard_Real w = intersector.WParameter(i);
                if (w >= 0.0 && w < bestW) {
                    bestW = w;
                    bestP = intersector.Pnt(i);
                    hasBest = true;
                }
            }

            if (hasBest) {
                selectedPoint = bestP;
                pointType = "面点";
                pointFound = true;
                break;
            }
        }
    }

    if (!pointFound) {
        for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
            const IVtk_IdType subShapeId = sIt.Value();
            const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);

            if (subShape.ShapeType() == TopAbs_VERTEX) {
                TopoDS_Vertex vertex = TopoDS::Vertex(subShape);
                selectedPoint = BRep_Tool::Pnt(vertex);
                pointType = "顶点";
                pointFound = true;
                break;
            } else if (subShape.ShapeType() == TopAbs_EDGE) {
                TopoDS_Edge edge = TopoDS::Edge(subShape);
                if (SketchGeometry::edgeMidPoint(edge, selectedPoint)) {
                    pointType = "中点";
                    pointFound = true;
                    break;
                }
            }
        }
    }

    if (!pointFound) {
        QMessageBox::warning(this, "提示", "未拾取到有效的点、边或面，请点击模型的顶点、棱或圆形面！");
        return;
    }

    if (cuboidDialog) {
        cuboidDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
        if (cuboidInteractiveActive_) {
            applyCuboidInteractiveOrigin(selectedPoint);
        }
    } else if (cylinderDialog) {
        cylinderDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (coneDialog) {
        coneDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (sphereDialog) {
        sphereDialog->setOriginPoint(selectedPoint.X(), selectedPoint.Y(), selectedPoint.Z());
    } else if (patternDialog_) {
        patternDialog_->setRotationCenter(selectedPoint, true);
        if (patternDialog_->hasDirection1()) {
            updatePatternRotationAxisArrow();
        }
    }

    selectedOriginPoint = selectedPoint;
    hasSelectedOriginPoint = true;

    const QString label = QString("%1\n(%2, %3, %4)")
        .arg(pointType)
        .arg(selectedPoint.X(), 0, 'f', 2)
        .arg(selectedPoint.Y(), 0, 'f', 2)
        .arg(selectedPoint.Z(), 0, 'f', 2);

    showSelectedPoint(selectedPoint, label);
    clearPointSelectionHover();

    if (cuboidInteractiveActive_) {
        currentSelectionMode = PointSelection;
        updateCuboidInteractivePreview();
    } else if (patternDialog_) {
        restorePatternPitchInteractiveAfterOriginPick();
    } else {
        currentSelectionMode = None;
    }
}

void Widget::getLastWorldPoint(double worldPoint[4]) const
{
    worldPoint[0] = lastWorldPoint[0];
    worldPoint[1] = lastWorldPoint[1];
    worldPoint[2] = lastWorldPoint[2];
    worldPoint[3] = lastWorldPoint[3];
}
