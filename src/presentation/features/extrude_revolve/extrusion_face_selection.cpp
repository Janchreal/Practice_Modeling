// Updated at 2026-01-10 to fix compilation errors
#include "main_window.h"
#include "presentation/dialogs/extrude_revolve/extrusion_dialog.h"
#include "rendering/model/model_display_style.h"
#include "rendering/pipeline/model_shape_pipeline.h"
#include "geometry/sketch/sketch_geometry.h"
#include <QSet>
#include <QScopeGuard>
#include <QStatusBar>
#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkRenderWindowInteractor.h>
#include <IVtkTools_ShapePicker.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>
#include <IVtk_Types.hxx>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkMapper.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkIdTypeArray.h>
#include <vtkPolyData.h>
#include <vtkIdList.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkExtractSelection.h>
#include <vtkNew.h>
#include <vtkFeatureEdges.h>
#include <TColStd_MapIteratorOfPackedMapOfInteger.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Pln.hxx>
#include <Standard_Failure.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <string>
#include "selection_geometry.h"

namespace {

bool shapeContainsEquivalentEdge(const TopoDS_Shape& shape, const TopoDS_Edge& edge)
{
    if (shape.IsNull() || edge.IsNull()) {
        return false;
    }
    for (TopExp_Explorer edgeExp(shape, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
        const TopoDS_Shape current = edgeExp.Current();
        if (!current.IsNull()
            && current.ShapeType() == TopAbs_EDGE
            && SketchGeometry::sketchEdgesEquivalent(TopoDS::Edge(current), edge)) {
            return true;
        }
    }
    return false;
}

bool firstEdgeOfShape(const TopoDS_Shape& shape, TopoDS_Edge& outEdge)
{
    outEdge = TopoDS_Edge();
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_EDGE) {
        outEdge = TopoDS::Edge(shape);
        return !outEdge.IsNull();
    }
    for (TopExp_Explorer edgeExp(shape, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
        const TopoDS_Shape current = edgeExp.Current();
        if (!current.IsNull() && current.ShapeType() == TopAbs_EDGE) {
            outEdge = TopoDS::Edge(current);
            return !outEdge.IsNull();
        }
    }
    return false;
}

int profileIndexForPickedPart(const TopoDS_Shape& profileRoot,
                              const TopoDS_Shape& pickedPart)
{
    if (profileRoot.IsNull() || pickedPart.IsNull()) {
        return -1;
    }

    TopoDS_Edge pickedEdge;
    const bool hasPickedEdge = firstEdgeOfShape(pickedPart, pickedEdge);
    int index = 0;
    for (TopExp_Explorer faceExp(profileRoot, TopAbs_FACE); faceExp.More(); faceExp.Next(), ++index) {
        const TopoDS_Shape face = faceExp.Current();
        if (face.IsNull() || face.ShapeType() != TopAbs_FACE) {
            continue;
        }
        if (pickedPart.ShapeType() == TopAbs_FACE && face.IsSame(pickedPart)) {
            return index;
        }
        if (hasPickedEdge && shapeContainsEquivalentEdge(face, pickedEdge)) {
            return index;
        }
    }

    if (index == 0 && profileRoot.ShapeType() == TopAbs_FACE) {
        if (profileRoot.IsSame(pickedPart)) {
            return 0;
        }
        if (hasPickedEdge && shapeContainsEquivalentEdge(profileRoot, pickedEdge)) {
            return 0;
        }
    }

    return -1;
}

} // namespace

// 处理拉伸界面的面悬停
void Widget::handleExtrusionFaceHover(int x, int y)
{
    try {
        if (!vtkWidget || !renderer || !shapePicker) return;
        if (featureResultPreviewActive_) return;

        // 手柄/预览无 OCC ShapeSource：必须临时不可拾取，否则 IVtk ShapePicker 易崩溃
        setFeatureGizmoActorsPickable(false);

        refreshShapePickerBindingsForCurrentContext(0.05, false);
        
        IVtkTools_ShapePicker* picker = shapePicker;

        IVtk_IdType subShapeId = -1;
        int modelIndex = -1;
        bool found = false;
        Handle(IVtkOCC_Shape) pickedShapeWrapper;
        const auto findHistoryForShapeId = [this](
            IVtk_IdType shapeId,
            Handle(IVtkOCC_Shape)& outWrapper) -> int {
            outWrapper = nullptr;
            for (int i = 0; i < historyList.size(); ++i) {
                const ModelRenderState& state = renderStateFor(historyList[i]);
                if (!state.shapeWrapper.IsNull() && state.shapeWrapper->GetId() == shapeId) {
                    outWrapper = state.shapeWrapper;
                    return i;
                }
                if (!state.profilePickShapeWrapper.IsNull()
                    && state.profilePickShapeWrapper->GetId() == shapeId) {
                    outWrapper = state.profilePickShapeWrapper;
                    return i;
                }
            }
            return -1;
        };
        // 根据选择模式决定拾取面还是边
        if (currentSelectionMode == FaceSelection) {
            // 面选择模式：只拾取面
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                }
            }
        } else if (currentSelectionMode == EdgeSelection) {
            // 边选择模式：拾取边
            picker->SetSelectionMode(SM_Edge);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                }
            }
        } else {
            // 默认模式（ExtrusionSelection）：优先面，然后边
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0);

            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                }
            }

            // 如果没找到面，尝试拾取边
            if (!found || subShapeId == -1) {
                picker->SetSelectionMode(SM_Edge);
                picker->Pick(x, y, 0);
                ids = picker->GetPickedShapesIds();
                if (!ids.IsEmpty()) {
                    IVtk_IdType shapeId = ids.First();
                    modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                    found = modelIndex >= 0;
                    if (found) {
                        subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                    }
                }
            }
        }

        // 更新悬停状态
        if (found) {
            // 验证模型索引和子形状ID是否有效
            if (modelIndex >= 0 && modelIndex < historyList.size()) {
                // 检查模型是否可见，如果隐藏则忽略拾取
                ModelingHistory& record = historyList[modelIndex];
                if (renderStateFor(record).actor && renderStateFor(record).actor->GetVisibility() == 0) {
                    // 模型被隐藏，忽略这次拾取
                    if (hasHoveredFace) {
                        hasHoveredFace = false;
                        updateExtrusionHoverHighlight();
                    }
                    return;
                }
                
                // 获取悬停形状的信息并验证形状类型
                try {
                    if (!pickedShapeWrapper.IsNull()) {
                        TopoDS_Shape hoveredShape;
                        if (subShapeId != -1) {
                            try {
                                hoveredShape = pickedShapeWrapper->GetSubShape(subShapeId);
                            } catch (Standard_Failure&) {
                                hoveredShape = TopoDS_Shape();
                            } catch (...) {
                                hoveredShape = TopoDS_Shape();
                            }
                        } else if (currentSelectionMode != FaceSelection) {
                            TopoDS_Wire fallbackWire;
                            if (SelectionGeometry::singleWireFromShape(geometryStateFor(record).occShape, fallbackWire)) {
                                hoveredShape = fallbackWire;
                            }
                        }
                        if (!hoveredShape.IsNull()) {
                            // 验证拾取到的形状类型是否符合选择模式
                            bool isValid = false;
                            const bool isSketchProfilePick =
                                record.type == SKETCH
                                && !renderStateFor(record).profilePickShapeWrapper.IsNull()
                                && pickedShapeWrapper->GetId()
                                    == renderStateFor(record).profilePickShapeWrapper->GetId();
                            if (isSketchProfilePick) {
                                isValid = hoveredShape.ShapeType() == TopAbs_FACE
                                    || hoveredShape.ShapeType() == TopAbs_EDGE
                                    || hoveredShape.ShapeType() == TopAbs_WIRE;
                            } else if (currentSelectionMode == FaceSelection) {
                                // 面选择模式：只接受面
                                isValid = (hoveredShape.ShapeType() == TopAbs_FACE);
                            } else if (currentSelectionMode == EdgeSelection) {
                                // 边选择模式（与旋转曲线选择一致）：仅高亮拾取到的边
                                isValid = (hoveredShape.ShapeType() == TopAbs_EDGE);
                            } else {
                                // 默认模式：接受面、边和线框
                                isValid = (hoveredShape.ShapeType() == TopAbs_FACE || 
                                          hoveredShape.ShapeType() == TopAbs_EDGE ||
                                          hoveredShape.ShapeType() == TopAbs_WIRE);
                            }
                            
                            if (isValid) {
                                // 检查是否与当前悬停的相同
                                const bool sameHover =
                                    hasHoveredFace &&
                                    hoveredFace.modelIndex == modelIndex &&
                                    hoveredFace.shapeType == hoveredShape.ShapeType() &&
                                    !hoveredFace.shape.IsNull() &&
                                    hoveredFace.shape.IsSame(hoveredShape);
                                if (!sameHover) {
                                    hasHoveredFace = true;
                                    hoveredFace.modelIndex = modelIndex;
                                    hoveredFace.subShapeId = subShapeId;
                                    hoveredFace.shape = hoveredShape;
                                    hoveredFace.shapeType = hoveredShape.ShapeType();
                                    // 更新悬停高亮
                                    updateExtrusionHoverHighlight();
                                }
                            } else {
                                // 形状类型不符合选择模式，清除悬停高亮
                                if (hasHoveredFace) {
                                    hasHoveredFace = false;
                                    updateExtrusionHoverHighlight();
                                }
                            }
                        }
                    }
                } catch (Standard_Failure& e) {
                    (void)e;
                    if (hasHoveredFace) {
                        hasHoveredFace = false;
                        updateExtrusionHoverHighlight();
                    }
                } catch (...) {
                    if (hasHoveredFace) {
                        hasHoveredFace = false;
                        updateExtrusionHoverHighlight();
                    }
                }
            }
        } else {
            // 没有找到，清除悬停高亮
            if (hasHoveredFace) {
                hasHoveredFace = false;
                updateExtrusionHoverHighlight();
            }
        }
    } catch (Standard_Failure& e) {
        (void)e;
    } catch (const std::exception& e) {
        (void)e;
    } catch (...) {
    }
    setFeatureGizmoActorsPickable(true);
}

