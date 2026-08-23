// Updated at 2026-01-10 to fix compilation errors
#include "widget.h"
#include "extrusiondialog.h"
#include <QSet>
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
#include <vtkVersionMacros.h>
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
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <IVtkVTK_ShapeData.hxx>
#include <IVtkOCC_ShapeMesher.hxx>

namespace {

// 选择/悬浮高亮：强制走不透明通道 + 深度偏移，保证画在幽灵模式之上也清晰
void styleFeaturePickHighlight(vtkActor* actor, bool isLine, bool /*selected*/)
{
    if (!actor) return;
    vtkProperty* p = actor->GetProperty();
    p->SetLighting(false);
    p->SetAmbient(1.0);
    p->SetDiffuse(0.0);
    p->SetSpecular(0.0);
    p->SetBackfaceCulling(false);
#if VTK_MAJOR_VERSION >= 9
    actor->ForceOpaqueOn();
#endif
    if (vtkMapper* mapper = actor->GetMapper()) {
        mapper->SetResolveCoincidentTopologyToPolygonOffset();
        if (isLine) {
            mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-24.0, -24.0);
        } else {
            mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-16.0, -16.0);
        }
    }
}

// 拷贝 IVtk 网格，避免局部 Handle 析构后 mapper 悬空崩溃
vtkSmartPointer<vtkPolyData> ownHighlightPolyData(vtkPolyData* src)
{
    if (!src) return nullptr;
    vtkSmartPointer<vtkPolyData> owned = vtkSmartPointer<vtkPolyData>::New();
    owned->ShallowCopy(src);
    return owned;
}

