// OCC 基本体创建、旋转复合体、网格化显示与形状校验（从 widget.cpp 拆出）
#include "widget.h"
#include "widget_mirror_globals.h"

#include <QDateTime>
#include <QMessageBox>
#include <QVTKOpenGLNativeWidget.h>

#include <BRep_Builder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkOCC_ShapeMesher.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>
#include <IVtkTools_DisplayModeFilter.hxx>
#include <IVtk_Types.hxx>
#include <IVtkVTK_ShapeData.hxx>

#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <vtkCamera.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellArrayIterator.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDataSetMapper.h>
#include <vtkFeatureEdges.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkMapper.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

namespace {
vtkSmartPointer<vtkPolyData> extractIVtkBoundaryEdgesFull(vtkPolyData* src);

gp_Dir resolvePrimitiveAxis(AxisDirection axisDirection,
                            bool axisReversed,
                            bool hasCustomVectorDir,
                            const gp_Dir& customVectorDir)
{
    gp_Dir axis(0, 0, 1);
    if (hasCustomVectorDir) {
        axis = customVectorDir;
    } else if (axisDirection == AxisDirection::X) {
        axis = gp_Dir(1, 0, 0);
    } else if (axisDirection == AxisDirection::Y) {
        axis = gp_Dir(0, 1, 0);
    }
    if (axisReversed) axis.Reverse();
    return axis;
}

bool modelNeedsBoundaryOutline(ModelType type)
{
    return type != SKETCH
        && type != DATUM_PLANE
        && type != DATUM_AXIS
        && type != WORK_CSYS
        && type != REFERENCE_CSYS;
}
} // namespace

vtkSmartPointer<IVtkTools_DisplayModeFilter>
Widget::configureSolidShapePipeline(IVtkTools_ShapeDataSource* source,
                                    vtkDataSetMapper* mapper,
                                    vtkActor* actor)
{
    if (!source || !mapper || !actor) return nullptr;
    vtkSmartPointer<IVtkTools_DisplayModeFilter> filter =
        vtkSmartPointer<IVtkTools_DisplayModeFilter>::New();
    filter->SetInputConnection(source->GetOutputPort());
    filter->SetDisplayMode(DM_Shading);
    filter->SetSmoothShading(true);
    mapper->SetInputConnection(filter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    actor->SetMapper(mapper);
    return filter;
}

void Widget::configureSolidMapperForBoundaryOutline(vtkMapper* mapper)
{
    if (!mapper) return;
    // factor 必须为 0：factor*DZ 会随相机拉远变大，导致背面轮廓“穿透变完整”。
    // 仅用很小的 units 后推面，减轻与可见棱的共面闪烁。
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0.0, 1.0);
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0.0, 0.0);
}

void Widget::configureOutlineMapperHiddenLine(vtkMapper* mapper)
{
    if (!mapper) return;
    mapper->ScalarVisibilityOff();
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0.0, 0.0);
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0.0, 0.0);
}

void Widget::styleModelBoundaryOutline(int index, bool selectedHighlight)
{
    if (index < 0 || index >= historyList.size()) return;
    ModelingHistory& rec = historyList[index];
    if (!rec.outlineActor || !modelNeedsBoundaryOutline(rec.type)) return;

    double r = 0.0, g = 0.0, b = 0.0;
    double lineWidth = 1.6;
    if (selectedHighlight) {
        r = 0.0; g = 1.0; b = 1.0;
        lineWidth = 2.8;
    } else if (featureOperationGhostMode_) {
        r = 1.0; g = 0.55; b = 0.12;
        lineWidth = 2.4;
    }

    vtkProperty* p = rec.outlineActor->GetProperty();
    p->SetRepresentationToWireframe();
    p->SetColor(r, g, b);
    p->SetLineWidth(lineWidth);
    p->SetLighting(false);
    p->SetAmbient(1.0);
    p->SetDiffuse(0.0);
    p->SetSpecular(0.0);
    p->RenderLinesAsTubesOff();
}

