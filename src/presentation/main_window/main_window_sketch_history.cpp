#include "main_window.h"
#include "rendering/model/model_display_style.h"
#include "rendering/model/shape_presentation_factory.h"
#include "geometry/sketch/sketch_geometry.h"
#include "rendering/pipeline/model_shape_pipeline.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>

#include <TopAbs_ShapeEnum.hxx>

#include <vtkCellArray.h>
#include <vtkDataSetMapper.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>


void Widget::ensureSketchHistoryRecord()
{
    if (activeSketchHistoryIndex_ >= 0 && activeSketchHistoryIndex_ < historyList.size()
        && historyList[activeSketchHistoryIndex_].type == SKETCH) {
        return;
    }

    // 为草图创建独立的“曲线显示”历史项（线框渲染，保留 lines）
    const QColor color(60, 170, 255);
    const gp_Pnt o = activeSketchPlane_.Location();
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(o, gp_Pnt(o.X() + 1e-3, o.Y(), o.Z())).Edge();

    ++shapeIDCounter;
    Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(e);
    shapeWrapper->SetId(shapeIDCounter);

    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
        vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    shapeDataSource->SetShape(shapeWrapper);
    shapeDataSource->Modified();
    shapeDataSource->Update();
    // 草图主显示与高亮统一使用 shapeDataSource。
    // 注意：纯线框草图（尤其是仅包含 Edge）在该输出里可能没有可用点集，
    // 不能在这里提前返回，否则会出现“只看到预览高亮、正式草图不落地”的现象。
    vtkPolyData* meshPolyData = vtkPolyData::SafeDownCast(shapeDataSource->GetOutput());

    vtkSmartPointer<vtkPolyData> polyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    polyDataCopy->ShallowCopy(meshPolyData);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyDataCopy);
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPickable(true);
    ModelDisplayStyle::applySketchModelAppearance(actor->GetProperty(), color);

    IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, actor);
    // 注意：SetShapeSource 可能改写 mapper 连接，最后再强制回设显示 mapper
    actor->SetMapper(mapper);

    vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter =
        vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
    highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
    highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");

    vtkSmartPointer<vtkPolyDataMapper> highlightMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    highlightMapper->SetInputConnection(highlightFilter->GetOutputPort());
    highlightMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> highlightActor = vtkSmartPointer<vtkActor>::New();
    highlightActor->SetMapper(highlightMapper);
    highlightActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
    highlightActor->GetProperty()->SetOpacity(1.0);
    highlightActor->GetProperty()->SetRepresentationToWireframe();
    highlightActor->GetProperty()->SetLineWidth(4.0);
    highlightActor->GetProperty()->SetLighting(false);
    highlightActor->SetVisibility(false);
    highlightActor->SetPickable(false);

    renderer->AddActor(actor);
    addAppearanceActor(highlightActor);

    ModelingHistory record;
    QString sketchName = tr("草图");
    if (sketchSelectedPlaneHistoryIndex_ >= 0
        && sketchSelectedPlaneHistoryIndex_ < historyList.size()) {
        sketchName = tr("草图(%1)").arg(historyList[sketchSelectedPlaneHistoryIndex_].name);
    }
    record.type = SKETCH;
    record.name = sketchName;
    record.timestamp = QDateTime::currentDateTime();
    record.color = color;
    record.param1 = 0;
    record.param2 = 0;
    record.param3 = 0;

    const int index = modelDocument_.append(record);
    ModelingHistory& storedRecord = historyList[index];
    renderStateFor(storedRecord).actor = actor;
    renderStateFor(storedRecord).polyData = polyDataCopy;
    // 只有实际绘制出来的草图几何才是 Profile 来源；哨兵边只服务于线框显示/拾取。
    geometryStateFor(storedRecord).occShape = TopoDS_Shape();
    renderStateFor(storedRecord).shapeWrapper = shapeWrapper;
    renderStateFor(storedRecord).shapeDataSource = shapeDataSource;
    renderStateFor(storedRecord).highlightFilter = highlightFilter;
    renderStateFor(storedRecord).highlightActor = highlightActor;
    activeSketchHistoryIndex_ = index;

    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    const gp_Ax3 sketchAx = activeSketchPlane_.Position();
    recipe.sketch.planeOrigin = sketchAx.Location();
    recipe.sketch.planeNormal = sketchAx.Direction();
    recipe.sketch.planeXDir = sketchAx.XDirection();
    if (sketchSelectedPlaneHistoryIndex_ >= 0) {
        recipe.sketch.datumPlaneIndex = sketchSelectedPlaneHistoryIndex_;
        recipe.parentIndices.append(sketchSelectedPlaneHistoryIndex_);
    }
    assignFeatureRecipe(activeSketchHistoryIndex_, recipe);

    updateFeatureTree();
    markDocumentModified(true);
}