static bool extractSingleWireFromShape(const TopoDS_Shape& shape, TopoDS_Wire& outWire)
{
    if (shape.IsNull()) return false;
    if (shape.ShapeType() == TopAbs_WIRE) {
        outWire = TopoDS::Wire(shape);
        return !outWire.IsNull();
    }

    TopoDS_Wire candidate;
    int wireCount = 0;
    for (TopExp_Explorer ex(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        const TopoDS_Wire w = TopoDS::Wire(ex.Current());
        if (w.IsNull()) continue;
        candidate = w;
        ++wireCount;
        if (wireCount > 1) {
            return false;
        }
    }
    if (wireCount == 1 && !candidate.IsNull()) {
        outWire = candidate;
        return true;
    }
    return false;
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

        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        prepareShapePickerBindingsForCurrentContext();
        
        IVtkTools_ShapePicker* picker = shapePicker;

        IVtk_IdType subShapeId = -1;
        int modelIndex = -1;
        bool found = false;

        // 根据选择模式决定拾取面还是边
        if (currentSelectionMode == FaceSelection) {
            // 面选择模式：只拾取面
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
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
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
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
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
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
                    for (int i = 0; i < historyList.size(); ++i) {
                        if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                            modelIndex = i;
                            found = true;
                            break;
                        }
                    }
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
                if (record.actor && record.actor->GetVisibility() == 0) {
                    // 模型被隐藏，忽略这次拾取
                    if (hasHoveredFace) {
                        hasHoveredFace = false;
                        updateExtrusionHoverHighlight();
                    }
                    return;
                }
                
                // 获取悬停形状的信息并验证形状类型
                try {
                    if (!record.shapeWrapper.IsNull()) {
                        TopoDS_Shape hoveredShape;
                        if (subShapeId != -1) {
                            try {
                                hoveredShape = record.shapeWrapper->GetSubShape(subShapeId);
                            } catch (Standard_Failure&) {
                                hoveredShape = TopoDS_Shape();
                            } catch (...) {
                                hoveredShape = TopoDS_Shape();
                            }
                        } else if (currentSelectionMode != FaceSelection) {
                            TopoDS_Wire fallbackWire;
                            if (extractSingleWireFromShape(record.occShape, fallbackWire)) {
                                hoveredShape = fallbackWire;
                            }
                        }
                        if (!hoveredShape.IsNull()) {
                            // 验证拾取到的形状类型是否符合选择模式
                            bool isValid = false;
                            if (currentSelectionMode == FaceSelection) {
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

        shapePicker->SetRenderer(renderer);
        shapePicker->SetTolerance(0.05);
        prepareShapePickerBindingsForCurrentContext();
        
        IVtkTools_ShapePicker* picker = shapePicker;

        IVtk_IdType subShapeId = -1;
        int modelIndex = -1;
        bool found = false;

        // 根据选择模式决定拾取面还是边
        if (currentSelectionMode == FaceSelection) {
            // 面选择模式：只拾取面
            picker->SetSelectionMode(SM_Face);
            picker->Pick(x, y, 0);
            
            IVtk_ShapeIdList ids = picker->GetPickedShapesIds();
            if (!ids.IsEmpty()) {
                IVtk_IdType shapeId = ids.First();
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
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
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    subShapeId = picker->GetPickedSubShapesIds(shapeId).IsEmpty() ? -1 : picker->GetPickedSubShapesIds(shapeId).First();
                    
                    // 验证拾取到的确实是边，而不是面
                    if (subShapeId != -1) {
                        ModelingHistory& record = historyList[modelIndex];
                        if (!record.shapeWrapper.IsNull()) {
                            TopoDS_Shape pickedShape = record.shapeWrapper->GetSubShape(subShapeId);
                            if (!pickedShape.IsNull()) {
                                if (pickedShape.ShapeType() != TopAbs_EDGE) {
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
                for (int i = 0; i < historyList.size(); ++i) {
                    if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                        modelIndex = i;
                        found = true;
                        break;
                    }
                }
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
                    for (int i = 0; i < historyList.size(); ++i) {
                        if (!historyList[i].shapeWrapper.IsNull() && historyList[i].shapeWrapper->GetId() == shapeId) {
                            modelIndex = i;
                            found = true;
                            break;
                        }
                    }
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
                if (record.actor && record.actor->GetVisibility() == 0) {
                    // 模型被隐藏，忽略这次拾取
                    return;
                }
            }
            
            // 执行拾取
                try {
                    ModelingHistory& record = historyList[modelIndex];
                    if (record.shapeWrapper.IsNull()) return;

                    TopoDS_Shape mainShape = record.occShape;
                    TopoDS_Shape selectedSubShape;
                    if (subShapeId != -1) {
                        try {
                            selectedSubShape = record.shapeWrapper->GetSubShape(subShapeId);
                        } catch (Standard_Failure&) {
                            return;
                        } catch (...) {
                            return;
                        }
                    } else if (currentSelectionMode != FaceSelection) {
                        TopoDS_Wire fallbackWire;
                        if (extractSingleWireFromShape(mainShape, fallbackWire)) {
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
                        TopExp_Explorer wireExp(mainShape, TopAbs_WIRE);
                        bool wireFound = false;
                        for (; wireExp.More(); wireExp.Next()) {
                            TopoDS_Wire wire = TopoDS::Wire(wireExp.Current());
                            TopExp_Explorer edgeExp(wire, TopAbs_EDGE);
                            for (; edgeExp.More(); edgeExp.Next()) {
                                if (edgeExp.Current().IsSame(selectedSubShape)) {
                                    selectedSubShape = wire;
                                    wireFound = true;
                                    break;
                                }
                            }
                            if (wireFound) break;
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
                        if (selection.shapeType == TopAbs_WIRE && ex.shapeType == TopAbs_WIRE) {
                            if (!ex.shape.IsNull() && ex.shape.IsSame(selection.shape)) {
                                sameShapeIdx = i;
                                break;
                            }
                        } else if (ex.subShapeId == selection.subShapeId) {
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
        }
    } catch (Standard_Failure& e) {
        (void)e;
    } catch (const std::exception& e) {
        (void)e;
    } catch (...) {
    }
    setFeatureGizmoActorsPickable(true);
}

vtkRenderer* Widget::featureSelectionOverlay()
{
    if (!featureSelectionOverlayRenderer_ && vtkWidget && renderer) {
        // 兜底创建（正常应在 setupCenterAxisSelector 中初始化）
        if (vtkWidget->renderWindow()->GetNumberOfLayers() < 4) {
            vtkWidget->renderWindow()->SetNumberOfLayers(4);
        }
        featureSelectionOverlayRenderer_ = vtkSmartPointer<vtkRenderer>::New();
        featureSelectionOverlayRenderer_->SetLayer(1);
        featureSelectionOverlayRenderer_->SetViewport(0.0, 0.0, 1.0, 1.0);
        featureSelectionOverlayRenderer_->InteractiveOff();
        featureSelectionOverlayRenderer_->SetUseFXAA(false);
        configureFeatureSelectionOverlayAlwaysOnTop();
        {
            vtkSmartPointer<vtkCamera> cam = vtkSmartPointer<vtkCamera>::New();
            if (renderer->GetActiveCamera()) {
                cam->DeepCopy(renderer->GetActiveCamera());
            }
            featureSelectionOverlayRenderer_->SetActiveCamera(cam);
        }
        configureSceneLights(featureSelectionOverlayRenderer_);
        vtkWidget->renderWindow()->AddRenderer(featureSelectionOverlayRenderer_);
        if (referenceOverlayRenderer_) {
            referenceOverlayRenderer_->SetLayer(2);
        }
        if (centerAxesRenderer) {
            centerAxesRenderer->SetLayer(3);
        }
    } else if (featureSelectionOverlayRenderer_) {
        configureFeatureSelectionOverlayAlwaysOnTop();
    }
    // 位姿跟随主相机，裁切保持独立（勿再 SetActiveCamera(main)）
    syncOverlayCameras();
    return featureSelectionOverlayRenderer_;
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
        if (dimRec.actor && dimRec.actor->GetVisibility() != 0) {
            dimRec.actor->GetProperty()->SetColor(
                extrusionHoverDimOriginalColor_.redF(),
                extrusionHoverDimOriginalColor_.greenF(),
                extrusionHoverDimOriginalColor_.blueF()
            );
            dimRec.actor->GetProperty()->SetOpacity(extrusionHoverDimOriginalOpacity_);
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
                    historyList[hoveredFace.modelIndex].actor) {
                    auto& rec = historyList[hoveredFace.modelIndex];
                    if (rec.actor && rec.actor->GetVisibility() != 0) {
                        extrusionHoverDimModelIndex_ = hoveredFace.modelIndex;

                        double rgb[3] = {0, 0, 0};
                        rec.actor->GetProperty()->GetColor(rgb);
                        extrusionHoverDimOriginalColor_ = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
                        extrusionHoverDimOriginalOpacity_ = rec.actor->GetProperty()->GetOpacity();

                        rec.actor->GetProperty()->SetColor(
                            extrusionHoverDimOriginalColor_.redF() * 0.3,
                            extrusionHoverDimOriginalColor_.greenF() * 0.3,
                            extrusionHoverDimOriginalColor_.blueF() * 0.3
                        );
                        rec.actor->GetProperty()->SetOpacity(0.25);
                    }
                }

                // 使用手动构建 PolyData 的方式创建悬停高亮
                // Step 1: 对形状进行网格离散化
                BRepMesh_IncrementalMesh mesh(hoveredShape, 0.05, Standard_False, 0.3, Standard_True);
                mesh.Perform();

                // Step 2: 创建VIS形状包装器
                Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(hoveredShape);
                hlShape->SetId(999998); // 使用另一个特殊的ID，避免与选中高亮冲突

                // Step 3: 手动构建 PolyData
                Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
                IVtkOCC_ShapeMesher mesher;
                mesher.Build(hlShape, shapeData);
                vtkPolyData* meshPolyData = shapeData->getVtkPolyData();
                
                if (meshPolyData && meshPolyData->GetNumberOfPoints() > 0) {
                    vtkSmartPointer<vtkPolyData> ownedPd = ownHighlightPolyData(meshPolyData);
                    if (!ownedPd) {
                        // fall through
                    } else {
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
                        styleFeaturePickHighlight(actor, false, false);
                    } else if (hoverCurve) {
                        actor->GetProperty()->SetRepresentationToWireframe();
                        actor->GetProperty()->SetOpacity(1.0);
                        actor->GetProperty()->SetLineWidth(8.0);
                        styleFeaturePickHighlight(actor, true, false);
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
                        styleFeaturePickHighlight(edgeActor, true, false);

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
        if (historyList[i].actor) {
            bool wasVisible = (historyList[i].actor->GetVisibility() != 0);
            if (wasVisible) {
                if (featureOperationGhostMode_) {
                    applyFeatureGhostStyleToModel(i);
                } else {
                    historyList[i].actor->GetProperty()->SetColor(
                        historyList[i].color.redF(),
                        historyList[i].color.greenF(),
                        historyList[i].color.blueF()
                    );
                    historyList[i].actor->GetProperty()->SetOpacity(1.0);
                }
            }
        }
        if (historyList[i].highlightActor) {
            // 默认先隐藏高亮 actor，后面对有选中面的模型再打开
            historyList[i].highlightActor->SetVisibility(false);
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
                            if (record.actor) {
                                bool wasVisible = (record.actor->GetVisibility() != 0);
                                if (wasVisible && !featureOperationGhostMode_) {
                                    record.actor->GetProperty()->SetColor(
                                        record.color.redF() * 0.3,
                                        record.color.greenF() * 0.3,
                                        record.color.blueF() * 0.3
                                    );
                                    record.actor->GetProperty()->SetOpacity(0.3);
                                }
                            }
                            dimmedModels.insert(faceSel.modelIndex);
                        }
                    }

                    BRepMesh_IncrementalMesh mesh(faceSel.shape, 0.05, Standard_False, 0.3, Standard_True);
                    mesh.Perform();

                    Handle(IVtkOCC_Shape) hlShape = new IVtkOCC_Shape(faceSel.shape);
                    hlShape->SetId(999999 + idx);

                    Handle(IVtkVTK_ShapeData) shapeData = new IVtkVTK_ShapeData();
                    IVtkOCC_ShapeMesher mesher;
                    mesher.Build(hlShape, shapeData);
                    vtkPolyData* meshPolyData = shapeData->getVtkPolyData();

                    if (meshPolyData && meshPolyData->GetNumberOfPoints() > 0) {
                        vtkSmartPointer<vtkPolyData> ownedPd = ownHighlightPolyData(meshPolyData);
                        if (!ownedPd) continue;

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
                            styleFeaturePickHighlight(actor, false, true);
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
                            styleFeaturePickHighlight(edgeActor, true, true);

                            target->AddActor(edgeActor);
                            extrusionFaceHighlightActors.append(edgeActor);
                        } else if (selCurve) {
                            actor->GetProperty()->SetRepresentationToWireframe();
                            actor->GetProperty()->SetOpacity(1.0);
                            actor->GetProperty()->EdgeVisibilityOn();
                            actor->GetProperty()->SetLineWidth(9.0);
                            styleFeaturePickHighlight(actor, true, true);
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
        if (historyList[i].highlightActor) {
            historyList[i].highlightActor->SetVisibility(false);
        }
        if (historyList[i].actor) {
            // 恢复原本可见的模型的颜色和透明度，但不改变隐藏模型的可见性
            bool wasVisible = (historyList[i].actor->GetVisibility() != 0);
            if (wasVisible) {
                if (featureOperationGhostMode_) {
                    applyFeatureGhostStyleToModel(i);
                } else {
                    historyList[i].actor->GetProperty()->SetColor(
                        historyList[i].color.redF(),
                        historyList[i].color.greenF(),
                        historyList[i].color.blueF()
                    );
                    historyList[i].actor->GetProperty()->SetOpacity(1.0);
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
        if (dimRec.actor && dimRec.actor->GetVisibility() != 0) {
            dimRec.actor->GetProperty()->SetColor(
                extrusionHoverDimOriginalColor_.redF(),
                extrusionHoverDimOriginalColor_.greenF(),
                extrusionHoverDimOriginalColor_.blueF()
            );
            dimRec.actor->GetProperty()->SetOpacity(extrusionHoverDimOriginalOpacity_);
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