// 处理拉伸界面的面点击拾取
void Widget::handleExtrusionFaceClick(int x, int y)
{
    try {
        if (!vtkWidget || !renderer || !shapePicker) return;

        setFeatureGizmoActorsPickable(false);
        const auto restoreFeatureGizmoPickable = qScopeGuard([this]() {
            setFeatureGizmoActorsPickable(true);
        });

        refreshShapePickerBindingsForCurrentContext(0.05, false);
        
        IVtkTools_ShapePicker* picker = shapePicker;

        IVtk_IdType subShapeId = -1;
        int modelIndex = -1;
        bool found = false;
        Handle(IVtkOCC_Shape) pickedShapeWrapper;
        const auto findHistoryForShapeId = [this](
            IVtk_IdType shapeId,
            Handle(IVtkOCC_Shape)& outWrapper) -> int {
            outWrapper = nullptr;
            for (int i = 0; i < historyList.size(); ++i) {
                const ModelRenderState& state = renderStateFor(historyList[i]);
                if (!state.shapeWrapper.IsNull() && state.shapeWrapper->GetId() == shapeId) {
                    outWrapper = state.shapeWrapper;
                    return i;
                }
                if (!state.profilePickShapeWrapper.IsNull()
                    && state.profilePickShapeWrapper->GetId() == shapeId) {
                    outWrapper = state.profilePickShapeWrapper;
                    return i;
                }
            }
            return -1;
        };

        // 根据选择模式决定拾取面还是边
        if (currentSelectionMode == FaceSelection) {
            // 面选择模式：只拾取面
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                }
            }
        } else if (currentSelectionMode == EdgeSelection) {
            // 边选择模式：只拾取边
            picker->SetSelectionMode(SM_Edge);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                    
                    // 验证拾取到的确实是边，而不是面
                    if (subShapeId != -1) {
                        ModelingHistory& record = historyList[modelIndex];
                        if (!pickedShapeWrapper.IsNull()) {
                            TopoDS_Shape pickedShape = pickedShapeWrapper->GetSubShape(subShapeId);
                            if (!pickedShape.IsNull()) {
                                if (record.type != SKETCH
                                    && pickedShape.ShapeType() != TopAbs_EDGE) {
                                    // 拾取到的不是边，清除结果
                                    found = false;
                                    subShapeId = -1;
                                } else {
                                }
                            }
                        }
                    }
                }
            } else {
            }
        } else {
            // 默认模式（ExtrusionSelection）：优先面，然后边
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0, renderer);

            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                found = modelIndex >= 0;
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                }
            }

            // 尝试拾取边（用于线框/线拾取）
            if (!found || subShapeId == -1) {
                picker->SetSelectionMode(SM_Edge);
                picker->Pick(x, y, 0, renderer);
                ids = picker->GetPickedShapesIds();
                if (!ids.IsEmpty()) {
                    IVtk_IdType shapeId = ids.First();
                    modelIndex = findHistoryForShapeId(shapeId, pickedShapeWrapper);
                    found = modelIndex >= 0;
                    if (found) {
                        subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                    }
                }
            }
        }

        if (found) {
            // 检查模型是否可见，如果隐藏则忽略拾取
            if (modelIndex >= 0 && modelIndex < historyList.size()) {
                ModelingHistory& record = historyList[modelIndex];
                if (renderStateFor(record).actor && renderStateFor(record).actor->GetVisibility() == 0) {
                    // 模型被隐藏，忽略这次拾取
                    return;
                }
            }
            
            // 执行拾取
                try {
                    ModelingHistory& record = historyList[modelIndex];
                    if (pickedShapeWrapper.IsNull()) return;

                    TopoDS_Shape mainShape = geometryStateFor(record).occShape;

                    const bool profilePickingActive = extrusionDialog || revolveDialog;
                    const bool pickedSketchProfile =
                        record.type == SKETCH
                        && profilePickingActive
                        && !renderStateFor(record).profilePickShapeWrapper.IsNull()
                        && pickedShapeWrapper->GetId()
                            == renderStateFor(record).profilePickShapeWrapper->GetId();

                    TopoDS_Shape selectedSketchProfile;
                    int sketchContourIndex = -1;
                    if (record.type == SKETCH && profilePickingActive) {
                        const gp_Pln sketchPlane(record.recipe.sketch.planeOrigin,
                                                 record.recipe.sketch.planeNormal);
                        TopoDS_Shape pickedPart;
                        if (subShapeId != -1) {
                            try {
                                pickedPart = pickedShapeWrapper->GetSubShape(subShapeId);
                            } catch (Standard_Failure&) {
                                pickedPart = TopoDS_Shape();
                            } catch (...) {
                                pickedPart = TopoDS_Shape();
                            }
                        } else if (pickedSketchProfile) {
                            pickedPart = pickedShapeWrapper->GetShape();
                        }

                        std::string profileError;
                        if (pickedSketchProfile && !pickedPart.IsNull()) {
                            sketchContourIndex = profileIndexForPickedPart(
                                pickedShapeWrapper->GetShape(), pickedPart);
                            if (sketchContourIndex >= 0) {
                                if (!SketchGeometry::buildPlanarProfileAt(
                                        mainShape, sketchPlane, sketchContourIndex,
                                        selectedSketchProfile, &profileError)) {
                                    selectedSketchProfile = TopoDS_Shape();
                                }
                            }
                        } else if (!pickedPart.IsNull()) {
                            TopoDS_Edge pickedEdge;
                            if (firstEdgeOfShape(pickedPart, pickedEdge)) {
                                SketchGeometry::findClosedProfileContainingEdge(
                                    mainShape, sketchPlane, pickedEdge,
                                    selectedSketchProfile, &sketchContourIndex,
                                    &profileError);
                            }
                        }

                        if (selectedSketchProfile.IsNull() && pickedSketchProfile) {
                            if (statusBar()) {
                                statusBar()->showMessage(
                                    tr("无法识别选中的草图轮廓：%1")
                                        .arg(QString::fromStdString(profileError)),
                                    3500);
                            }
                            return;
                        }
                    }

                    if (!selectedSketchProfile.IsNull()) {
                        ExtrusionFaceSelection selection;
                        selection.modelIndex = modelIndex;
                        selection.subShapeId = subShapeId;
                        selection.shape = selectedSketchProfile;
                        selection.shapeType = selectedSketchProfile.ShapeType();
                        selection.isSketchContour = true;
                        selection.sketchContourIndex = sketchContourIndex;

                        int sameShapeIdx = -1;
                        for (int i = 0; i < extrusionSelectedFaces.size(); ++i) {
                            const ExtrusionFaceSelection& existing = extrusionSelectedFaces[i];
                            if (existing.modelIndex == selection.modelIndex
                                && existing.isSketchContour
                                && existing.sketchContourIndex == selection.sketchContourIndex) {
                                sameShapeIdx = i;
                                break;
                            }
                        }
                        if (sameShapeIdx >= 0) {
                            extrusionSelectedFaces.removeAt(sameShapeIdx);
                        } else {
                            extrusionSelectedFaces.append(selection);
                        }
                        extrusionSelectedIndices.clear();
                        updateExtrusionFaceHighlight();
                        if (extrusionDialog) {
                            extrusionDialog->setSelectedGeometryCount(
                                extrusionSelectedFaces.size());
                        }
                        if (revolveDialog) {
                            revolveDialog->setSelectedGeometryCount(
                                extrusionSelectedFaces.size());
                        }
                        const int epoch = extrudeRevolveSelectionEpoch_;
                        QTimer::singleShot(0, this, [this, epoch]() {
                            if (epoch != extrudeRevolveSelectionEpoch_) return;
                            if (!extrusionDialog && !revolveDialog) return;
                            try {
                                applyAutoVectorFromSelection();
                                if (extrusionDialog) {
                                    updateExtrusionHandles();
                                    refreshExtrusionLivePreview();
                                }
                                if (revolveDialog) {
                                    updateRevolveHandles();
                                    refreshRevolveLivePreview();
                                }
                            } catch (Standard_Failure&) {
                            } catch (...) {
                            }
                        });
                        return;
                    }

                    TopoDS_Shape selectedSubShape;
                    if (subShapeId != -1) {
                        try {
                            selectedSubShape = pickedShapeWrapper->GetSubShape(subShapeId);
                        } catch (Standard_Failure&) {
                            return;
                        } catch (...) {
                            return;
                        }
                    } else if (currentSelectionMode != FaceSelection) {
                        TopoDS_Wire fallbackWire;
                        if (SelectionGeometry::singleWireFromShape(mainShape, fallbackWire)) {
                            selectedSubShape = fallbackWire;
                        }
                    }

                    if (selectedSubShape.IsNull()) return;

                    // 验证拾取到的形状类型是否符合选择模式
                    if (currentSelectionMode == FaceSelection) {
                        // 面选择模式：只接受面
                        if (selectedSubShape.ShapeType() != TopAbs_FACE) {
                            return;
                        }
                    } else if (currentSelectionMode == EdgeSelection) {
                        // 边选择模式（与旋转曲线选择一致）：只接受单条边
                        if (selectedSubShape.ShapeType() != TopAbs_EDGE) {
                            return;
                        }
                    }
                    
                    // 如果拾取的是边，且不是边选择模式，尝试寻找包含它的线框（Wire）
                    if (selectedSubShape.ShapeType() == TopAbs_EDGE && currentSelectionMode != EdgeSelection) {
                        TopoDS_Wire wire = SelectionGeometry::wireContainingEdge(mainShape, TopoDS::Edge(selectedSubShape));
                        if (!wire.IsNull()) {
                            selectedSubShape = wire;
                        }
                    }

                    ExtrusionFaceSelection selection;
                    selection.modelIndex = modelIndex;
                    selection.subShapeId = subShapeId;
                    selection.shape = selectedSubShape;
                    selection.shapeType = selectedSubShape.ShapeType();

                    // 按形状语义去重：wire 按 IsSame 对比，避免同一线框被重复加入
                    int sameShapeIdx = -1;
                    for (int i = 0; i < extrusionSelectedFaces.size(); ++i) {
                        const ExtrusionFaceSelection& ex = extrusionSelectedFaces[i];
                        if (ex.modelIndex != selection.modelIndex) continue;
                        if (selection.isSketchContour && ex.isSketchContour) {
                            if (ex.sketchContourIndex == selection.sketchContourIndex) {
                                sameShapeIdx = i;
                                break;
                            }
                        } else if (selection.shapeType == TopAbs_WIRE && ex.shapeType == TopAbs_WIRE) {
                            if (!ex.shape.IsNull() && ex.shape.IsSame(selection.shape)) {
                                sameShapeIdx = i;
                                break;
                            }
                        } else if (!selection.isSketchContour && !ex.isSketchContour
                                   && ex.subShapeId == selection.subShapeId) {
                            sameShapeIdx = i;
                            break;
                        }
                    }
                    if (sameShapeIdx >= 0) {
                        extrusionSelectedFaces.removeAt(sameShapeIdx);
                    } else {
                        extrusionSelectedFaces.append(selection);
                    }
                    
                    // 触发渲染更新
                    updateExtrusionFaceHighlight();
                    
                    // 更新对话框中的选中计数
                    if (extrusionDialog) {
                        extrusionDialog->setSelectedGeometryCount(extrusionSelectedFaces.size());
                    }
                    if (revolveDialog) {
                        revolveDialog->setSelectedGeometryCount(extrusionSelectedFaces.size());
                    }
                    // 推迟矢量/手柄/预览：避免在 shapePicker 回调栈内 Render/OCC 重入崩溃
                    const int epoch = extrudeRevolveSelectionEpoch_;
                    QTimer::singleShot(0, this, [this, epoch]() {
                        if (epoch != extrudeRevolveSelectionEpoch_) return;
                        if (!extrusionDialog && !revolveDialog) return;
                        try {
                            applyAutoVectorFromSelection();
                            if (extrusionDialog) {
                                updateExtrusionHandles();
                                refreshExtrusionLivePreview();
                            }
                            if (revolveDialog) {
                                updateRevolveHandles();
                                refreshRevolveLivePreview();
                            }
                        } catch (Standard_Failure&) {
                        } catch (...) {
                        }
                    });
                } catch (...) {
                }
        } else if (!extrusionSelectedFaces.isEmpty() || !extrusionSelectedIndices.isEmpty()) {
            clearExtrudeRevolveProfileSelection(true);
        }
    } catch (Standard_Failure& e) {
        (void)e;
    } catch (const std::exception& e) {
        (void)e;
    } catch (...) {
    }
}