namespace {

bool isIVtkTopoEdgeType(vtkIdType mt)
{
    return mt == MT_FreeEdge
        || mt == MT_BoundaryEdge
        || mt == MT_SharedEdge
        || mt == MT_SeamEdge;
}

bool isFaceFrontFacing(const TopoDS_Face& face, const gp_Dir& viewDir)
{
    try {
        BRepAdaptor_Surface ads(face);
        const double u = 0.5 * (ads.FirstUParameter() + ads.LastUParameter());
        const double v = 0.5 * (ads.FirstVParameter() + ads.LastVParameter());
        gp_Pnt p;
        gp_Vec d1u, d1v;
        ads.D1(u, v, p, d1u, d1v);
        gp_Vec n = d1u.Crossed(d1v);
        if (n.SquareMagnitude() < 1e-18) {
            return true;
        }
        if (face.Orientation() == TopAbs_REVERSED) {
            n.Reverse();
        }
        // viewDir：相机指向场景；朝向相机的面法向与 viewDir 夹角 > 90°
        return gp_Dir(n).Dot(viewDir) < -1e-8;
    } catch (...) {
        return true;
    }
}

bool isEdgeHiddenByBackFaces(const TopoDS_Edge& edge,
                             const TopTools_IndexedDataMapOfShapeListOfShape& edgeFaceMap,
                             const gp_Dir& viewDir)
{
    if (!edgeFaceMap.Contains(edge)) {
        return false;
    }
    const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
    if (faces.IsEmpty()) {
        return false;
    }
    for (TopTools_ListIteratorOfListOfShape it(faces); it.More(); it.Next()) {
        if (it.Value().ShapeType() != TopAbs_FACE) {
            continue;
        }
        if (isFaceFrontFacing(TopoDS::Face(it.Value()), viewDir)) {
            return false; // 至少有一个朝前面 → 保留该棱
        }
    }
    return true; // 邻接面全朝后 → 当前视角下应不可见
}

/** 从完整 IVtk 网格提取全部拓扑边，并保留 SUBSHAPE_IDS（供视角过滤） */
vtkSmartPointer<vtkPolyData> extractIVtkBoundaryEdgesFull(vtkPolyData* src)
{
    vtkSmartPointer<vtkPolyData> out = vtkSmartPointer<vtkPolyData>::New();
    if (!src || src->GetNumberOfPoints() <= 0) {
        return out;
    }

    vtkCellArray* srcLines = src->GetLines();
    if (!srcLines || srcLines->GetNumberOfCells() <= 0) {
        return out;
    }

    // IVtk：MESH_TYPES 为 vtkShortArray，SUBSHAPE_IDS 为 vtkIdTypeArray
    vtkDataArray* typeArr = nullptr;
    vtkIdTypeArray* subIdArr = nullptr;
    if (src->GetCellData()) {
        typeArr = src->GetCellData()->GetArray(IVtkVTK_ShapeData::ARRNAME_MESH_TYPES());
        subIdArr = vtkIdTypeArray::SafeDownCast(
            src->GetCellData()->GetArray(IVtkVTK_ShapeData::ARRNAME_SUBSHAPE_IDS()));
        if (!subIdArr) {
            // 部分管线可能把名称写成普通字符串
            subIdArr = vtkIdTypeArray::SafeDownCast(src->GetCellData()->GetArray("SUBSHAPE_IDS"));
        }
    }

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->DeepCopy(src->GetPoints());
    out->SetPoints(pts);

    vtkSmartPointer<vtkCellArray> filtered = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkIdTypeArray> outSubIds = vtkSmartPointer<vtkIdTypeArray>::New();
    outSubIds->SetName(IVtkVTK_ShapeData::ARRNAME_SUBSHAPE_IDS());
    outSubIds->SetNumberOfComponents(1);

    const vtkIdType lineCellIdBase = src->GetNumberOfVerts();
    vtkIdType lineOrdinal = 0;
    bool anyTyped = false;

    vtkSmartPointer<vtkCellArrayIterator> it = vtk::TakeSmartPointer(srcLines->NewIterator());
    for (it->GoToFirstCell(); !it->IsDoneWithTraversal(); it->GoToNextCell(), ++lineOrdinal) {
        vtkIdType npts = 0;
        const vtkIdType* ptsIds = nullptr;
        it->GetCurrentCell(npts, ptsIds);
        if (npts < 2 || !ptsIds) continue;

        const vtkIdType globalId = lineCellIdBase + lineOrdinal;
        if (typeArr && globalId >= 0 && globalId < typeArr->GetNumberOfTuples()) {
            const vtkIdType mt = static_cast<vtkIdType>(typeArr->GetTuple1(globalId));
            if (!isIVtkTopoEdgeType(mt)) continue;
            anyTyped = true;
        }

        filtered->InsertNextCell(npts, ptsIds);
        if (subIdArr && globalId >= 0 && globalId < subIdArr->GetNumberOfTuples()) {
            outSubIds->InsertNextValue(subIdArr->GetValue(globalId));
        } else {
            outSubIds->InsertNextValue(-1);
        }
    }

    if (filtered->GetNumberOfCells() <= 0 && !anyTyped) {
        // 类型数组缺失时：保留全部线
        filtered->DeepCopy(srcLines);
        outSubIds->SetNumberOfTuples(0);
        if (subIdArr) {
            lineOrdinal = 0;
            it = vtk::TakeSmartPointer(srcLines->NewIterator());
            for (it->GoToFirstCell(); !it->IsDoneWithTraversal(); it->GoToNextCell(), ++lineOrdinal) {
                const vtkIdType globalId = lineCellIdBase + lineOrdinal;
                if (globalId >= 0 && globalId < subIdArr->GetNumberOfTuples()) {
                    outSubIds->InsertNextValue(subIdArr->GetValue(globalId));
                } else {
                    outSubIds->InsertNextValue(-1);
                }
            }
        }
    }

    if (filtered->GetNumberOfCells() > 0) {
        out->SetLines(filtered);
        if (outSubIds->GetNumberOfTuples() == filtered->GetNumberOfCells()) {
            out->GetCellData()->AddArray(outSubIds);
        }
    }
    return out;
}

/** 按视角剔除邻接面全朝后的棱（凸多面体 hidden-line） */
vtkSmartPointer<vtkPolyData> filterOutlineEdgesByView(vtkPolyData* fullEdges,
                                                      const Handle(IVtkOCC_Shape)& shapeWrapper,
                                                      const gp_Dir& viewDir)
{
    vtkSmartPointer<vtkPolyData> out = vtkSmartPointer<vtkPolyData>::New();
    if (!fullEdges || fullEdges->GetNumberOfCells() <= 0 || shapeWrapper.IsNull()) {
        if (fullEdges) out->DeepCopy(fullEdges);
        return out;
    }

    vtkCellArray* srcLines = fullEdges->GetLines();
    if (!srcLines || srcLines->GetNumberOfCells() <= 0) {
        out->DeepCopy(fullEdges);
        return out;
    }

    vtkIdTypeArray* subIdArr = nullptr;
    if (fullEdges->GetCellData()) {
        subIdArr = vtkIdTypeArray::SafeDownCast(
            fullEdges->GetCellData()->GetArray(IVtkVTK_ShapeData::ARRNAME_SUBSHAPE_IDS()));
        if (!subIdArr) {
            subIdArr = vtkIdTypeArray::SafeDownCast(fullEdges->GetCellData()->GetArray("SUBSHAPE_IDS"));
        }
    }
    if (!subIdArr || subIdArr->GetNumberOfTuples() < srcLines->GetNumberOfCells()) {
        out->DeepCopy(fullEdges);
        return out;
    }

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shapeWrapper->GetShape(), TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->DeepCopy(fullEdges->GetPoints());
    out->SetPoints(pts);

    vtkSmartPointer<vtkCellArray> filtered = vtkSmartPointer<vtkCellArray>::New();
    // 纯 Lines polydata：cellId 从 0 起
    vtkIdType cellId = 0;
    vtkSmartPointer<vtkCellArrayIterator> it = vtk::TakeSmartPointer(srcLines->NewIterator());
    for (it->GoToFirstCell(); !it->IsDoneWithTraversal(); it->GoToNextCell(), ++cellId) {
        vtkIdType npts = 0;
        const vtkIdType* ptsIds = nullptr;
        it->GetCurrentCell(npts, ptsIds);
        if (npts < 2 || !ptsIds) continue;

        bool keep = true;
        try {
            const IVtk_IdType sid = static_cast<IVtk_IdType>(subIdArr->GetValue(cellId));
            if (sid >= 0) {
                const TopoDS_Shape& sub = shapeWrapper->GetSubShape(sid);
                if (!sub.IsNull() && sub.ShapeType() == TopAbs_EDGE) {
                    if (isEdgeHiddenByBackFaces(TopoDS::Edge(sub), edgeFaceMap, viewDir)) {
                        keep = false;
                    }
                }
            }
        } catch (...) {
            keep = true;
        }
        if (keep) {
            filtered->InsertNextCell(npts, ptsIds);
        }
    }

    if (filtered->GetNumberOfCells() > 0) {
        out->SetLines(filtered);
    }
    return out;
}

vtkSmartPointer<vtkPolyData> extractFeatureEdgesFallback(vtkPolyData* polyData)
{
    vtkSmartPointer<vtkPolyData> out = vtkSmartPointer<vtkPolyData>::New();
    if (!polyData || polyData->GetNumberOfPoints() <= 0) return out;

    vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
    featureEdges->SetInputData(polyData);
    featureEdges->BoundaryEdgesOn();
    featureEdges->FeatureEdgesOn();
    featureEdges->ManifoldEdgesOff();
    featureEdges->NonManifoldEdgesOff();
    featureEdges->SetFeatureAngle(30.0);
    featureEdges->Update();
    if (featureEdges->GetOutput()) {
        out->DeepCopy(featureEdges->GetOutput());
    }
    return out;
}

vtkSmartPointer<vtkPolyData> buildOutlineSourcePolyData(const ModelingHistory& rec)
{
    // 优先：IVtk mesher 完整拓扑边（带 SUBSHAPE_IDS）
    if (!rec.shapeWrapper.IsNull()) {
        try {
            Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
            IVtkOCC_ShapeMesher mesher;
            mesher.Build(rec.shapeWrapper, shapeData);
            if (vtkPolyData* full = shapeData->getVtkPolyData()) {
                vtkSmartPointer<vtkPolyData> edges = extractIVtkBoundaryEdgesFull(full);
                if (edges && edges->GetNumberOfCells() > 0) {
                    return edges;
                }
            }
        } catch (...) {
        }
    }

    if (rec.shapeDataSource) {
        try {
            rec.shapeDataSource->Update();
            vtkSmartPointer<vtkPolyData> edges =
                extractIVtkBoundaryEdgesFull(rec.shapeDataSource->GetOutput());
            if (edges && edges->GetNumberOfCells() > 0) {
                return edges;
            }
        } catch (...) {
        }
    }

    return extractFeatureEdgesFallback(rec.polyData);
}

} // namespace