void Widget::updateSketchHistoryShape()
{
    if (activeSketchHistoryIndex_ < 0 || activeSketchHistoryIndex_ >= historyList.size()) return;
    if (!hasActiveSketch_) return;

    ModelingHistory& history = historyList[activeSketchHistoryIndex_];
    const QList<TopoDS_Shape> geometries = activeSketch_.getGeometries();
    if (geometries.isEmpty()) {
        geometryStateFor(history).occShape = TopoDS_Shape();
        renderStateFor(history).polyData = vtkSmartPointer<vtkPolyData>::New();
        if (renderStateFor(history).actor) {
            vtkSmartPointer<vtkPolyDataMapper> emptyMapper =
                vtkSmartPointer<vtkPolyDataMapper>::New();
            emptyMapper->SetInputData(renderStateFor(history).polyData);
            emptyMapper->ScalarVisibilityOff();
            renderStateFor(history).actor->SetMapper(emptyMapper);
            renderStateFor(history).actor->SetVisibility(false);
            renderStateFor(history).actor->SetPickable(false);
        }
        if (renderStateFor(history).highlightActor) {
            renderStateFor(history).highlightActor->SetVisibility(false);
        }
        if (renderStateFor(history).profilePickActor) {
            removeSceneActor(renderStateFor(history).profilePickActor);
            renderStateFor(history).profilePickActor = nullptr;
        }
        renderStateFor(history).profilePickShapeWrapper = nullptr;
        renderStateFor(history).profilePickShapeDataSource = nullptr;
        rebuildSketchCommittedOverlay();
        return;
    }

    // 复用现有 VIS 管线，但对曲线：保留 lines，使用线框显示
    TopoDS_Compound newShape;
    BRep_Builder builder;
    builder.MakeCompound(newShape);
    for (const TopoDS_Shape& geom : geometries) {
        if (!geom.IsNull()) {
            builder.Add(newShape, geom);
        }
    }
    if (newShape.IsNull()) return;

    if (renderStateFor(history).profilePickActor) {
        removeSceneActor(renderStateFor(history).profilePickActor);
        renderStateFor(history).profilePickActor = nullptr;
    }
    renderStateFor(history).profilePickShapeWrapper = nullptr;
    renderStateFor(history).profilePickShapeDataSource = nullptr;

    geometryStateFor(history).occShape = newShape;
    history.type = SKETCH;

    ++shapeIDCounter;
    Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(newShape);
    shapeWrapper->SetId(shapeIDCounter);

    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
        vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    shapeDataSource->SetShape(shapeWrapper);
    shapeDataSource->Modified();
    shapeDataSource->Update();
    // 草图主显示与高亮统一使用 shapeDataSource，避免圆弧在 Mesher 管线下偶发缺失
    vtkPolyData* meshPolyData = vtkPolyData::SafeDownCast(shapeDataSource->GetOutput());

    vtkSmartPointer<vtkPolyData> polyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    {
        // 参照直线显示思路：草图主显示统一转为显式折线，避免圆弧在可视化管线下丢失
        vtkSmartPointer<vtkPoints> displayPoints = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> displayLines = vtkSmartPointer<vtkCellArray>::New();
        for (const TopoDS_Shape& geom : geometries) {
            if (geom.IsNull() || geom.ShapeType() != TopAbs_EDGE) continue;
            const TopoDS_Edge edge = TopoDS::Edge(geom);
            const int sampleCount = SketchGeometry::preferredEdgeSampleCount(edge);
            const QList<gp_Pnt> sampledPoints = SketchGeometry::sampleEdgePoints(edge, sampleCount);
            if (sampledPoints.size() < 2) continue;

            vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
            polyLine->GetPointIds()->SetNumberOfIds(sampledPoints.size());

            for (int i = 0; i < sampledPoints.size(); ++i) {
                const gp_Pnt& p = sampledPoints[i];
                const vtkIdType pid = displayPoints->InsertNextPoint(p.X(), p.Y(), p.Z());
                polyLine->GetPointIds()->SetId(i, pid);
            }
            displayLines->InsertNextCell(polyLine);
        }

        if (displayPoints->GetNumberOfPoints() > 0 && displayLines->GetNumberOfCells() > 0) {
            polyDataCopy->SetPoints(displayPoints);
            polyDataCopy->SetLines(displayLines);
        } else if (meshPolyData && meshPolyData->GetNumberOfPoints() > 0) {
            polyDataCopy->ShallowCopy(meshPolyData);
        } else {
            return;
        }
    }

    if (renderStateFor(history).actor) {
        vtkSmartPointer<vtkPolyDataMapper> newMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        newMapper->SetInputData(polyDataCopy);
        newMapper->ScalarVisibilityOff();
        // 草图线与参考平面共面：只用该 mapper 的相对线偏移，禁止改进程级全局参数
        //（全局 LineOffset 会污染实体轮廓深度测试）。
        newMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2.0, -2.0);
        ModelDisplayStyle::applySketchModelAppearance(
            renderStateFor(history).actor->GetProperty(),
            history.color);
        renderStateFor(history).actor->SetPickable(true);

        IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, renderStateFor(history).actor);
        // 注意：SetShapeSource 可能改写 mapper 连接，最后再强制回设显示 mapper
        renderStateFor(history).actor->SetMapper(newMapper);
        renderStateFor(history).actor->SetVisibility(1);
        if (renderer && !renderer->HasViewProp(renderStateFor(history).actor)) {
            renderer->AddActor(renderStateFor(history).actor);
        }
    }

    if (renderStateFor(history).highlightFilter) {
        renderStateFor(history).highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
        renderStateFor(history).highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");
    }

    renderStateFor(history).polyData = polyDataCopy;
    renderStateFor(history).shapeWrapper = shapeWrapper;
    renderStateFor(history).shapeDataSource = shapeDataSource;

    // 封闭草图保留一个不可见的面代理。它只用于面/轮廓点击，不改变草图的线框外观。
    TopoDS_Shape profileShape;
    if (renderer
        && SketchGeometry::buildPlanarProfile(
               newShape, activeSketchPlane_, profileShape, nullptr)
        && !profileShape.IsNull()) {
        ++shapeIDCounter;
        Handle(IVtkOCC_Shape) profileWrapper = new IVtkOCC_Shape(profileShape);
        profileWrapper->SetId(shapeIDCounter);
        vtkSmartPointer<IVtkTools_ShapeDataSource> profileSource =
            ModelShapePipeline::createShapeDataSource(profileWrapper);
        if (profileSource) {
            profileSource->Modified();
            profileSource->Update();

            vtkSmartPointer<vtkDataSetMapper> profileMapper =
                vtkSmartPointer<vtkDataSetMapper>::New();
            profileMapper->SetInputConnection(profileSource->GetOutputPort());
            profileMapper->ScalarVisibilityOff();

            vtkSmartPointer<vtkActor> profileActor =
                vtkSmartPointer<vtkActor>::New();
            profileActor->SetMapper(profileMapper);
            profileActor->SetPickable(true);
            profileActor->SetVisibility(true);
            profileActor->GetProperty()->SetRepresentationToSurface();
            profileActor->GetProperty()->SetOpacity(0.0);
            profileActor->GetProperty()->SetLighting(false);
            profileActor->GetProperty()->EdgeVisibilityOff();
            IVtkTools_ShapeObject::SetShapeSource(profileSource, profileActor);
            // SetShapeSource 只负责 VIS 拾取关联，最后固定回面代理的 mapper。
            profileActor->SetMapper(profileMapper);

            renderer->AddActor(profileActor);
            renderStateFor(history).profilePickActor = profileActor;
            renderStateFor(history).profilePickShapeWrapper = profileWrapper;
            renderStateFor(history).profilePickShapeDataSource = profileSource;
        }
    }

    rebuildSketchCommittedOverlay();
}
