// 拉伸：对话框、选择、预览与执行（从 widget.cpp 拆出；与旋转预览之间的函数拆到本文件）
#include "widget.h"
#include "ui_widget.h"
#include "extrusiondialog.h"
#include "addmodelcommand.h"
#include "feature_topology.h"
#include "command.h"

#include <QColor>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVTKOpenGLNativeWidget.h>
#include <QWidget>
#include <Qt>

#include <cmath>

#include <Standard_Failure.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>

#include <Geom_Surface.hxx>

#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>

namespace {

void captureBaseParams(const ModelingHistory& base, ExtrusionRecipeData& extrusion)
{
    extrusion.baseType = base.type;
    extrusion.baseParam1 = base.param1;
    extrusion.baseParam2 = base.param2;
    extrusion.baseParam3 = base.param3;
    extrusion.baseHasOrigin = base.hasOrigin;
    extrusion.baseOriginX = base.originX;
    extrusion.baseOriginY = base.originY;
    extrusion.baseOriginZ = base.originZ;
}

FeatureRecipe buildFaceExtrusionRecipe(const Widget* widget,
                                       int modelIndex,
                                       const TopoDS_Shape& profileShape,
                                       TopAbs_ShapeEnum shapeType,
                                       IVtk_IdType subShapeId,
                                       const gp_Dir& direction,
                                       double distance,
                                       double startOffset,
                                       bool makeSheetBody,
                                       bool useVectorDirection,
                                       bool mergedInPlace,
                                       const ModelingHistory* baseBeforeMerge = nullptr,
                                       const TopoDS_Shape* parentShapeOverride = nullptr)
{
    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    recipe.parentIndices = { modelIndex };
    recipe.extrusion.direction = direction;
    recipe.extrusion.lengthFwd = distance;
    recipe.extrusion.startOffset = startOffset;
    recipe.extrusion.makeSheetBody = makeSheetBody;
    recipe.extrusion.useVectorDirection = useVectorDirection;
    recipe.extrusion.mergedInPlace = mergedInPlace;
    recipe.extrusion.mergeTargetIndex = modelIndex;
    if (widget && modelIndex >= 0 && modelIndex < widget->getHistoryList().size()) {
        const TopoDS_Shape& parentShape = parentShapeOverride
            ? *parentShapeOverride
            : widget->getHistoryList()[modelIndex].occShape;
        recipe.extrusion.profiles.append(makeSubShapeRef(
            modelIndex, parentShape, profileShape, shapeType, subShapeId));
    }
    if (mergedInPlace && baseBeforeMerge) {
        captureBaseParams(*baseBeforeMerge, recipe.extrusion);
    }
    return recipe;
}

FeatureRecipe buildModelExtrusionRecipe(int modelIndex,
                                        const gp_Dir& direction,
                                        double distance,
                                        bool makeSheetBody,
                                        bool useVectorDirection)
{
    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    recipe.parentIndices = { modelIndex };
    recipe.extrusion.profileModelIndices = { modelIndex };
    recipe.extrusion.direction = direction;
    recipe.extrusion.lengthFwd = distance;
    recipe.extrusion.makeSheetBody = makeSheetBody;
    recipe.extrusion.useVectorDirection = useVectorDirection;
    return recipe;
}

} // namespace

#include <TopAbs_ShapeEnum.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