bool Widget::currentCameraViewDir(gp_Dir& outDir) const
{
    if (!renderer) return false;
    vtkCamera* cam = renderer->GetActiveCamera();
    if (!cam) return false;
    double pos[3] = {0, 0, 0};
    double fp[3] = {0, 0, 0};
    cam->GetPosition(pos);
    cam->GetFocalPoint(fp);
    gp_Vec look(fp[0] - pos[0], fp[1] - pos[1], fp[2] - pos[2]);
    if (look.SquareMagnitude() < 1e-18) return false;
    outDir = gp_Dir(look);
    return true;
}

void Widget::applyOutlinePolyData(int index, vtkSmartPointer<vtkPolyData> edges)
{
    Q_UNUSED(edges);
    if (index < 0 || index >= historyList.size() || !renderer) return;
    ModelingHistory& rec = historyList[index];
    if (rec.outlineActor) {
        renderer->RemoveActor(rec.outlineActor);
        rec.outlineActor = nullptr;
    }
    rec.outlinePolyData = nullptr;
    rec.outlineSourcePolyData = nullptr;
    styleModelBoundaryOutline(index, index == currentSelectedIndex);
}

void Widget::ensureModelBoundaryOutline(int index)
{
    if (!renderer || index < 0 || index >= historyList.size()) return;
    ModelingHistory& rec = historyList[index];
    if (!modelNeedsBoundaryOutline(rec.type) || !rec.shapeDataSource) return;
    if (!rec.outlineActor) {
        rec.edgeDisplayFilter = vtkSmartPointer<IVtkTools_DisplayModeFilter>::New();
        rec.edgeDisplayFilter->SetInputConnection(rec.shapeDataSource->GetOutputPort());
        IVtk_IdTypeMap edgeTypes;
        edgeTypes.Add(MT_FreeEdge);
        edgeTypes.Add(MT_BoundaryEdge);
        edgeTypes.Add(MT_SharedEdge);
        edgeTypes.Add(MT_SeamEdge);
        rec.edgeDisplayFilter->SetMeshTypesForMode(DM_Wireframe, edgeTypes);
        rec.edgeDisplayFilter->SetDisplaySharedVertices(false);
        rec.edgeDisplayFilter->SetDisplayMode(DM_Wireframe);

        vtkSmartPointer<vtkDataSetMapper> edgeMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        edgeMapper->SetInputConnection(rec.edgeDisplayFilter->GetOutputPort());
        edgeMapper->ScalarVisibilityOff();

        rec.outlineActor = vtkSmartPointer<vtkActor>::New();
        rec.outlineActor->SetMapper(edgeMapper);
        rec.outlineActor->SetPickable(false);
        rec.outlineActor->SetVisibility(rec.actor ? rec.actor->GetVisibility() : 1);
        renderer->AddActor(rec.outlineActor);
    }
    styleModelBoundaryOutline(index, index == currentSelectedIndex);
}