vtkRenderer* Widget::featureSelectionOverlay()
{
    return appearanceOverlay();
}

// 更新悬停面的高亮显示
void Widget::updateExtrusionHoverHighlight()
{
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay;
    if (!target) {
        target = renderer.GetPointer();
    }

    // 先移除旧的悬停高亮 actor
    if (extrusionHoverHighlightActor) {
        removeSceneActor(extrusionHoverHighlightActor);
        extrusionHoverHighlightActor = nullptr;
    }
    if (extrusionHoverOutlineActor) {
        removeSceneActor(extrusionHoverOutlineActor);
        extrusionHoverOutlineActor = nullptr;
    }

    // 恢复“悬浮变暗”的模型，避免悬浮离开后仍保持变暗
    if (extrusionHoverDimModelIndex_ >= 0 && extrusionHoverDimModelIndex_ < historyList.size()) {
        auto& dimRec = historyList[extrusionHoverDimModelIndex_];
        if (renderStateFor(dimRec).actor && renderStateFor(dimRec).actor->GetVisibility() != 0) {
            renderStateFor(dimRec).actor->GetProperty()->SetColor(
                extrusionHoverDimOriginalColor_.redF(),
                extrusionHoverDimOriginalColor_.greenF(),
                extrusionHoverDimOriginalColor_.blueF()
            );
            renderStateFor(dimRec).actor->GetProperty()->SetOpacity(extrusionHoverDimOriginalOpacity_);
        }
        extrusionHoverDimModelIndex_ = -1;
        extrusionHoverDimOriginalOpacity_ = 1.0;
    }

    // 如果存在悬停的面或边，创建高亮
    if (hasHoveredFace && target && !hoveredFace.shape.IsNull()) {
        try {
            // 直接使用已存储的悬停形状
            TopoDS_Shape hoveredShape = hoveredFace.shape;
            if (!hoveredShape.IsNull()) {
                // 关键：当悬浮到“面”时，临时变暗该面的所属模型，避免半透明高亮在遮挡视角下被不透明面挡住
                if (!featureOperationGhostMode_ &&
                    hoveredFace.shapeType == TopAbs_FACE &&
                    hoveredFace.modelIndex >= 0 &&
                    hoveredFace.modelIndex < historyList.size() &&
                    renderStateFor(historyList[hoveredFace.modelIndex]).actor) {
                    auto& rec = historyList[hoveredFace.modelIndex];
                    if (renderStateFor(rec).actor && renderStateFor(rec).actor->GetVisibility() != 0) {
                        extrusionHoverDimModelIndex_ = hoveredFace.modelIndex;

                        double rgb[3] = {0, 0, 0};
                        renderStateFor(rec).actor->GetProperty()->GetColor(rgb);
                        extrusionHoverDimOriginalColor_ = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
                        extrusionHoverDimOriginalOpacity_ = renderStateFor(rec).actor->GetProperty()->GetOpacity();

                        renderStateFor(rec).actor->GetProperty()->SetColor(
                            extrusionHoverDimOriginalColor_.redF() * 0.3,
                            extrusionHoverDimOriginalColor_.greenF() * 0.3,
                            extrusionHoverDimOriginalColor_.blueF() * 0.3
                        );
                        renderStateFor(rec).actor->GetProperty()->SetOpacity(0.25);
                    }
                }

                // 使用 VIS 自动数据源创建悬停高亮。
                // Step 1: 对形状进行网格离散化
                BRepMesh_IncrementalMesh mesh(hoveredShape, 0.05, Standard_False, 0.3, Standard_True);
                mesh.Perform();

                // Step 2: 创建VIS形状包装器
                Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(hoveredShape);
                hlShape->SetId(999998); // 使用另一个特殊的ID，避免与选中高亮冲突

                vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
                    ModelShapePipeline::createShapeDataSource(hlShape);
                vtkSmartPointer<vtkPolyData> ownedPd =
                    ModelShapePipeline::copyShapeDataSourceOutput(shapeDataSource, true);

                if (ownedPd && ownedPd->GetNumberOfPoints() > 0) {
                    // 对于边，保留线条数据；对于面，清理线条和顶点数据
                    if (hoveredFace.shapeType == TopAbs_FACE) {
                        ownedPd->SetLines(nullptr);
                        ownedPd->SetVerts(nullptr);
                    }

                    // Step 4: 创建映射器
                    vtkSmartPointer<vtkPolyDataMapper> mapper =
                        vtkSmartPointer<vtkPolyDataMapper>::New();
                    mapper->SetInputData(ownedPd);
                    mapper->ScalarVisibilityOff();

                    // Step 5: 悬浮 = 鲜红（叠层绘制，不被幽灵体挡住）
                    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                    actor->SetMapper(mapper);
                    actor->GetProperty()->SetColor(1.0, 0.12, 0.08);
                    const bool hoverFace = (hoveredFace.shapeType == TopAbs_FACE);
                    const bool hoverCurve = (hoveredFace.shapeType == TopAbs_EDGE
                                            || hoveredFace.shapeType == TopAbs_WIRE);
                    if (hoverFace) {
                        actor->GetProperty()->SetRepresentationToSurface();
                        actor->GetProperty()->SetOpacity(0.55);
                        ModelDisplayStyle::applyFeaturePickHighlight(actor, false);
                    } else if (hoverCurve) {
                        actor->GetProperty()->SetRepresentationToWireframe();
                        actor->GetProperty()->SetOpacity(1.0);
                        actor->GetProperty()->SetLineWidth(8.0);
                        ModelDisplayStyle::applyFeaturePickHighlight(actor, true);
                    }
                    if (hoverFace) {
                        // 面悬停：不要显示三角网格边（会出现对角线），只显示边界轮廓
                        actor->GetProperty()->EdgeVisibilityOff();

                        vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
                        featureEdges->SetInputData(ownedPd);
                        featureEdges->BoundaryEdgesOn();
                        featureEdges->FeatureEdgesOff();
                        featureEdges->ManifoldEdgesOff();
                        featureEdges->NonManifoldEdgesOff();
                        featureEdges->Update();

                        vtkSmartPointer<vtkPolyDataMapper> edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                        edgeMapper->SetInputConnection(featureEdges->GetOutputPort());
                        edgeMapper->ScalarVisibilityOff();

                        vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
                        edgeActor->SetMapper(edgeMapper);
                        edgeActor->GetProperty()->SetColor(1.0, 0.12, 0.08);
                        edgeActor->GetProperty()->SetLineWidth(7.0);
                        edgeActor->GetProperty()->SetOpacity(1.0);
                        edgeActor->SetPickable(false);
                        ModelDisplayStyle::applyFeaturePickHighlight(edgeActor, true);

                        target->AddActor(edgeActor);
                        extrusionHoverOutlineActor = edgeActor;
                    } else if (hoverCurve) {
                        actor->GetProperty()->EdgeVisibilityOn();
                    }
                    actor->SetPickable(false);  // 高亮对象不可拾取

                    target->AddActor(actor);
                    extrusionHoverHighlightActor = actor;

                    if (vtkWidget && vtkWidget->renderWindow()) {
                        vtkWidget->renderWindow()->Render();
                    }
                }
            }
        } catch (Standard_Failure& e) {
            (void)e;
        } catch (const std::exception& e) {
            (void)e;
        } catch (...) {
        }
    } else {
        // 没有悬停的面，确保移除高亮
        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
}

// 更新面的/边的高亮显示
void Widget::updateExtrusionFaceHighlight()
{
    // 1. 先恢复所有原本可见的模型的原始显示状态（不改变隐藏模型的可见性）
    //    若处于拉伸/旋转幽灵模式，则恢复为半透明+轮廓，而不是不透明实心
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor) {
            bool wasVisible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            if (wasVisible) {
                if (featureOperationGhostMode_) {
                    applyFeatureGhostStyleToModel(i);
                } else {
                    ModelDisplayStyle::applyModelColorOpacity(
                        renderStateFor(historyList[i]).actor->GetProperty(),
                        historyList[i].color,
                        1.0);
                }
            }
        }
        if (renderStateFor(historyList[i]).highlightActor) {
            // 默认先隐藏高亮 actor，后面对有选中面的模型再打开
            renderStateFor(historyList[i]).highlightActor->SetVisibility(false);
        }
    }

    // 2. 使用单独的 VIS 管线为所有选中的面/边创建高亮几何（叠层，避免被幽灵体遮挡）
    vtkRenderer* overlay = featureSelectionOverlay();
    vtkRenderer* target = overlay;
    if (!target) {
        target = renderer.GetPointer();
    }

    auto removeHighlightActors = [&]() {
        for (int i = 0; i < extrusionFaceHighlightActors.size(); ++i) {
            if (!extrusionFaceHighlightActors[i]) continue;
            removeSceneActor(extrusionFaceHighlightActors[i]);
        }
        extrusionFaceHighlightActors.clear();
    };
    removeHighlightActors();

    // 为所有选中的面/边创建高亮
    if (!extrusionSelectedFaces.isEmpty() && target) {
        QSet<int> dimmedModels;

        for (int idx = 0; idx < extrusionSelectedFaces.size(); ++idx) {
            const ExtrusionFaceSelection& faceSel = extrusionSelectedFaces[idx];
            if (!faceSel.shape.IsNull() &&
                (faceSel.shapeType == TopAbs_FACE ||
                 faceSel.shapeType == TopAbs_EDGE ||
                 faceSel.shapeType == TopAbs_WIRE)) {
                try {
                    if (currentSelectionMode == FaceSelection &&
                        faceSel.modelIndex >= 0 && faceSel.modelIndex < historyList.size()) {
                        if (!dimmedModels.contains(faceSel.modelIndex)) {
                            ModelingHistory& record = historyList[faceSel.modelIndex];
                            if (renderStateFor(record).actor) {
                                bool wasVisible = (renderStateFor(record).actor->GetVisibility() != 0);
                                if (wasVisible && !featureOperationGhostMode_) {
                                    ModelDisplayStyle::applyDimmedModelAppearance(
                                        renderStateFor(record).actor->GetProperty(),
                                        record.color,
                                        0.3,
                                        0.3);
                                }
                            }
                            dimmedModels.insert(faceSel.modelIndex);
                        }
                    }

                    BRepMesh_IncrementalMesh mesh(faceSel.shape, 0.05, Standard_False, 0.3, Standard_True);
                    mesh.Perform();

                    Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(faceSel.shape);
                    hlShape->SetId(999999 + idx);

                    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource =
                        ModelShapePipeline::createShapeDataSource(hlShape);
                    vtkSmartPointer<vtkPolyData> ownedPd =
                        ModelShapePipeline::copyShapeDataSourceOutput(shapeDataSource, true);

                    if (ownedPd && ownedPd->GetNumberOfPoints() > 0) {
                        if (faceSel.shapeType == TopAbs_FACE) {
                            ownedPd->SetLines(nullptr);
                            ownedPd->SetVerts(nullptr);
                        }

                        vtkSmartPointer<vtkPolyDataMapper> mapper =
                            vtkSmartPointer<vtkPolyDataMapper>::New();
                        mapper->SetInputData(ownedPd);
                        mapper->ScalarVisibilityOff();

                        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                        actor->SetMapper(mapper);
                        actor->GetProperty()->SetColor(1.0, 0.12, 0.08);
                        const bool selFace = (faceSel.shapeType == TopAbs_FACE);
                        const bool selCurve = (faceSel.shapeType == TopAbs_EDGE
                                              || faceSel.shapeType == TopAbs_WIRE);
                        if (selFace) {
                            actor->GetProperty()->SetRepresentationToSurface();
                            actor->GetProperty()->SetOpacity(0.65);
                            ModelDisplayStyle::applyFeaturePickHighlight(actor, false);
                            actor->GetProperty()->EdgeVisibilityOff();

                            vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
                            featureEdges->SetInputData(ownedPd);
                            featureEdges->BoundaryEdgesOn();
                            featureEdges->FeatureEdgesOff();
                            featureEdges->ManifoldEdgesOff();
                            featureEdges->NonManifoldEdgesOff();
                            featureEdges->Update();

                            vtkSmartPointer<vtkPolyDataMapper> edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                            edgeMapper->SetInputConnection(featureEdges->GetOutputPort());
                            edgeMapper->ScalarVisibilityOff();

                            vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
                            edgeActor->SetMapper(edgeMapper);
                            edgeActor->GetProperty()->SetColor(1.0, 0.12, 0.08);
                            edgeActor->GetProperty()->SetLineWidth(8.0);
                            edgeActor->GetProperty()->SetOpacity(1.0);
                            edgeActor->SetPickable(false);
                            ModelDisplayStyle::applyFeaturePickHighlight(edgeActor, true);

                            target->AddActor(edgeActor);
                            extrusionFaceHighlightActors.append(edgeActor);
                        } else if (selCurve) {
                            actor->GetProperty()->SetRepresentationToWireframe();
                            actor->GetProperty()->SetOpacity(1.0);
                            actor->GetProperty()->EdgeVisibilityOn();
                            actor->GetProperty()->SetLineWidth(9.0);
                            ModelDisplayStyle::applyFeaturePickHighlight(actor, true);
                        } else {
                            actor->GetProperty()->SetOpacity(1.0);
                        }
                        actor->SetPickable(false);

                        target->AddActor(actor);
                        extrusionFaceHighlightActors.append(actor);
                    }
                } catch (...) {
                    (void)idx;
                }
            }
        }

        if (vtkWidget && vtkWidget->renderWindow()) {
            vtkWidget->renderWindow()->Render();
        }
    }
}