// 拉伸按钮点击事件
void Widget::on_extrude_clicked()
{
    // 清除之前的拉伸选择
    extrusionSelectedIndices.clear();
    extrusionSelectedFaces.clear();
    clearExtrusionFaceHighlight();

    // 创建拉伸对话框
    ExtrusionDialog *dialog = new ExtrusionDialog(dialogParentWidget());

    // 设置对话框为非模态，允许与主窗口交互
    dialog->setModal(false);

    // 设置对话框始终显示在最前面，但不会阻塞交互
    dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    // 保存对话框指针
    extrusionDialog = dialog;

    // 连接信号槽
    connect(dialog, &ExtrusionDialog::startSelection, this, &Widget::onExtrusionStartSelection);
    connect(dialog, &ExtrusionDialog::clearSelection, this, &Widget::onExtrusionClearSelection);
    connect(dialog, &ExtrusionDialog::previewRequested, this, &Widget::onExtrusionPreviewRequested);
    connect(dialog, &ExtrusionDialog::cancelPreviewRequested, this, &Widget::onExtrusionCancelPreviewRequested);
    connect(dialog, &ExtrusionDialog::selectionModeChanged, this, &Widget::onExtrusionSelectionModeChanged);
    connect(dialog, &ExtrusionDialog::requestVectorMode, this, [this](int modeIndex) {
        if (extrusionDialog) {
            extrusionDialog->setVectorModeIndex(modeIndex);
        }
        if (modeIndex == 0) {
            applyAutoVectorFromSelection();
        } else {
            openVectorDialog(modeIndex);
        }
    });
    connect(dialog, &ExtrusionDialog::vectorSelectionRequested, this, [this]() {
        statusBar()->showMessage(tr("请在视图中点击左下角三重轴选择拉伸方向"), 5000);
    });
    connect(dialog, &ExtrusionDialog::parametersChanged, this, [this]() {
        if (featureResultPreviewActive_) return;
        updateExtrusionHandles();
        refreshExtrusionLivePreview();
    });
    connect(dialog, &ExtrusionDialog::startBooleanTargetSelection, this, [this]() {
        currentSelectionMode = FeatureBooleanTargetSelect;
        if (statusBar()) {
            statusBar()->showMessage(tr("请选择布尔运算的目标体"), 4000);
        }
    });

    // 打开矢量对话框：允许任意方向 gp_Dir
    if (auto* vectorDialogBtn = dialog->findChild<QPushButton*>("pushButton_2")) {
        connect(vectorDialogBtn, &QPushButton::clicked, this, [this]() {
            openVectorDialog();
        });
    }

    // 同步“反向”按钮：让向量箭头预览方向随之变化
    if (auto* reverseBtn = dialog->findChild<QPushButton*>("pushButton")) {
        connect(reverseBtn, &QPushButton::clicked, this, [this, dialog]() {
            updateVectorArrowPreviewWithAxisReversed(dialog->isAxisReversed());
            updateExtrusionHandles();
            refreshExtrusionLivePreview();
        });
    }
    connect(dialog, &ExtrusionDialog::accepted, this, [this, dialog]() {
        // 统一交给 performExtrusion 做校验与提示，避免重复弹窗
        performExtrusion(dialog);
        clearVectorDialogArrowPreview();
        clearExtrusionHandles();
        clearSelectedPoint(); // 清理两点取矢量时的点高亮残留
    });
    connect(dialog, &ExtrusionDialog::rejected, this, [this]() {
        cleanupExtrudeRevolveDialogSession();
    });
    
    // 对话框关闭时清理（含标题栏关闭 / Esc / 取消）
    connect(dialog, &ExtrusionDialog::finished, this, [this](int result) {
        Q_UNUSED(result);
        cleanupExtrudeRevolveDialogSession();
        if (vtkWidget) {
            vtkWidget->setFocus();
        }
        extrusionDialog = nullptr;
    });

    // 对话框关闭时自动删除
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // 使用 show() 而不是 exec()，这样不会阻塞主窗口
    dialog->show();
    // 幽灵模式延后到开始选择截面时再启用，避免一点击拉伸就全体半透明
    if (statusBar()) {
        statusBar()->showMessage(tr("拉伸：默认自动判断方向，请选择截面"), 4000);
    }
    // 打开时先刷新拾取绑定，降低“新建体后立刻选边崩溃”概率
    if (renderer && shapePicker) {
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
    // 确保焦点回到VTK窗口，这样它才能接收鼠标事件
    vtkWidget->setFocus();
    vtkWidget->renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}
// 开始拉伸选择
void Widget::onExtrusionStartSelection()
{
    // 获取对话框并确定选择模式
    ExtrusionDialog *dialog = qobject_cast<ExtrusionDialog*>(sender());
    if (dialog) {
        QString sectionType = dialog->getSectionType();
        if (sectionType == "曲线") {
            currentSelectionMode = EdgeSelection;
        } else if (sectionType == "面片") {
            currentSelectionMode = FaceSelection;
        } else {
            currentSelectionMode = ExtrusionSelection;
        }
    } else {
        currentSelectionMode = ExtrusionSelection;
    }

    // 进入子形状拾取前刷新绑定（与新建体后 displayOccShape 末尾一致），
    // 避免“刚建长方体就拉伸选边”时 ShapePicker 绑定不完整导致崩溃。
    if (renderer && shapePicker) {
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
    setFeatureGizmoActorsPickable(false);
    setFeatureOperationGhostMode(true);

    // 不弹出阻塞式提示，改为状态栏轻提示（避免打断操作）
    if (statusBar()) {
        QString msg = "进入拉伸选择：请在视图中点击选择";
        if (currentSelectionMode == EdgeSelection) {
            msg = "拉伸-曲线选择：请在视图中点击边/曲线";
        } else if (currentSelectionMode == FaceSelection) {
            msg = "拉伸-面片选择：请在视图中点击面";
        } else {
            msg = "拉伸-模型选择：请在视图中点击模型";
        }
        statusBar()->showMessage(msg, 3000);
    }

}

// 清除拉伸选择
void Widget::onExtrusionClearSelection()
{
    ++extrudeRevolveSelectionEpoch_;
    extrusionSelectedIndices.clear();
    extrusionSelectedFaces.clear();
    clearExtrusionSelectionHighlight();
    clearExtrusionFaceHighlight();
    clearFeatureLivePreview();
    clearExtrusionHandles();

    ExtrusionDialog *dialog = qobject_cast<ExtrusionDialog*>(sender());
    if (dialog) {
        dialog->setSelectedGeometryCount(0);
    }
}

// 处理拉伸选择模式改变
void Widget::onExtrusionSelectionModeChanged(const QString& mode)
{
    if (mode == "edge") {
        currentSelectionMode = EdgeSelection;
    } else if (mode == "face") {
        currentSelectionMode = FaceSelection;
    }
    // 清除之前的选择
    ++extrudeRevolveSelectionEpoch_;
    extrusionSelectedFaces.clear();
    clearExtrusionFaceHighlight();
    clearFeatureLivePreview();
    clearExtrusionHandles();
    
    // 更新对话框中的选中计数
    ExtrusionDialog *dialog = qobject_cast<ExtrusionDialog*>(sender());
    if (dialog) {
        dialog->setSelectedGeometryCount(0);
    }

    if (renderer && shapePicker) {
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
    setFeatureGizmoActorsPickable(false);
    // 已进入幽灵模式时，切换截面类型后重新刷一遍（避免被 clear 掉）
    if (featureOperationGhostMode_) {
        setFeatureOperationGhostMode(true);
    }
}
// 处理预览请求：锁定并显示接近最终结果的实体
void Widget::onExtrusionPreviewRequested()
{
    enterFeatureResultPreview(true);
}

void Widget::onExtrusionCancelPreviewRequested()
{
    leaveFeatureResultPreview(true);
}

// 带预览的拉伸操作
void Widget::performExtrusionWithPreview(ExtrusionDialog* dialog)
{
    if (!dialog || extrusionSelectedIndices.isEmpty()) {
        return;
    }

    try {
        // 清除之前的预览
        if (previewActor) {
            removeSceneActor(previewActor);
            previewActor = nullptr;
        }

        // 获取对话框参数
        double startDistance = dialog->getStartDistance();
        double endDistance = dialog->getEndDistance();

        double actualDistance = endDistance - startDistance;
        gp_Dir extrudeDir = getExtrusionDirection(dialog);
        gp_Vec extrudeVec(extrudeDir);
        extrudeVec.Scale(actualDistance);
        const bool makeSheetBody = dialog->isSheetBodyType();

        // 创建装配体来组合所有拉伸结果
        TopoDS_Compound previewCompound;
        BRep_Builder compoundBuilder;
        compoundBuilder.MakeCompound(previewCompound);

        bool hasValidResult = false;

        // 对每个选中的几何体执行拉伸
        for (int index : extrusionSelectedIndices) {
            TopoDS_Shape originalShape = getShapeFromHistory(index);
            if (originalShape.IsNull()) {
                continue;
            }

            // 执行拉伸
            BRepPrimAPI_MakePrism prism(originalShape, extrudeVec);
            if (!prism.IsDone()) {
                continue;
            }

            TopoDS_Shape resultShape = prism.Shape();
            compoundBuilder.Add(previewCompound, resultShape);
            hasValidResult = true;
        }

        if (!hasValidResult) {
            QMessageBox::warning(this, "警告", "无法生成预览，请检查选中的几何体！");
            return;
        }

        // 创建预览actor
        vtkSmartPointer<vtkPolyData> previewPolyData = m_occConverter.convert(previewCompound);
        if (!previewPolyData || previewPolyData->GetNumberOfPoints() == 0) {
            QMessageBox::warning(this, "警告", "预览数据转换失败！");
            return;
        }

        vtkSmartPointer<vtkPolyDataMapper> previewMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        previewMapper->SetInputData(previewPolyData);

        previewActor = vtkSmartPointer<vtkActor>::New();
        previewActor->SetMapper(previewMapper);
        previewActor->GetProperty()->SetColor(0.5, 0.5, 1.0);  // 蓝色预览
        previewActor->GetProperty()->SetOpacity(0.6);          // 半透明
        previewActor->GetProperty()->SetBackfaceCulling(false);
        previewActor->SetPickable(false);                      // 不可选择

        addAppearanceActor(previewActor);
        refreshCameraClippingRange();
        vtkWidget->renderWindow()->Render();

        QMessageBox::information(this, "预览", "拉伸预览已生成，点击确定按钮应用更改。");

    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误", QString("预览拉伸操作失败: %1").arg(e.GetMessageString()));
    }
}

// 处理拉伸选择
void Widget::handleExtrusionSelection(vtkActor* selectedActor)
{
    if (!selectedActor || currentSelectionMode != ExtrusionSelection) {
        return;
    }

    // 查找对应的历史记录索引
    int selectedIndex = resolveHistoryIndexByActor(selectedActor);

    if (selectedIndex == -1) {
        return;
    }

    // 切换选择状态
    if (extrusionSelectedIndices.contains(selectedIndex)) {
        extrusionSelectedIndices.removeOne(selectedIndex);
    } else {
        extrusionSelectedIndices.append(selectedIndex);
    }

    // 更新高亮显示
    updateExtrusionSelectionHighlight();

    // 更新对话框中的选中计数
    // 我们需要找到当前的拉伸对话框
    QWidget *parent = this;
    ExtrusionDialog *dialog = nullptr;
    while (parent) {
        dialog = parent->findChild<ExtrusionDialog*>();
        if (dialog) break;
        parent = parent->parentWidget();
    }

    if (dialog) {
        dialog->setSelectedGeometryCount(extrusionSelectedIndices.size());
    }

}

// 更新拉伸选择的高亮显示
void Widget::updateExtrusionSelectionHighlight()
{
    // 首先取消所有模型的高亮
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor) {
            QColor originalColor = historyList[i].color;
            historyList[i].actor->GetProperty()->SetColor(
                originalColor.redF(), originalColor.greenF(), originalColor.blueF());
        }
    }

    // 高亮选中的模型
    for (int index : extrusionSelectedIndices) {
        if (index >= 0 && index < historyList.size() && historyList[index].actor) {
            historyList[index].actor->GetProperty()->SetColor(0.8, 0.8, 0.2);  // 黄色高亮
        }
    }

    vtkWidget->renderWindow()->Render();
}

// 清除拉伸选择的高亮
void Widget::clearExtrusionSelectionHighlight()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (historyList[i].actor) {
            QColor originalColor = historyList[i].color;
            historyList[i].actor->GetProperty()->SetColor(
                originalColor.redF(), originalColor.greenF(), originalColor.blueF());
        }
    }
    vtkWidget->renderWindow()->Render();
}