void Widget::updateModelBoundaryOutlinesForView()
{
    // The wireframe filter follows the shared shape source; no camera rebuild is needed.
}

void Widget::refreshAllModelBoundaryOutlines()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].outlineActor && renderer) {
            renderer->RemoveActor(historyList[i].outlineActor);
            historyList[i].outlineActor = nullptr;
        }
        historyList[i].outlinePolyData = nullptr;
        historyList[i].outlineSourcePolyData = nullptr;
        ensureModelBoundaryOutline(i);
    }
}

bool Widget::buildRevolutionCompound(const QList<ExtrusionFaceSelection>& selections,
                                      const gp_Ax1& axis,
                                      double angleRad,
                                      TopoDS_Compound& outCompound,
                                      double startAngleRad)
{
    BRep_Builder builder;
    builder.MakeCompound(outCompound);

    gp_Trsf startRot;
    const bool needStart = std::abs(startAngleRad) > 1e-12;
    if (needStart) {
        startRot.SetRotation(axis, startAngleRad);
    }

    bool hasValid = false;
    for (const ExtrusionFaceSelection& sel : selections) {
        if (sel.shape.IsNull()) continue;

        TopoDS_Shape profile = sel.shape;
        if (needStart) {
            BRepBuilderAPI_Transform mover(profile, startRot, true);
            profile = mover.Shape();
        }

        BRepPrimAPI_MakeRevol revol(profile, axis, angleRad);
        if (!revol.IsDone()) continue;

        builder.Add(outCompound, revol.Shape());
        hasValid = true;
    }
    return hasValid;
}