// 清除面的高亮
void Widget::clearExtrusionFaceHighlight()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).highlightActor) {
            renderStateFor(historyList[i]).highlightActor->SetVisibility(false);
        }
        if (renderStateFor(historyList[i]).actor) {
            // 恢复原本可见的模型的颜色和透明度，但不改变隐藏模型的可见性
            bool wasVisible = (renderStateFor(historyList[i]).actor->GetVisibility() != 0);
            if (wasVisible) {
                if (featureOperationGhostMode_) {
                    applyFeatureGhostStyleToModel(i);
                } else {
                    ModelDisplayStyle::applyModelColorOpacity(
                        renderStateFor(historyList[i]).actor->GetProperty(),
                        historyList[i].color,
                        1.0);
                }
            }
            // 注意：不改变模型的可见性状态，保持用户设置的隐藏/显示状态
        }
    }

    // 移除所有选中/悬浮高亮（主渲染器与叠加层都清）
    for (int i = 0; i < extrusionFaceHighlightActors.size(); ++i) {
        if (!extrusionFaceHighlightActors[i]) continue;
        removeSceneActor(extrusionFaceHighlightActors[i]);
    }
    extrusionFaceHighlightActors.clear();

    if (extrusionHoverHighlightActor) {
        removeSceneActor(extrusionHoverHighlightActor);
        extrusionHoverHighlightActor = nullptr;
    }
    if (extrusionHoverOutlineActor) {
        removeSceneActor(extrusionHoverOutlineActor);
        extrusionHoverOutlineActor = nullptr;
    }

    // 恢复“悬浮变暗”的模型
    if (extrusionHoverDimModelIndex_ >= 0 && extrusionHoverDimModelIndex_ < historyList.size()) {
        auto& dimRec = historyList[extrusionHoverDimModelIndex_];
        if (renderStateFor(dimRec).actor && renderStateFor(dimRec).actor->GetVisibility() != 0) {
            ModelDisplayStyle::applyModelColorOpacity(
                renderStateFor(dimRec).actor->GetProperty(),
                extrusionHoverDimOriginalColor_,
                extrusionHoverDimOriginalOpacity_);
        }
        extrusionHoverDimModelIndex_ = -1;
        extrusionHoverDimOriginalOpacity_ = 1.0;
    }

    clearSubShapeHighlight();
    hasHoveredFace = false;
    
    if (vtkWidget && vtkWidget->renderWindow()) {
        vtkWidget->renderWindow()->Render();
    }
}