// 执行最终的拉伸操作
void Widget::performExtrusion(ExtrusionDialog* dialog)
{
    // 检查对话框是否有效
    if (!dialog) {
        QMessageBox::warning(this, "警告", "对话框无效！");
        return;
    }
    
    // 优先使用面选择，如果没有面选择则使用模型选择（兼容旧版本）
    bool useFaceSelection = !extrusionSelectedFaces.isEmpty();
    bool useModelSelection = !extrusionSelectedIndices.isEmpty();
    
    // 只有当既没有面选择也没有模型选择时，才弹出警告
    // 注意：这个检查必须在执行拉伸操作之前进行，因为拉伸完成后会清空选择
    if (!useFaceSelection && !useModelSelection) {
        // 不弹出提示；无选择时直接返回
        if (statusBar()) {
            statusBar()->showMessage(tr("未选择拉伸对象"), 2000);
        }
        return;
    }

    try {
        // 清除预览
        if (previewActor) {
            removeSceneActor(previewActor);
            previewActor = nullptr;
        }

        // 获取对话框参数
        double startDistance = dialog->getStartDistance();
        double endDistance = dialog->getEndDistance();

        // 计算实际拉伸距离与方向（支持对话框矢量 + 反向）
        // start=起始偏移，end=终止位置，拉伸长度为 end-start
        double actualDistance = endDistance - startDistance;
        gp_Dir extrudeDir = getExtrusionDirection(dialog);
        gp_Trsf startShift;
        startShift.SetTranslation(gp_Vec(extrudeDir) * startDistance);
        gp_Vec extrudeVec(extrudeDir);
        extrudeVec.Scale(actualDistance);
        // 修复：当通过“矢量对话框（如两点）”得到 customVectorDir_ 时，
        // 即使对话框里的“选择矢量”未勾选，也应按该方向拉伸。
        bool useVectorDir = dialog->isAxisVectorSelected() || hasCustomVectorDir_
                            || dialog->isAutoVectorMode();
        const bool makeSheetBody = dialog->isSheetBodyType();
        const int boolMode = dialog->booleanMode();
        const int boolTargetIdx = dialog->booleanTargetIndex();
        if (boolMode >= 0 && (boolTargetIdx < 0 || boolTargetIdx >= historyList.size())) {
            QMessageBox::warning(this, "警告", "请先选择布尔运算的目标体！");
            return;
        }
        if (makeSheetBody && boolMode >= 0) {
            QMessageBox::warning(this, "警告", "片体类型暂不支持布尔运算，请改用实体或将布尔设为「无」。");
            return;
        }

        if (useFaceSelection) {
            // 与实时预览共用同一套几何（起止距离+布尔），避免确认结果与预览不一致
            if (std::abs(actualDistance) < 1e-9) {
                QMessageBox::warning(this, "警告", "拉伸长度为 0，无法执行！");
                return;
            }

            TopoDS_Shape resultShape;
            if (!buildExtrusionPreviewShape(dialog, resultShape, false) || resultShape.IsNull()) {
                if (boolMode >= 0) {
                    QMessageBox::warning(this, "错误",
                        QStringLiteral("布尔运算失败！请确认拉伸体已切入目标体内部（减去/求交需有重叠体积）。"));
                } else {
                    QMessageBox::warning(this, "错误", "拉伸结果为空，请检查截面与方向！");
                }
                return;
            }

            FeatureRecipe recipe;
            recipe.hasRecipe = true;
            recipe.extrusion.direction = extrudeDir;
            recipe.extrusion.lengthFwd = actualDistance;
            recipe.extrusion.startOffset = startDistance;
            recipe.extrusion.makeSheetBody = makeSheetBody;
            recipe.extrusion.useVectorDirection = useVectorDir;
            recipe.extrusion.boolOpType = boolMode;
            recipe.extrusion.boolTargetIndex = boolTargetIdx;
            for (const ExtrusionFaceSelection& sel : extrusionSelectedFaces) {
                if (sel.modelIndex < 0 || sel.modelIndex >= historyList.size()) {
                    continue;
                }
                recipe.extrusion.profiles.append(makeSubShapeRef(
                    sel.modelIndex,
                    historyList[sel.modelIndex].occShape,
                    sel.shape,
                    sel.shapeType,
                    sel.subShapeId));
                if (!recipe.parentIndices.contains(sel.modelIndex)) {
                    recipe.parentIndices.append(sel.modelIndex);
                }
            }

            QList<int> hideSources = recipe.parentIndices;
            if (boolMode >= 0 && boolTargetIdx >= 0 && !hideSources.contains(boolTargetIdx)) {
                hideSources.append(boolTargetIdx);
            }

            const QString afterName = (boolMode >= 0)
                ? QStringLiteral("拉伸(布尔,距离=%1)").arg(actualDistance)
                : QStringLiteral("拉伸(距离=%1)").arg(actualDistance);

            executeCommand(new AddModelCommand(
                this, resultShape, afterName, EXTRUSION, QColor(255, 140, 0),
                actualDistance, 0.0, 0.0, hideSources, !hideSources.isEmpty(), &recipe));

            extrusionSelectedFaces.clear();
            clearExtrusionFaceHighlight();
            currentSelectionMode = None;

        } else {
            // 使用模型选择进行拉伸（兼容旧版本）
            for (int index : extrusionSelectedIndices) {
                TopoDS_Shape originalShape = getShapeFromHistory(index);
                if (originalShape.IsNull()) {
                    QMessageBox::warning(this, "错误",
                                         QString("无法获取模型\"%1\"的形状数据！").arg(historyList[index].name));
                    continue;
                }

                // 实体模式下：若是草图线框/边，优先构面后再拉伸，避免固定生成片体
                TopoDS_Shape shapeToExtrude = originalShape;
                if (!makeSheetBody) {
                    TopoDS_Wire wireForFace;
                    bool hasWire = false;

                    if (originalShape.ShapeType() == TopAbs_WIRE) {
                        wireForFace = TopoDS::Wire(originalShape);
                        hasWire = !wireForFace.IsNull();
                    } else {
                        BRepBuilderAPI_MakeWire wireMaker;
                        for (TopExp_Explorer ex(originalShape, TopAbs_EDGE); ex.More(); ex.Next()) {
                            wireMaker.Add(TopoDS::Edge(ex.Current()));
                        }
                        if (wireMaker.IsDone()) {
                            wireForFace = wireMaker.Wire();
                            hasWire = !wireForFace.IsNull();
                        }
                    }

                    if (hasWire) {
                        ShapeFix_Wire fixer;
                        fixer.Load(wireForFace);
                        fixer.FixReorder();
                        fixer.FixConnected();
                        fixer.FixClosed();
                        TopoDS_Wire fixedWire = fixer.Wire();

                        BRepBuilderAPI_MakeFace faceMaker(fixedWire);
                        if (faceMaker.IsDone()) {
                            shapeToExtrude = faceMaker.Face();
                        } else {
                            shapeToExtrude = fixedWire;
                        }
                    }
                }

                // 执行拉伸（与面选择一致：先起始偏移再扫掠）
                BRepBuilderAPI_Transform mover(shapeToExtrude, startShift, true);
                BRepPrimAPI_MakePrism prism(mover.Shape(), extrudeVec);
                if (!prism.IsDone()) {
                    QMessageBox::warning(this, "错误",
                                         QString("模型\"%1\"拉伸操作失败！").arg(historyList[index].name));
                    continue;
                }

                TopoDS_Shape resultShape;
                if (!applyDialogBooleanToShape(boolMode, boolTargetIdx, prism.Shape(), resultShape)
                    || resultShape.IsNull()) {
                    QMessageBox::warning(this, "错误",
                                         QString("模型\"%1\"拉伸或布尔运算失败！").arg(historyList[index].name));
                    continue;
                }

                QString originalName = historyList[index].name;
                QString resultName = QString("拉伸(距离=%1)_%2").arg(actualDistance).arg(originalName);
                QColor resultColor = QColor(255, 140, 0);

                FeatureRecipe recipe = buildModelExtrusionRecipe(
                    index, extrudeDir, actualDistance, makeSheetBody, useVectorDir);
                recipe.extrusion.startOffset = startDistance;
                recipe.extrusion.boolOpType = boolMode;
                recipe.extrusion.boolTargetIndex = boolTargetIdx;
                QList<int> hideSources;
                if (boolMode >= 0 && boolTargetIdx >= 0) {
                    hideSources.append(boolTargetIdx);
                } else {
                    hideSources.append(index);
                }
                executeCommand(new AddModelCommand(this, resultShape, resultName, EXTRUSION, resultColor, actualDistance,
                                                   0.0, 0.0, hideSources, true, &recipe));
            }

            // 拉伸完成后，清除选择并重置选择模式
            extrusionSelectedIndices.clear();
            clearExtrusionSelectionHighlight();
            currentSelectionMode = None;  // 重置选择模式，恢复视角旋转
        }


    } catch (Standard_Failure& e) {
        QMessageBox::critical(this, "错误",
                              QString("拉伸操作异常: %1").arg(e.GetMessageString()));
    }
}