// 使用OpenCASCADE创建长方体（以指定原点为角点）
TopoDS_Shape Widget::createCuboidShape(double length, double width, double height,
                                       bool hasOrigin, double originX, double originY, double originZ,
                                       AxisDirection axisDirection,
                                       bool axisReversed,
                                       bool hasCustomVectorDir,
                                       const gp_Dir& customVectorDir)
{
    gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    gp_Dir axis = resolvePrimitiveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeBox(axisSystem, length, width, height).Shape();
}

// 使用OpenCASCADE创建圆柱体（轴向支持X/Y/Z）
TopoDS_Shape Widget::createCylinderShape(double radius, double height,
                                         bool hasOrigin, double originX, double originY, double originZ,
                                         AxisDirection axisDirection,
                                         bool axisReversed,
                                         bool hasCustomVectorDir,
                                         const gp_Dir& customVectorDir)
{
    gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    gp_Dir axis = resolvePrimitiveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeCylinder(axisSystem, radius, height).Shape();
}

// 使用OpenCASCADE创建圆锥体（圆台）
TopoDS_Shape Widget::createConeShape(double radius1, double radius2, double height,
                                     bool hasOrigin, double originX, double originY, double originZ,
                                     AxisDirection axisDirection,
                                     bool axisReversed,
                                     bool hasCustomVectorDir,
                                     const gp_Dir& customVectorDir)
{
    // OpenCASCADE的圆锥体需要两个半径（底部和顶部）
    // radius1: 底部半径, radius2: 顶部半径
    // 当 radius2 = 0 时，创建标准圆锥；否则创建圆台
    gp_Pnt origin = hasOrigin ? gp_Pnt(originX, originY, originZ) : gp_Pnt(0, 0, 0);
    gp_Dir axis = resolvePrimitiveAxis(axisDirection, axisReversed, hasCustomVectorDir, customVectorDir);
    gp_Ax2 axisSystem(origin, axis);
    return BRepPrimAPI_MakeCone(axisSystem, radius1, radius2, height).Shape();
}

// 使用OpenCASCADE创建球体
TopoDS_Shape Widget::createSphereShape(double radius,
                                       bool hasOrigin, double originX, double originY, double originZ)
{
    if (hasOrigin) {
        gp_Pnt center(originX, originY, originZ);
        return BRepPrimAPI_MakeSphere(center, radius).Shape();
    }
    return BRepPrimAPI_MakeSphere(radius).Shape();
}

// 通用的显示函数
void Widget::displayOccShape(const TopoDS_Shape& shape, const QString& name,
                             ModelType type, const QColor& color,
                             double param1, double param2, double param3,
                             bool hasOrigin /*= false*/,
                             double originX /*= 0.0*/, double originY /*= 0.0*/, double originZ /*= 0.0*/,
                             AxisDirection axisDirection /*= AxisDirection::Z*/,
                             bool axisReversed /*= false*/,
                             bool hasCustomVectorDir /*= false*/,
                             const gp_Dir& customVectorDir /*= gp_Dir(0,0,1)*/)
{
    // 统一在主上下文写入建模结果，避免“只在子窗口出现”的分叉状态。
    QObject* previousMirrorKey = nullptr;
    if (vtkWidget && g_mirrorRenderContextMap.contains(this)) {
        auto& map = g_mirrorRenderContextMap[this];
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it.value().vtkWidget == vtkWidget) {
                previousMirrorKey = it.key();
                break;
            }
        }
    }
    if (previousMirrorKey && renderer) {
        syncMainCameraFromWindowRenderer(renderer);
    }
    activateMainRenderContext();

    if (shape.IsNull()) {
        QMessageBox::warning(dialogParentWidget(), "错误", "创建的形状无效！");
        if (previousMirrorKey) {
            activateMirrorRenderContext(previousMirrorKey);
            if (vtkWidget) vtkWidget->setFocus();
        }
        return;
    }

    // 为形状分配唯一ID
    ++shapeIDCounter;

        // Step 1: 先对OCC形状进行网格离散化（提高质量）
        double meshDeflection = (type == BOOLEAN_RESULT) ? 0.03 : 0.05;
        BRepMesh_IncrementalMesh mesh(shape, meshDeflection, Standard_False, 0.3, Standard_True);
        mesh.Perform();

        // Step 2: 创建VIS形状包装器
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(shapeIDCounter);

        // Step 3: 手动构建 PolyData
        Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
        IVtkOCC_ShapeMesher mesher;
        mesher.Build(shapeWrapper, shapeData);
        vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
        
        // 清理线条和顶点数据，只保留多边形（面）
        if (meshPolyData) {
            meshPolyData->SetLines(nullptr);  // 移除线条
            meshPolyData->SetVerts(nullptr);  // 移除顶点
        }

        // Step 4: 创建 ShapeDataSource（仅用于拾取功能）
        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDataSource->SetShape(shapeWrapper);
        // 更新
        shapeDataSource->Modified();
        shapeDataSource->Update();

        // Step 5: 创建主映射器
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        configureSolidMapperForBoundaryOutline(mapper);

        // Step 6: 创建 Actor 并设置渲染属性
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetPickable(true);
        
        // 设置渲染属性（配合场景三点光）
        applySolidActorMaterial(actor->GetProperty());
        actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
        vtkSmartPointer<IVtkTools_DisplayModeFilter> solidDisplayFilter =
            configureSolidShapePipeline(shapeDataSource, mapper, actor);

        // Step 7: 绑定VIS数据源到Actor（用于拾取）
        IVtkTools_ShapeObject::SetShapeSource(shapeDataSource, actor);

        // Step 8: 创建 VIS 高亮管线（与 vis_picker_example：SubPolyDataFilter 接 ShapeDataSource 输出）
        vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter =
            vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
        highlightFilter->SetInputConnection(shapeDataSource->GetOutputPort());
        highlightFilter->SetIdsArrayName("SUBSHAPE_IDS");

        vtkSmartPointer<vtkPolyDataMapper> highlightMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        highlightMapper->SetInputConnection(highlightFilter->GetOutputPort());  // Pipeline 连接
        highlightMapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> highlightActor = vtkSmartPointer<vtkActor>::New();
        highlightActor->SetMapper(highlightMapper);
        highlightActor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 黄色高亮
        highlightActor->GetProperty()->SetOpacity(0.6);  // 半透明
        
        // 设置为面渲染，不显示边缘
        highlightActor->GetProperty()->SetRepresentationToSurface();  // 面渲染
        highlightActor->GetProperty()->EdgeVisibilityOff();  // 不显示边缘
        highlightActor->GetProperty()->SetLighting(true);  // 启用光照
        highlightActor->SetPickable(false);  // 避免 VIS 高亮层抢走整模拾取
        
        highlightActor->SetVisibility(false);

        // Step 9: 添加到渲染器
        renderer->AddActor(actor);
        addAppearanceActor(highlightActor);

        // Step 10: 保存 PolyData
        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->ShallowCopy(meshPolyData);

        // Step 11: 创建历史记录
        ModelingHistory record;
        record.type = type;
        record.name = name;
        record.timestamp = QDateTime::currentDateTime();
        record.actor = actor;
        record.color = color;
        record.param1 = param1;
        record.param2 = param2;
        record.param3 = param3;
        record.polyData = polyData;
        record.occShape = shape;
        record.shapeWrapper = shapeWrapper;  // 保存 Handle，防止被释放
        record.shapeDataSource = shapeDataSource;  // 保存数据源，防止被释放
        record.solidDisplayFilter = solidDisplayFilter;
        record.highlightFilter = highlightFilter;
        record.highlightActor = highlightActor;
        record.outlineActor = nullptr;
        record.outlinePolyData = nullptr;
        record.outlineSourcePolyData = nullptr;
        record.hasOrigin = hasOrigin;
        record.originX = originX;
        record.originY = originY;
        record.originZ = originZ;
        record.axisDirection = axisDirection;
        record.axisReversed = axisReversed;
        record.hasCustomVectorDir = hasCustomVectorDir;
        record.customVectorDir = customVectorDir;
        record.hasCustomVectorDir = hasCustomVectorDir;
        record.customVectorDir = customVectorDir;

        historyList.append(record);
        ensureModelBoundaryOutline(historyList.size() - 1);
        markDocumentModified(true);

        // Step 12: 渲染
        renderer->ResetCamera();
        refreshCameraClippingRange();
        refreshOverlayScreenScale();
        vtkWidget->renderWindow()->Render();
        updateHistoryList();
        updateFeatureTree();  // 更新特征树

        // 刷新拾取绑定：不重建 shapePicker，与 prepareShapePickerBindingsForCurrentContext 保持一致
        if (shapePicker && renderer) {
            shapePicker->SetRenderer(renderer);
            shapePicker->SetTolerance(0.05);
            prepareShapePickerBindingsForCurrentContext();
            for (int i = 0; i < historyList.size(); ++i) {
                if (historyList[i].shapeDataSource) {
                    historyList[i].shapeDataSource->Modified();
                    historyList[i].shapeDataSource->Update();
                }
            }
        }

        // 将主上下文的最新结果同步到所有子窗口，再恢复原交互上下文。
        syncMirrorWindows();
        if (previousMirrorKey) {
            activateMirrorRenderContext(previousMirrorKey);
            if (vtkWidget) vtkWidget->setFocus();
        }
}

// 检查形状是否适合操作
bool Widget::isValidShapeForOperation(int index)
{
    if (index < 0 || index >= historyList.size()) return false;

    // 检查多边形数据是否有效
    vtkPolyData* polyData = historyList[index].polyData;
    if (!polyData || polyData->GetNumberOfPoints() == 0) return false;

    return true;
}
