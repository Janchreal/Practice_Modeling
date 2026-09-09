#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H
#define _USE_MATH_DEFINES
#include <cmath>

#include <QMainWindow>
#include <QDockWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QInputDialog>
#include <QDateTime>
#include <QColor>
#include <QDialog>
#include <QList>
#include <QMouseEvent>
#include <QMessageBox>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointer>
#include <functional>
#include <QStackedWidget>
#include <QVector>

// 方向选择（标准轴）
#include "common/axisdirection.h"
#include "geometry/placement/axis_placement.h"
#include "domain/features/patternfeaturetypes.h"
#include "application/history/model_history_snapshot.h"

// OpenCASCADE 头文件
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Vec.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Compound.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>

//VTK
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkFollower.h>
#include <vtkVectorText.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkProp.h>
// VIS（ShapePicker 等会用到 vtkSmartPointer<vtkRenderer>，需完整类型）
#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_ShapePicker.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>
#include <IVtkTools.hxx>

// OCCT
#include <IVtk_Types.hxx>
#include <TColStd_PackedMapOfInteger.hxx>
#include <vtkFeatureEdges.h>

// 包含命令相关的头文件
#include "presentation/dialogs/extrude_revolve/extrusion_dialog.h"
#include "presentation/dialogs/extrude_revolve/revolve_dialog.h"
#include "application/ports/modeling_command_port.h"
#include "application/history/commandmanager.h"
#include "application/commands/creategeometrycommand.h"
#include "application/commands/booleancommand.h"
#include "common/modeltype.h" // 包含ModelType定义
#include "presentation/dialogs/pattern/pattern_feature_dialog.h"
#include "domain/features/featurerecipe.h"
#include "application/history/model_document.h"
#include "application/history/modeling_history_record.h"
#include "application/commands/deletemodelcommand.h"
#include "rendering/adapters/occvtkconverter.h"
#include "geometry/runtime/model_geometry_store.h"
#include "rendering/model/model_render_store.h"
#include "viewport/main_view/mouse_interactor.h"
#include "geometry/primitives/primitive_build_request.h"
#include "rendering/pipeline/render_pipeline.h"
#include "domain/sketch/sketch.h"
#include "presentation/dialogs/sketch/sketch_tool_input_dialog.h"
#include "presentation/dialogs/sketch/sketch_polygon_dialog.h"
#include "presentation/dialogs/sketch/sketch_ellipse_dialog.h"

class SketchRectangleModeDialog;
class SketchCircleModeDialog;
class SketchConicDialog;
class SketchPolygonValueDialog;
class SketchEllipseAngleDialog;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

struct MirrorRenderContext;

//VTK前向声明（vtkRenderer/vtkActor/vtkPolyData 已在上方包含对应头文件）
class QVTKOpenGLNativeWidget;
class HistoryListItem;
class Widget;
class BoolOperationDialog;
class CuboidParamsDialog;
class CylinderDialog;
class ConeParamsDialog;
class SphereParamsDialog;
class filletdialog;
class chamferdialog;
class QPushButton;
class QGroupBox;
class QToolButton;
class QMenu;

class Widget : public QMainWindow, public ModelingCommandPort
{
    Q_OBJECT
    friend class MouseInteractorStyle; // 添加友元声明
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    /** 为渲染器配置主光/补光/环境光（关闭默认自动建灯）；镜像窗口也可调用 */
    static void configureSceneLights(vtkRenderer* ren);
    /** 捕捉/确认点球材质：Confirmed=翠绿确认，Hover=琥珀悬停 */
    enum class MarkerSphereStyle { ConfirmedGreen, HoverYellow };
    static void applyMarkerSphereMaterial(class vtkProperty* prop, MarkerSphereStyle style);
    /** 高细分球体源（局部原点），配合 Position+Scale 使用 */
    static void configureMarkerSphereSource(class vtkSphereSource* sphere, double radius);

    /** 草图几何捕捉：屏幕空间候选（与全局捕捉合并）*/
    struct SketchSnapScreenCandidate {
        QString label;
        gp_Pnt p;
        double d2 = 0.0;
        TopoDS_Shape refShape;
        bool hasRef = false;
    };
    /** pickSnapAt：草图工具内（SketchTool）时不弹“最近点”列表、不追加常驻绿点 */
    enum class SnapPickContext { Normal, SketchTool };

    // 将 handleVtkMouseClick 设为公共
    void handleVtkMouseClick(int x, int y);
    // 获取最近一次点击的世界坐标
    void getLastWorldPoint(double worldPoint[4]) const;
    // 布尔运算选择状态
    enum SelectionMode {
        None,
        SelectTarget,
        SelectTool,
        ExtrusionSelection,  // 添加拉伸选择模式
        FaceSelection,    // 面片选择
        EdgeSelection,     // 曲线选择
        FilletEdgeSelection, // 倒圆角：边选择
        FilletRadiusHandleDrag, // 倒圆角：两侧半径手柄拖拽（同步）
        ChamferEdgeSelection, // 倒角（对称）：边选择
        ChamferAsymHandleDrag, // 倒角（非对称）：两侧距离手柄拖拽
        PointSelection,    // 点选择模式（用于指定原点）
        VectorDialogPickDirection, // 矢量方向拾取（悬停自动识别并点击确认）
        VectorDialogPickStartPoint, // 矢量：起点拾取
        VectorDialogPickEndPoint,   // 矢量：终点拾取
        VectorTwoPointInteractive,  // 两点矢量：起终点已确定，可拖拽/重选
        VectorTwoPointHandleDrag,   // 两点矢量：手柄拖拽中
        WorkCsysPlacement, // 工作坐标系放置
        WorkCsysDrag,      // 工作坐标系拖拽
        SketchPlaneSelection,   // 草图：拾取参考平面
        SketchDrawLine,         // 草图：画直线（两点）
        SketchDrawArc,          // 草图：画圆弧（三点或接续相切）
        SketchDrawRectangle,    // 草图：画矩形
        SketchDrawCircle,       // 草图：圆
        SketchDrawPoint,        // 草图：点（十字）
        SketchDrawPolygon,      // 草图：正多边形（中心+尺寸点）
        SketchPolygonPick,      // 草图：多边形对话框拾取点
        SketchEllipsePick,      // 草图：椭圆对话框拾取点
        SketchEllipseAdjust,    // 草图：椭圆已创建后调整旋转
        SketchConicPick,        // 草图：二次曲线拾取点
        SketchConicDragControl, // 草图：二次曲线拖动控制点
        SketchQuickTrim,        // 草图：快速修剪
        SketchQuickExtend,      // 草图：快速延伸
        CuboidInteractive,      // 长方体：交互式预览与 Gizmo
        PatternBodySelection,   // 阵列：选择体
        PatternPitchInteractive, // 阵列：节距拖拽
        ExtrusionHandleDrag,    // 拉伸：起止距离手柄拖拽
        RevolveHandleDrag,      // 旋转：起止角度手柄拖拽
        FeatureBooleanTargetSelect // 拉伸/旋转：选择布尔目标体
    };
    // 检查是否处于选择模式
    bool isInSelectionMode() const {
        return currentSelectionMode != None;
    }
    // 获取当前选择模式
    SelectionMode getCurrentSelectionMode() const {
        return currentSelectionMode;
    }
    /** 草图鼠标移动结束后刷新捕捉悬停（内部根据是否已武装捕捉决定 update/clear）*/
    void refreshSnapHoverAfterSketchMouseMove(int x, int y);
    // 添加这些公共函数供表达式对话框访问
    const QList<ModelingHistory>& getHistoryList() const { return modelDocument_.histories(); }
    void updateModelParameter(const QString& paramName, double newValue);

    //添加的槽函数
private slots:
    // 文件（文档）相关
    void handleNewFile();
    void handleOpenFile();
    void handleSaveFile();
    void handleSaveFileAs();

    void on_cuboid_clicked();  //点击长方体函数
    void on_sphere_clicked();  //点击球体函数
    void on_cylinder_clicked();  //点击圆柱体函数
    void on_cone_clicked();  //点击圆锥体函数
    void on_boolOperationButton_clicked();  // 布尔运算按钮点击事件
    void handleHistoryItemClicked(QListWidgetItem *item);  // 历史记录点击事件（已弃用）
    void handleClearHistory();  // 清空历史记录
    void onDeleteHistoryItem(int index);  // 删除单个历史记录项
    void on_expressionBtn_clicked();//表达式按钮
    //拉伸、旋转、倒角、挖空
    void on_extrude_clicked();
    void on_revolve_clicked();
    void on_fillet_clicked();
    void on_chamfer_clicked();
    void on_patternFeature_clicked();
    void handleHollow();
    // 操作执行函数
    //void performExtrusion(double distance, const gp_Dir& direction);
    void performRevolution(double angle, const gp_Ax1& axis);
    void performHollow(double thickness);
    // 辅助函数
    bool isValidShapeForOperation(int index);
    // 布尔运算相关槽函数
    void onBoolSelectionTarget();
    void onBoolSelectionTool();
    void onBoolOperationConfirmed();
    // 显示所有模型
    void showAllModels();
    //拉伸相关
    void onExtrusionStartSelection();
    void onExtrusionClearSelection();
    void onExtrusionPreviewRequested();
    void onExtrusionCancelPreviewRequested();
    void onExtrusionSelectionModeChanged(const QString& mode);
    // 旋转相关
    void onRevolveStartSelection();
    void onRevolveClearSelection();
    void onRevolvePreviewRequested();
    void onRevolveCancelPreviewRequested();
    void onRevolveSelectionModeChanged(const QString& mode);
    //重做和撤销
    void on_undoButton_clicked();
    void on_redoButton_clicked();
    void on_WindowpushButton_clicked();
    // 新添加的槽函数
    void createNewWindow();
    void showWindowLayoutDialog();
    void resetWindowLayout();
    void switchActiveWindow();
    void changeDisplayWidget();
    // Dock widget 位置交换相关
    void onDockWidgetLocationChanged(Qt::DockWidgetArea area);
    // 基准坐标系（工作坐标系）按钮
    void on_workAxisButton_clicked();
    // 基准轴按钮
    void on_pushButton_4_clicked();
    // 基准平面按钮
    void on_datum_plane_Button_clicked();
    void on_pushButton_6_clicked();
    void on_createSketchButton_clicked();

    // 草图相关
    void on_pushButton_5_clicked();   // 在任务环境中创建草图
    void on_pushButton_7_clicked();   // 轮廓（直线/圆弧综合）
    void on_pushButton_40_clicked();  // 直线（轮廓快捷）
    void on_pushButton_41_clicked();  // 长方体草图（矩形）
    void on_pushButton_42_clicked();  // 圆弧（轮廓快捷）
    void on_pushButton_11_clicked();  // 点
    void on_pushButton_12_clicked();  // 圆
    void on_pushButton_13_clicked();  // 二次曲线
    void on_pushButton_9_clicked();   // 多边形
    void on_pushButton_10_clicked();  // 椭圆
    void startSketchQuickTrim();
    void startSketchQuickExtend();

signals:
    void undoStateChanged(bool canUndo, bool canRedo);

public slots:
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;


private:
    Ui::Widget *ui;
    // 文件（文档）状
    QString currentDocumentPath_;
    bool documentModified_ = false;
    bool isLoadingDocument_ = false;

    void markDocumentModified(bool modified = true);
    QWidget* dialogParentWidget() const;
    void updateDocumentWindowTitle();
    bool maybeSaveDocument();
    bool saveDocumentToPath(const QString& filePath);
    bool saveDocument(bool forceSaveAs = false);
    bool loadDocumentFromPath(const QString& filePath);
    void clearAllModelsInternal();
    void addRecentFile(const QString& filePath);
    void saveRecentFilesToSettings() const;
    void loadRecentFilesFromSettings();
    QString autoRecoveryFilePath() const;
    void saveAutoRecoverySnapshot();
    void removeAutoRecoverySnapshot();
    void tryRecoverFromAutoSnapshotOnStartup();
    void setupRecentFilesUi();
    void refreshRecentFilesUi();
    void openRecentFileAt(int index);
    void rebuildOpenRecentMenu();
    void clearRecentFiles();

    QList<QString> recentFiles_;
    QTimer* autoSaveTimer_ = nullptr;

    /** 标准视图/三重轴切换的相机过渡 */
    struct ViewCameraPose {
        double pos[3] = {0, 0, 0};
        double fp[3] = {0, 0, 0};
        double up[3] = {0, 1, 0};
        double parallelScale = 1.0;
        bool parallel = false;
    };
    QTimer* viewAnimTimer_ = nullptr;
    QElapsedTimer viewAnimClock_;
    ViewCameraPose viewAnimFrom_{};
    ViewCameraPose viewAnimTo_{};
    bool viewTransitionAnimating_ = false;
    static constexpr int kViewAnimDurationMs_ = 380;
    void captureCameraPose(class vtkCamera* camera, ViewCameraPose& out) const;
    void applyCameraPose(class vtkCamera* camera, const ViewCameraPose& pose) const;
    void applyStandardViewCameraImmediate(const QString& normalizedViewName);
    void beginViewTransitionAnimation(const ViewCameraPose& from, const ViewCameraPose& to);
    void onViewTransitionAnimTick();
    void animateCameraToPose(const std::function<void()>& applyTargetImmediate);
    QGroupBox* recentFilesGroup_ = nullptr;
    QVector<QPushButton*> recentFileButtons_;
    QToolButton* openRecentMenuButton_ = nullptr;
    QMenu* openRecentMenu_ = nullptr;

    void syncMirrorWindows();
    void syncMainCameraFromWindowRenderer(vtkRenderer* sourceRenderer);
    void activateMainRenderContext();
    void activateMirrorRenderContext(QObject* windowKey);
    void prepareShapePickerBindingsForCurrentContext();
    void refreshShapePickerBindingsForCurrentContext(double tolerance = 0.05, bool updateDataSources = true);
    int resolveHistoryIndexByActor(vtkActor* actor) const;
    bool handleMirrorTriadClick(int x, int y);
    MirrorRenderContext* mirrorContextForVtkWidget(QVTKOpenGLNativeWidget* w);

    //VTK的相关成员
    QVTKOpenGLNativeWidget *vtkWidget;
    vtkSmartPointer<vtkRenderer> renderer;  // 改为智能指针
    vtkSmartPointer<vtkOrientationMarkerWidget> axesWidget;  // 坐标轴小部件

    /**
     * 渲染层级（与规划一致）：
     *  L0 renderer                         — 主渲染：模型默认样式 + 框架轮廓
     *  L1 RenderPipeline appearance layer  — 覆盖外观：高亮/选中/预览
     *  L2 RenderPipeline reference layer   — 参考：矢量/点操作柄、坐标系轴
     *  L3 centerAxesRenderer               — 视图三重轴
     */
    RenderPipeline renderPipeline_;
    vtkSmartPointer<vtkRenderer> centerAxesRenderer;
    void configureReferenceOverlayAlwaysOnTop();
    void configureFeatureSelectionOverlayAlwaysOnTop();
    vtkRenderer* appearanceOverlay();   // L1
    vtkRenderer* referenceOverlay();    // L2
    void addAppearanceActor(vtkProp* prop);
    void addReferenceActor(vtkProp* prop);
    void removeSceneActor(vtkProp* prop); // 从 L0/L1/L2 安全移除
    vtkSmartPointer<vtkActor> centerAxisXActor;
    vtkSmartPointer<vtkActor> centerAxisYActor;
    vtkSmartPointer<vtkActor> centerAxisZActor;
    vtkSmartPointer<vtkActor> centerTriadFaceActors_[6];
    vtkSmartPointer<vtkActor> centerTriadEdgeActors_[12];
    int centerTriadHoveredFace_ = -1;
    int centerTriadHoveredEdge_ = -1;
    int centerTriadCurrentFace_ = -1;

    AxisDirection currentAxisDirection = AxisDirection::Z;
    double centerAxesBaseScale = 1.2;          // 箭头几何基础缩放（稍大一些）
    double centerAxesCurrentScale = 1.0;       // 固定整体缩放（不随滚轮变化）
    double centerAxesCameraDistance = 5.0;     // 中心轴相机到原点的固定距离

    // 工作坐标系（可移动）
    vtkSmartPointer<vtkActor> workCsysActor;
    vtkSmartPointer<vtkTransform> workCsysTransform;
    bool hasWorkCsys = false;
    bool workCsysDragActive = false;
    int workCsysHistoryIndex_ = -1;
    double workCsysLastPickWorld[3] = {0.0, 0.0, 0.0};

    // 工作坐标系：三轴箭头与标签（点击可选矢量）
    vtkSmartPointer<vtkActor> workCsysAxisXActor_;
    vtkSmartPointer<vtkActor> workCsysAxisYActor_;
    vtkSmartPointer<vtkActor> workCsysAxisZActor_;
    vtkSmartPointer<vtkActor> workCsysLabelXActor_;
    vtkSmartPointer<vtkActor> workCsysLabelYActor_;
    vtkSmartPointer<vtkActor> workCsysLabelZActor_;
    bool ensureWorkCsysActorsCreated();
    void setWorkCsysVisible(bool visible);
    bool handleWorkCsysAxisPick(int x, int y);
    void applyAxisDirectionHighlight(AxisDirection dir);

    // 默认参考坐标系（原点）：VTK 三轴箭头 + 三主平面；轴供矢量选择，平面点击切视图
    vtkSmartPointer<vtkActor> refCsysActor_;
    vtkSmartPointer<vtkTransform> refCsysTransform_;
    bool hasReferenceCsys_ = false;
    int referenceCsysHistoryIndex_ = -1;
    vtkSmartPointer<vtkActor> refCsysAxisXActor_;
    vtkSmartPointer<vtkActor> refCsysAxisYActor_;
    vtkSmartPointer<vtkActor> refCsysAxisZActor_;
    vtkSmartPointer<vtkActor> refCsysLabelXActor_;
    vtkSmartPointer<vtkActor> refCsysLabelYActor_;
    vtkSmartPointer<vtkActor> refCsysLabelZActor_;
    vtkSmartPointer<vtkActor> refCsysPlaneXYActor_; // 法向 Z
    vtkSmartPointer<vtkActor> refCsysPlaneYZActor_; // 法向 X
    vtkSmartPointer<vtkActor> refCsysPlaneXZActor_; // 法向 Y
    vtkSmartPointer<vtkActor> refCsysPlaneFrameActor_; // XY/YZ/XZ 平面闭合方框
    bool ensureReferenceCsysActorsCreated();
    void setReferenceCsysVisible(bool visible);
    bool handleReferenceCsysAxisPick(int x, int y);
    bool handleReferenceCsysPlanePick(int x, int y);
    void applyReferenceAxisHighlight(AxisDirection dir);
    void resetReferenceAxisHighlight();
    void applyReferencePlaneHighlight(int planeId);
    void resetReferencePlaneHighlight();
    void updateReferenceCsysAxisHover(int x, int y);
    bool pickReferenceCsysAxisAt(int x, int y, AxisDirection& outAxis) const;
    bool pickReferenceCsysPlaneAt(int x, int y, int& outPlaneId) const;
    QString viewNameForReferenceCsysPlane(int planeId) const;
    void initDefaultReferenceCsys();
    void ensureDefaultReferenceCsysIfMissing();
    void clearReferenceCsysState();
    void refreshReferenceCsysScreenScale();
    bool isVectorAxisPickContext() const;
    void applyVectorDirFromDatumAxis(const gp_Dir& baseDir);
    // 最近一次点击的世界坐标
    double lastWorldPoint[4];  // 存储最近一次点击的世界坐标 (x, y, z, w)
    // 历史记录相关
    ModelDocument modelDocument_;
    ModelGeometryStore geometryStore_;
    ModelRenderStore renderStore_;
    QList<ModelingHistory>& historyList;
    void addToHistory(ModelType type, const QString& name, vtkSmartPointer<vtkActor> actor, const QColor& color, double param1, double param2,double param3,vtkSmartPointer<vtkPolyData> polyData,const TopoDS_Shape& occShape = TopoDS_Shape(), Handle(IVtkOCC_Shape) shapeWrapper = nullptr, vtkSmartPointer<IVtkTools_ShapeDataSource> dataSource = nullptr);
    void updateHistoryList();
    void updateHistoryListSelection();
    void highlightModel(int index);
    void updateModelHoverHighlight(int x, int y);
    void clearModelHoverHighlight();
    // 特征树相关
    void updateFeatureTree();  // 更新特征树显示
    void onFeatureTreeItemClicked(QTreeWidgetItem* item, int column);  // 单击事件
    void onFeatureTreeItemDoubleClicked(QTreeWidgetItem* item, int column);  // 双击事件
    void onFeatureTreeVisibilityChanged(int index, bool visible);  // 可见性改变
    void onFeatureTreeContextMenuRequested(const QPoint& pos);  // 右键菜单：隐藏/显示、删除、修改名称
    // 视图切换和右键菜单相关
    void switchToView(const QString& viewName);
    void applyInitialSceneView();
    void refreshCameraClippingRange();
    /** 覆盖层(L1/L2)使用独立相机：位姿跟随主相机，裁切各自计算，避免长预览毁掉主场景深度精度 */
    void syncOverlayCameras();
    void alignViewToSketchPlane(const gp_Pln& plane);
    /** 中断视图过渡动画（用户手动旋转/平移/缩放时） */
    void stopViewTransitionAnimation();
    void setupToolbarModeStack();
    void setupAppRibbon();
    void enterSketchEnvironment();
    void exitSketchEnvironment();
    int pickModelAtPosition(int x, int y);
    /** ShapePicker 失败时：网格/可见像素拾取 + OCC 射线；禁止大半径空白吸附 */
    /** 右键菜单等：允许少量邻近容差的稳健拾取 */
    int pickHistoryModelFallback(int x, int y) const;
    /** 左键整模高亮/空白取消：严格像素命中，禁止邻近吸附（避免点空白也高亮） */
    int pickHistoryModelStrict(int x, int y) const;
    void showContextMenu(int x, int y, int modelIndex, const QPoint& globalPos);
    void editModelFromContextMenu(int index);
    void deleteModelFromContextMenu(int index);
    void toggleModelVisibilityFromContextMenu(int index);
    void createCylinder(double radius, double height);
    void setupVTK();
    /** 实体模型黑色边界轮廓（IVtk 线框边，框架管线不可拾取）；草图等线框跳过 */
    void ensureModelBoundaryOutline(int index);
    void styleModelBoundaryOutline(int index, bool selectedHighlight);
    void refreshAllModelBoundaryOutlines();
    /** 旋转轴矢量箭头起点：用户指定的旋转轴基点 */
    gp_Pnt revolveVectorArrowOrigin() const;
    void setupCoordinateAxes();  // 设置坐标轴
    void setupCenterAxisSelector(); // 设置屏幕中心矢量选择器
    bool handleCenterAxisPick(int x, int y); // 返回是否命中中心轴
    void setupCenterTriadCube();
    void updateCenterTriadHover(int x, int y);
    int triadFaceFromViewName(const QString& viewName) const;
    QString viewNameFromTriadFace(int faceId) const;
    void refreshCenterTriadFaceStyle();
    void updateCenterTriadViewport(bool rightBottom);
     void syncCenterAxisCamera();             // 让中心轴相机跟随主相机旋转/缩放
    /**
     * 叠加物（拾取球/手柄/矢量箭头）屏幕尺寸恒定缩放：
     * 世界尺寸 ∝ 相机到该点的视线深度，使滚轮 Dolly 后像素大小不变。
     */
    double overlayWorldScale() const;
    double overlayWorldScaleAt(double x, double y, double z) const;
    void refreshOverlayScreenScale();
    /** 与 applyInitialSceneView 初始视距一致；亦会在首次布局时按实际距离校正 */
    double overlayScaleRefDistance_ = 14.0;
    double overlayScaleRefParallel_ = 1.0;
    gp_Dir getExtrusionDirection(ExtrusionDialog* dialog) const;  // 根据对话框与当前三重轴得到拉伸方向
    gp_Ax1 getRevolutionAxis(revolvedialog* dialog) const;        // 根据对话框与当前选点/轴得到旋转轴
    bool inferDirectionFromExtrusionSelection(gp_Dir& outDir, gp_Pnt* outOrigin = nullptr) const;
    void applyAutoVectorFromSelection();
    void refreshExtrusionLivePreview();
    void refreshRevolveLivePreview();
    void clearFeatureLivePreview();
    /** 清空拉伸/旋转正在选择的剖面、面或边，并同步高亮、预览与手柄 */
    void clearExtrudeRevolveProfileSelection(bool showStatusMessage = false);
    /** 预览按钮：显示接近最终结果的实体外观（不透明），并锁定对话框 */
    void showFeatureResultPreviewShape(const TopoDS_Shape& shape, const QColor& color);
    void enterFeatureResultPreview(bool isExtrusion);
    void leaveFeatureResultPreview(bool isExtrusion);
    /** 关闭拉伸/旋转对话框时统一清理：选中数据、红边高亮、幽灵模式、手柄/预览 */
    void cleanupExtrudeRevolveDialogSession();
    void hideModelsForResultPreview();
    void restoreModelsAfterResultPreview();
    /** 拉伸/旋转会话中：模型半透明 + 橙色轮廓线；onlyModelIndex>=0 时仅对该模型生效 */
    void setFeatureOperationGhostMode(bool on, int onlyModelIndex = -1);
    void applyFeatureGhostStyleToModel(int index);
    void refreshFilletLivePreview();
    void refreshChamferLivePreview();
    bool buildFilletPreviewShape(TopoDS_Shape& outShape) const;
    bool buildChamferPreviewShape(TopoDS_Shape& outShape) const;
    void showFeatureLivePreviewShape(const TopoDS_Shape& shape);
    void syncFilletChamferGhostAndPreview(bool isFillet);
    void updateExtrusionHandles();
    void clearExtrusionHandles();
    void updateRevolveHandles();
    void clearRevolveHandles();
    void handleExtrusionHandleMouseMove(int x, int y);
    void handleExtrusionHandleMouseDown(int x, int y);
    void handleExtrusionHandleMouseUp(int x, int y);
    void handleRevolveHandleMouseMove(int x, int y);
    void handleRevolveHandleMouseDown(int x, int y);
    void handleRevolveHandleMouseUp(int x, int y);
    /** OCC shapePicker 拾取前临时关闭手柄/预览拾取，防止误命中无 ShapeSource 的 Actor 崩溃 */
    void setFeatureGizmoActorsPickable(bool pickable);
    bool buildExtrusionPreviewShape(ExtrusionDialog* dialog, TopoDS_Shape& outShape,
                                    bool softBooleanFallback = false) const;
    bool buildRevolvePreviewShape(revolvedialog* dialog, TopoDS_Shape& outShape,
                                  bool softBooleanFallback = false) const;
    bool computeExtrusionProfileCenter(gp_Pnt& outCenter) const;
    vtkSmartPointer<vtkActor> cylinderActor;  // 这个声明必须存在
     QString getTypeName(ModelType type);
    // 当前选中的模型索引
    int currentSelectedIndex;
    int hoveredModelIndex_ = -1;
    int hoverBaseSelectedIndex_ = -2;
 private:
     // OpenCASCADE 转换器
    OccShapeToVtkConverter m_occConverter;

     // ===== 草图状态 =====
     bool hasActiveSketch_ = false;
     Sketch activeSketch_;
     gp_Pln activeSketchPlane_;
     int activeSketchHistoryIndex_ = -1; // historyList 内的索引（草图曲线）
     class SketchCreateDialog* activeSketchCreateDialog_ = nullptr; // 创建草图对话框（用于拾取回写）
     int sketchClickCount_ = 0;
     gp_Pnt sketchP1_, sketchP2_, sketchP3_;
     bool sketchChainTangentValid_ = false;
     gp_Dir sketchChainTangentDir_;
     /// 仅「轮廓」工具链式续接（上一终点为下一起点）；独立直线/圆弧/圆等不续接。
     bool sketchContourChaining_ = false;
     // 草图：拾取平面悬浮高亮 + 选中平面显示
     vtkSmartPointer<vtkActor> sketchPlaneHoverActor_;
     vtkSmartPointer<vtkActor> sketchSelectedPlaneFillActor_;
     vtkSmartPointer<vtkActor> sketchSelectedPlaneOutlineActor_;
     int sketchSelectedPlaneHistoryIndex_ = -1; // 自动创建的透明基准平面（历史索引）

     // 草图：动态预览（直线/矩形/圆弧）
     vtkSmartPointer<vtkActor> sketchPreviewLineActor_;
     vtkSmartPointer<class vtkLineSource> sketchPreviewLineSource_;
     vtkSmartPointer<vtkActor> sketchPreviewRectangleActor_;
     vtkSmartPointer<vtkPolyData> sketchPreviewRectanglePolyData_;
     vtkSmartPointer<vtkActor> sketchPreviewArcActor_;
     vtkSmartPointer<vtkPolyData> sketchPreviewArcPolyData_;
     vtkSmartPointer<vtkActor> sketchPreviewCircleActor_;
     vtkSmartPointer<vtkPolyData> sketchPreviewCirclePolyData_;
     vtkSmartPointer<vtkActor> sketchPreviewPolygonActor_;
     vtkSmartPointer<vtkPolyData> sketchPreviewPolygonPolyData_;
     bool sketchPreviewPolygonGuideDashed_ = false;
     vtkSmartPointer<vtkActor> sketchPreviewConicActor_;
     vtkSmartPointer<vtkPolyData> sketchPreviewConicPolyData_;
     vtkSmartPointer<vtkActor> sketchConicMarkerActors_[3];
     vtkSmartPointer<vtkSphereSource> sketchConicMarkerSpheres_[3];
     vtkSmartPointer<vtkActor> sketchEditHoverActor_;
     QList<vtkSmartPointer<vtkActor>> sketchCommittedOverlayActors_;
     bool sketchBrushActive_ = false;
     TopoDS_Shape sketchLastBrushShape_;

     bool tryPickPointOnPlane(const gp_Pln& pln, int x, int y, gp_Pnt& outP) const;
     bool tryPickFacePlaneUnderCursor(int x, int y, gp_Pln& outPlane);
     bool tryPickPlanarFaceUnderCursor(int x, int y, gp_Pln& outPlane, TopoDS_Face& outFace);
     void ensureSketchHistoryRecord();
     void updateSketchHistoryShape();
    void rebindHistoryShapeSource(ModelingHistory& history);
    ModelGeometryState& geometryStateFor(const ModelingHistory& record);
    const ModelGeometryState& geometryStateFor(const ModelingHistory& record) const;
    ModelRenderState& renderStateFor(const ModelingHistory& record);
    const ModelRenderState& renderStateFor(const ModelingHistory& record) const;
    void removeRuntimeStateFor(const ModelingHistory& record);
     void clearSketchPlaneHover();
     void updateSketchPlaneHover(int x, int y);
     void createOrUpdateSketchSelectedDatumPlane(const gp_Pln& pln, const TopoDS_Face& refFace);
     void ensureSketchPreviewLineActor();
     void updateSketchPreviewLine(const gp_Pnt& p1, const gp_Pnt& p2);
     void clearSketchPreviewLine();
     void ensureSketchPreviewRectangleActor();
     void updateSketchPreviewRectangle(const gp_Pnt& p1, const gp_Pnt& p3);
     void updateSketchPreviewRectangleGeneral(const gp_Pnt& a, const gp_Pnt& b, const gp_Pnt& c, const gp_Pnt& d);
     void clearSketchPreviewRectangle();
     void ensureSketchPreviewArcActor();
     void updateSketchPreviewArc(const gp_Pnt& p1, const gp_Pnt& pm, const gp_Pnt& p3);
     void clearSketchPreviewArc();
     void ensureSketchPreviewCircleActor();
     void updateSketchPreviewCircle(const gp_Pnt& center, double radius);
     void clearSketchPreviewCircle();
     void ensureSketchPreviewPolygonActor();
     void updateSketchPreviewPolygon(const QList<gp_Pnt>& verts);
     void clearSketchPreviewPolygon();
     void updateSketchPreviewPolygonGuideLine(const gp_Pnt& center, const gp_Pnt& guideEnd, bool dashed);
     void ensureSketchPreviewConicActor();
     void updateSketchPreviewConicBezier(const gp_Pnt& pole0, const gp_Pnt& pole1, const gp_Pnt& pole2);
     void clearSketchPreviewConic();
     void ensureSketchConicMarkerActors();
     void updateSketchConicMarkers();
     void clearSketchConicMarkers();
     gp_Pnt sketchConicRhoAdjustedShoulder(const gp_Pnt& p0, const gp_Pnt& pEnd, const gp_Pnt& shoulderUser,
                                          double rho) const;
     bool tryMakeSketchConicBezierEdge(const gp_Pnt& p0, const gp_Pnt& pEnd, const gp_Pnt& shoulderUser, double rho,
                                       TopoDS_Edge& outEdge) const;
     void armSnapFiltersForSketchToolFromMenuKind(int snapKind);
     void restoreSnapAfterSketchConicPick();
     void syncSketchConicDialogOkState();
     void clearSketchConicInternalState(bool clearDialogFields);
     void rebuildSketchConicPreview();
     void beginSketchConicPick(int whichField);
     void completeSketchConicPick(const gp_Pnt& p);
     void commitSketchConicFromDialog();
     void onSketchConicDialogRhoChanged(double v);
     void openOrRaiseSketchConicDialog();
     void closeSketchConicDialog();
     void armSnapForCuboidOriginKind(int snapKind);
     void beginSketchPolygonPick(int field);
     void completeSketchPolygonPick(const gp_Pnt& p);
     void openOrRaiseSketchPolygonDialog();
     void closeSketchPolygonDialog();
     void openOrRaiseSketchPolygonValueDialog();
     void closeSketchPolygonValueDialog();
     void positionSketchPolygonValueDialog(int screenX, int screenY);
     void rebuildSketchPolygonPreviewFromHover(const gp_Pnt& hoverWorld);
     void sketchApplyPolygonManualInput();
     bool commitSketchPolygonFromParams(const gp_Pnt& center, const gp_Pnt& sizeRef);
     bool sketchPolygonParamsFromHover(const gp_Pnt& center, const gp_Pnt& hover,
                                        double& outSize, double& outRotDeg) const;
     QList<gp_Pnt> sketchPolygonVertices(const gp_Pnt& center, int n,
                                         SketchPolygonDialog::SizeMode mode,
                                         double sizeVal, double rotDeg) const;
     void beginSketchEllipsePick(int field);
     void completeSketchEllipsePick(const gp_Pnt& p);
     void openOrRaiseSketchEllipseDialog();
     void closeSketchEllipseDialog();
     void openOrRaiseSketchEllipseAngleDialog();
     void closeSketchEllipseAngleDialog();
     void positionSketchEllipseAngleDialog(int screenX, int screenY);
     bool buildSketchEllipseEdge(const gp_Pnt& center, double majorR, double minorR, double rotDeg,
                                 TopoDS_Edge& outEdge) const;
     void updateSketchEllipseGeometry();
     void sketchApplyEllipseAngleFromDialog();
     void sketchEllipseRotationFromHover(const gp_Pnt& hoverWorld, double& outRotDeg) const;
     void handleSketchEllipseAdjustMouseMove(int x, int y);
     void handleSketchEllipseAdjustMouseDown(int x, int y);
     void handleSketchEllipseAdjustMouseUp(int x, int y);
     void handleSketchConicDragMouseDown(int x, int y);
     void handleSketchConicDragMouseMove(int x, int y);
     void handleSketchConicDragMouseUp(int x, int y);
     void rebuildSketchCommittedOverlay();
     void clearSketchCommittedOverlay();
     void appendCommittedSketchEdgeOverlay(const TopoDS_Edge& edge);

     SketchToolInputDialog* sketchToolInputDialog_ = nullptr;
     SketchRectangleModeDialog* sketchRectangleModeDialog_ = nullptr;
     SketchCircleModeDialog* sketchCircleModeDialog_ = nullptr;
     SketchConicDialog* sketchConicDialog_ = nullptr;
     SketchPolygonDialog* sketchPolygonDialog_ = nullptr;
     SketchPolygonValueDialog* sketchPolygonValueDialog_ = nullptr;
     int sketchPolygonPendingField_ = -1;
     bool sketchPolygonHasCenter_ = false;
     gp_Pnt sketchPolygonCenter_;
     SketchEllipseDialog* sketchEllipseDialog_ = nullptr;
     SketchEllipseAngleDialog* sketchEllipseAngleDialog_ = nullptr;
     int sketchEllipsePendingField_ = -1;
     bool sketchEllipseHasCenter_ = false;
     gp_Pnt sketchEllipseCenter_;
     int sketchEllipseGeomIndex_ = -1;
     bool sketchEllipseAngleDragActive_ = false;
     int sketchConicPendingField_ = -1;
     bool sketchConicHasP0_ = false;
     bool sketchConicHasP1_ = false;
     bool sketchConicHasPc_ = false;
     gp_Pnt sketchConicP0_, sketchConicP1_, sketchConicPc_;
     int sketchConicCommittedGeomIndex_ = -1;
     bool sketchConicDragActive_ = false;
     /** 当前选中的草图几何创建入口按钮；再次点击同一按钮退出创建模式 */
     QPushButton* sketchCreationExclusiveButton_ = nullptr;
     gp_Pnt sketchLastHoverPoint_;
     bool sketchLastHoverValid_ = false;
     bool sketchCommittedPointValid_ = false;
     gp_Pnt sketchCommittedPoint_;
     void openOrRaiseSketchToolInput(SketchToolInputDialog::ObjectKind initialObject);
     void closeSketchToolInput();
     void positionSketchToolInputDialog();
     void positionSketchAuxDialog(QDialog* dlg);
     void openOrRaiseSketchRectangleModeDialog();
     void closeSketchRectangleModeDialog();
     void openOrRaiseSketchCircleModeDialog();
     void closeSketchCircleModeDialog();
     void setupSketchCreationToggleButtons();
     bool sketchCreationExitIfRepeatClick(QPushButton* clickedButton);
     void prepareSketchCreationToolClick(QPushButton* clickedButton);
     void uncheckAllSketchCreationButtons();
     void exitSketchCreationMode();
     void updateSketchToolInputDialogFields(const gp_Pnt& hoverWorld);
     bool sketchWorldToUV(const gp_Pln& pln, const gp_Pnt& p, double& xc, double& yc) const;
     gp_Pnt sketchUVToWorld(const gp_Pln& pln, double xc, double yc) const;
     void sketchLineLengthAngle(const gp_Pln& pln, const gp_Pnt& a, const gp_Pnt& b, double& len, double& angDeg) const;
     gp_Pnt sketchLineEndFromLengthAngle(const gp_Pln& pln, const gp_Pnt& start, double len, double angDeg) const;
     bool sketchArcCenterFromTwoPointsRadius(const gp_Pln& pln, const gp_Pnt& p1, const gp_Pnt& p2, double R,
                                             const gp_Pnt& hint, gp_Pnt& outCenter) const;
     gp_Pnt sketchArcMidPointOnCircle(const gp_Pln& pln, const gp_Pnt& C, double R, const gp_Pnt& p1, const gp_Pnt& p2) const;
     double sketchArcRadiusFromThreePoints(const gp_Pnt& p1, const gp_Pnt& pm, const gp_Pnt& p2) const;
     bool sketchArcMidFromTangentAndEnd(const gp_Pln& pln, const gp_Pnt& S, const gp_Dir& tanAtS, const gp_Pnt& E,
                                        gp_Pnt& outMid) const;
     bool sketchArcFromStartRadiusSweep(const gp_Pln& pln, const gp_Pnt& S, const gp_Dir& refTan, double R,
                                         double sweepDeg, gp_Pnt& outEnd, gp_Pnt& outMid, gp_Pnt& outCenter) const;
     void sketchApplyManualInputFromDialog();
     void ensureDefaultSketchForTools();
     void openSketchPlaneDialogFromTool();
     bool isSketchEditMode(SelectionMode mode) const;
     bool tryPickSketchEdgeAt(int x, int y, TopoDS_Edge& outEdge, gp_Pnt* outWorldPoint = nullptr, double* outCurveParam = nullptr);
     void handleSketchEditHover(int x, int y);
     void handleSketchEditClick(int x, int y, bool brushMode);
     bool applyQuickTrimAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint);
     bool applyQuickExtendAt(const TopoDS_Edge& targetEdge, const gp_Pnt& clickPoint);
     bool replaceSketchEdgeWith(const TopoDS_Edge& targetEdge, const QList<TopoDS_Edge>& replacements, const QString& actionName);
     void clearSketchEditHover();
     void beginSketchBrushStroke();
     void endSketchBrushStroke();

     SelectionMode currentSelectionMode;
     int selectedTargetIndex;
     QList<int> selectedToolIndices;
     void handleBooleanSelection(vtkActor* selectedActor);
     void highlightBooleanOperands();
    // 临时存储对话框指针
    BoolOperationDialog* boolDialog;
    ExtrusionDialog* extrusionDialog;   // 临时存储拉伸对话框指针
    revolvedialog* revolveDialog;       // 临时存储旋转对话框指针
    filletdialog* filletDialog;         // 倒圆角对话框指针
    chamferdialog* chamferDialog;       // 倒角对话框指针
    class datum_plane* datumPlaneDialog_ = nullptr;
    class DatumAxisDialog* datumAxisDialog_ = nullptr;
    vtkSmartPointer<vtkActor> datumPlanePreviewActor_;
    vtkSmartPointer<vtkActor> datumAxisPreviewActor_;
    void clearDatumPlanePreview();
    void updateDatumPlanePreview();
    void commitDatumPlane();
    void clearDatumAxisPreview();
    void updateDatumAxisPreview();
    void commitDatumAxis();
    // 点选择相关
    CuboidParamsDialog* cuboidDialog;  // 临时存储长方体对话框指针

    // ===== 长方体交互式预览（Gizmo：原点 + 三轴）=====
    enum class CuboidGizmoPart { None, Origin, AxisLength, AxisWidth, AxisHeight };
    bool cuboidInteractiveActive_ = false;
    CuboidGizmoPart cuboidHoverPart_ = CuboidGizmoPart::None;
    CuboidGizmoPart cuboidDragPart_ = CuboidGizmoPart::None;
    bool cuboidDragActive_ = false;
    gp_Pnt cuboidInteractiveOrigin_;
    double cuboidInteractiveLength_ = 2.0;
    double cuboidInteractiveWidth_ = 2.0;
    double cuboidInteractiveHeight_ = 2.0;
    double cuboidDragStartParam_ = 0.0;   // 轴向拖拽：按下时沿轴的屏幕参数
    double cuboidDragStartDim_ = 0.0;     // 轴向拖拽：按下时的棱长
    double cuboidDragAxisScrOx_ = 0.0;
    double cuboidDragAxisScrOy_ = 0.0;
    double cuboidDragAxisScrDx_ = 0.0;
    double cuboidDragAxisScrDy_ = 0.0;
    double cuboidScreenCoordAlongAxis(int x, int y, double ox, double oy, double ax, double ay) const;
    void cuboidBeginAxisDragFrame(int x, int y, const gp_Pnt& origin, const gp_Dir& axisDir, double currentDim);
    vtkSmartPointer<vtkActor> cuboidPreviewBoxActor_;
    vtkSmartPointer<vtkActor> cuboidGizmoOriginActor_;
    vtkSmartPointer<vtkActor> cuboidGizmoAxisLineActors_[3];
    vtkSmartPointer<vtkActor> cuboidGizmoAxisArrowActors_[3];
    vtkSmartPointer<vtkFollower> cuboidGizmoAxisLabelActors_[3];
    QWidget* cuboidDimOverlayWidgets_[3] = {nullptr, nullptr, nullptr};
    class QLineEdit* cuboidDimEdits_[3] = {nullptr, nullptr, nullptr};
    int cuboidActiveDimAxis_ = -1;  // 当前显示的轴端输入框：0长 1宽 2高，-1无
    void startCuboidInteractiveMode();
    void stopCuboidInteractiveMode();
    void updateCuboidInteractivePreview();
    void clearCuboidInteractiveGizmo();
    void handleCuboidInteractiveMouseMove(int x, int y);
    void handleCuboidInteractiveMouseDown(int x, int y);
    void handleCuboidInteractiveMouseUp(int x, int y);
    void applyCuboidInteractiveOrigin(const gp_Pnt& p);
    void syncCuboidDialogFromInteractive();
    void syncCuboidInteractiveFromDialog();
    gp_Ax2 cuboidInteractiveAxisSystem() const;
    gp_Dir cuboidInteractiveLengthDir() const;
    gp_Dir cuboidInteractiveWidthDir() const;
    gp_Dir cuboidInteractiveHeightDir() const;
    bool cuboidPickGizmoPart(int x, int y, CuboidGizmoPart& outPart) const;
    gp_Pnt cuboidProjectMouseOnViewPlane(int x, int y, const gp_Pnt& planePoint) const;
    Qt::CursorShape cuboidCursorForAxis(const gp_Dir& axisDir) const;
    void updateCuboidDimOverlays();
    void hideCuboidDimOverlays();
    void ensureCuboidDimOverlay(int axisIndex);
    void setCuboidAxisHighlight(int axisIndex, bool active);
    static int cuboidAxisIndexFromGizmoPart(CuboidGizmoPart part);

    // ===== 阵列特征 =====
    PatternFeatureDialog* patternDialog_ = nullptr;
    QList<int> patternSelectedIndices_;
    enum class PatternVectorPick { None, Direction1, Direction2 };
    PatternVectorPick patternVectorPick_ = PatternVectorPick::None;
    bool patternPitchInteractiveActive_ = false;
    int patternActivePitchAxis_ = 0;
    bool patternPitchDragActive_ = false;
    double patternDragStartPitch_ = 0.0;
    double patternDragStartParam_ = 0.0;
    double patternDragStartDim_ = 0.0;
    double patternDragAxisScrOx_ = 0.0;
    double patternDragAxisScrOy_ = 0.0;
    double patternDragAxisScrDx_ = 0.0;
    double patternDragAxisScrDy_ = 0.0;
    gp_Pnt patternArrayOrigin_;
    vtkSmartPointer<vtkActor> patternPreviewActor_;
    vtkSmartPointer<vtkActor> patternGizmoLineActor_;
    vtkSmartPointer<vtkActor> patternGizmoArrowActor_;
    QWidget* patternPitchOverlay_ = nullptr;
    class QLineEdit* patternPitchEdit_ = nullptr;
    void handlePatternBodySelection(vtkActor* selectedActor);
    void updatePatternSelectionHighlight();
    void clearPatternSelectionHighlight();
    void applyPatternVectorPick(const gp_Dir& dir);
    void updatePatternPreview();
    void clearPatternPreview();
    void clearPatternPitchGizmoOnly();
    void updatePatternPitchGizmo();
    void stopPatternInteractiveCleanup();
    void startPatternPitchInteractive(int axisIndex);
    void stopPatternPitchInteractive();
    void handlePatternPitchMouseMove(int x, int y);
    void handlePatternPitchMouseDown(int x, int y);
    void handlePatternPitchMouseUp(int x, int y);
    bool patternPickPitchArrow(int x, int y) const;
    void updatePatternPitchOverlay();
    void hidePatternPitchOverlay();
    gp_Pnt computePatternArrayOrigin() const;
    gp_Dir patternDirection(int axisIndex) const;
    gp_Dir patternGizmoArrowDirection(const gp_Pnt& origin, const gp_Pnt& tip, int axisIndex) const;
    void computePatternPitchDragScreenAxis(double& ox, double& oy, double& dx, double& dy,
                                           int axisIndex) const;
    void updatePatternRotationAxisArrow();
    void startPatternPointSelection(int snapKind);
    void restorePatternPitchInteractiveAfterOriginPick();
    double patternPitchInteractiveValue(int axisIndex) const;
    void applyPatternPitchInteractiveValue(double value, int axisIndex);

    CylinderDialog* cylinderDialog;  // 临时存储圆柱体对话框指针
    ConeParamsDialog* coneDialog;  // 临时存储圆锥体对话框指针
    SphereParamsDialog* sphereDialog;  // 临时存储球体对话框指针
    void handlePointSelection(vtkActor* selectedActor, int x, int y);  // 处理点选择（支持面上任意点）
    void handleVtkMouseMove(int x, int y);  // 处理鼠标移动（用于悬停提示）
     void updatePointSelectionHover(int x, int y);  // 更新点选择的悬停提示
    void clearPointSelectionHover();  // 清除悬停提示
     void showSelectedPoint(const gp_Pnt& point, const QString& label);  // 显示选中的点
     void clearSelectedPoint();  // 清除选中的点显示

    // ===== 矢量对话框：任意方向 gp_Dir（方式A动态定义区域）=====
    QPointer<class vectordialog> vectorDialog_; // 便于在拾取/数值变化时回写UI
    bool hasCustomVectorDir_ = false;
    gp_Dir customVectorDir_ = gp_Dir(0, 0, 1); // 最终用于拉伸/旋转的方向
    bool hasVectorDialogBaseDir_ = false;
    gp_Dir vectorDialogBaseDir_ = gp_Dir(0, 0, 1); // 未应用反转按钮前
    bool vectorDialogReverse_ = false;
    int vectorDialogModeIndex_ = 0; // vectordialog comboBox 当前下标

    // 曲线上矢量（modeIndex==4）：记录选中的曲线边与位置参数
    bool hasVectorDialogCurveEdge_ = false;
    TopoDS_Edge vectorDialogCurveEdge_;
    double vectorDialogCurveTotalLen_ = 0.0;
    // positionMode: 0=弧长百分比 1=弧长
    int vectorDialogCurvePosMode_ = 0;
    double vectorDialogCurvePosValue_ = 0.0;

    bool hasVectorStartPoint_ = false;
    bool hasVectorEndPoint_ = false;
    gp_Pnt vectorStartPoint_;
    gp_Pnt vectorEndPoint_;

    enum class VectorTwoPointHandlePart { None, StartSphere, EndSphere, DirectionArrow };
    VectorTwoPointHandlePart vectorTwoPointHandleHover_ = VectorTwoPointHandlePart::None;
    VectorTwoPointHandlePart vectorTwoPointHandleSelected_ = VectorTwoPointHandlePart::None;
    VectorTwoPointHandlePart vectorTwoPointHandleDrag_ = VectorTwoPointHandlePart::None;
    bool vectorTwoPointHandleDragging_ = false;
    bool vectorTwoPointHandlesVisible_ = false;
    bool vectorTwoPointAwaitingEndPick_ = false;
    SelectionMode vectorTwoPointHandleSavedMode_ = None;
    int vectorTwoPointHandleSelectDownX_ = -1;
    int vectorTwoPointHandleSelectDownY_ = -1;
    vtkSmartPointer<vtkActor> vectorTwoPointStartSphereActor_;
    vtkSmartPointer<vtkActor> vectorTwoPointEndSphereActor_;
    vtkSmartPointer<vtkActor> vectorTwoPointLineActor_;
    QList<vtkSmartPointer<vtkActor>> vectorTwoPointSnapGhostActors_;
    gp_Pnt snapHoverBestPoint_;
    bool hasSnapHoverBestPoint_ = false;

    // 用于“高亮箭头预览”
    vtkSmartPointer<vtkActor> vectorDialogArrowActor_;
    vtkSmartPointer<vtkTransform> vectorDialogArrowTransform_;
    vtkSmartPointer<vtkActor> vectorDialogHoverShapeActor_;
    vtkSmartPointer<vtkActor> vectorDialogHoverOutlineActor_; // 面高亮的边界轮廓（避免被遮挡时看不见）
    bool hasVectorDialogArrowOrigin_ = false;
    gp_Pnt vectorDialogArrowOrigin_;
    int vectorDialogHoverModelIndex_ = -1;
    IVtk_IdType vectorDialogHoverSubShapeId_ = static_cast<IVtk_IdType>(-1);

    // 面高亮“变暗遮挡源模型”的临时状态，用于保证透明高亮能在更多视角下清晰可
    int vectorDialogHoverDimModelIndex_ = -1;
    QColor vectorDialogHoverDimOriginalColor_;
    double vectorDialogHoverDimOriginalOpacity_ = 1.0;

    void openVectorDialog(int desiredModeIndex = -1);
    void ensureVectorDialogArrowActor();
    void updateVectorDialogArrow(const gp_Dir& dir, const gp_Pnt& origin);
    void setCustomVectorDirFromDialog(const gp_Dir& baseDir);
    bool tryComputeVectorDirUnderCursor(int x, int y, gp_Dir& outDir,
                                        int* outHoverModelIndex = nullptr,
                                        IVtk_IdType* outHoverSubShapeId = nullptr);
    bool tryPickPointOnModelForVector(int x, int y, gp_Pnt& outPoint);
    void clearVectorDialogArrowPreview();
    // 同步原始对话框“反向”按钮：让箭头预览和建模方向一致
    void updateVectorArrowPreviewWithAxisReversed(bool axisReversed);
    void clearVectorDialogHoverShape();
    void applyVectorDialogHoverSubShape(int modelIndex, IVtk_IdType subShapeId, bool isFace);

    // 两点模式：起点/终点分别使用的捕捉类型（由 vectordialog 的 toolbutton 选择）
    // snapKind 约定：1=端点, 2=中点, 5=象限点, 6=圆弧中点, 3=交点
    int vectorTwoPointStartSnapKind_ = 1;
    int vectorTwoPointEndSnapKind_ = 1;
    void applyTwoPointVectorSnapKind(int snapKind, bool clearExistingPoints);
    void reapplyTwoPointSnapKindFilters(int snapKind);
    bool isVectorTwoPointDialogActive() const;
    void beginVectorTwoPointPickStart(bool refreshSnapKindsFromDialog = true);
    void onVectorTwoPointStartPicked(const gp_Pnt& point);
    void onVectorTwoPointEndPicked(const gp_Pnt& point);
    void updateVectorTwoPointHandles(const gp_Pnt* previewEnd = nullptr, const gp_Pnt* previewStart = nullptr);
    void clearVectorTwoPointHandles();
    bool tryPickVectorTwoPointSnapAt(int x, int y);
    void handleVectorTwoPointHandleMouseDown(int x, int y);
    void handleVectorTwoPointHandleMouseMove(int x, int y);
    void handleVectorTwoPointHandleMouseUp(int x, int y);
    void handleVectorTwoPointArrowDoubleClick(int x, int y);
    void reverseVectorTwoPointDirection();
    gp_Pnt resolveVectorTwoPointDragPosition(int x, int y, int snapKind) const;
    gp_Pnt resolveVectorTwoPointPreviewPosition(int x, int y, int snapKind);
    void clearVectorTwoPointSnapGhosts();
    void updateVectorTwoPointSnapPresentation(int x, int y, int snapKind, bool dragMode);
    void disableSnapUiAfterVectorTwoPointComplete();
    void applyVectorTwoPointFromEndpoints();
    VectorTwoPointHandlePart pickVectorTwoPointHandlePart(int x, int y);

     // 捕捉点（Snap Point）相关
     struct SnapSettings {
         bool enabled = false;      // UI：是否启用捕捉点
         bool armed = false;        // 运行态：用户确认后开始捕捉
         bool nearest = false;      // 捕捉最近点（快速选取）
         bool endpoint = false;     // 端点
         bool midpoint = false;     // 中点
         bool arcMidpoint = false;  // 圆弧中点（仅对圆/圆曲线）
         bool intersection = false; // 交点
         bool center = false;       // 圆心
         bool quadrant = false;     // 象限点
         bool onCurve = false;      // 点在曲线上（边任意位置）
         bool onFace = false;       // 面上的点（面任意位置）
     };
     SnapSettings snap_;
     gp_Pnt snapSelectedPoint_;
     bool hasSnapSelectedPoint_ = false;
     // 捕捉点结果常驻列表（可多点）
     QList<gp_Pnt> snapPersistentPoints_;
     QList<vtkSmartPointer<vtkActor>> snapPersistentPointActors_;
     // 捕捉点高亮显示（与点选择分离，避免互相覆盖）
     vtkSmartPointer<vtkActor> snapHoverPointActor_;
     vtkSmartPointer<vtkFollower> snapHoverTextActor_;
     vtkSmartPointer<vtkActor> snapHoverShapeActor_;
     vtkSmartPointer<vtkActor> snapSelectedPointActor_;
     vtkSmartPointer<vtkActor> snapSelectedShapeActor_;

     void setupSnapPointUI();
     /** 合并「视图页 Capture_*」与「建模/草图 Tab 点选择器」到 snap_，并更新 armed/enabled */
     void mergeSnapFiltersFromToolbarAndCaptureUi();
     /** 建模/草图 Tab 点选择器：可勾选、两页互相同步、与捕捉逻辑绑定 */
     void setupTabPointSnapToolbars();
     void clearTabPointSnapToolbarButtons();
     void syncTabPointSnapToolbarsFromCaptureRow();
     /** 仅当视图页「启用捕捉点」勾选时启用 Capture_* 类型按钮 */
     void updateSnapTypeFilterButtonsEnabled();
     /** 仅当建模/草图 Tab「启用捕捉点」勾选时启用 Tab 内点类型按钮 */
     void updateTabSnapTypeFilterButtonsEnabled();
     void setSnapArmed(bool armed);
     /** 捕捉拾取时：模型改幽灵外观（类似拉伸），并禁止整模黄亮 */
     void updateSnapPickGhostPresentation();
     bool snapPickGhostOwned_ = false;
     struct SnapHoverOptions {
         bool suppressHoverBall;
         bool suppressHoverText;
         bool showCandidateGhosts;
         /** >0 时在屏幕像素半径内扫描捕捉候选（用于两点矢量拖拽/拾取） */
         double expandScreenPixelRadius;

         SnapHoverOptions()
             : suppressHoverBall(false)
             , suppressHoverText(false)
             , showCandidateGhosts(false)
             , expandScreenPixelRadius(0.0)
         {
         }
     };
     void clearSnapSettings();
     void updateSnapHover(int x, int y, SnapHoverOptions options = SnapHoverOptions());
     void clearSnapHover();
     void pickSnapAt(int x, int y, SnapPickContext ctx = SnapPickContext::Normal);
     void appendActiveSketchSnapScreenCandidates(int x, int y, double maxScreenDist2,
                                                 QList<SketchSnapScreenCandidate>& out) const;
     void showSnapPointBall(const gp_Pnt& p);
     void clearSnapSelected();
     void addSnapPersistentPoint(const gp_Pnt& p);
     void clearSnapPersistentPoints();
     vtkSmartPointer<vtkActor> buildSnapShapeHighlightActor(const TopoDS_Shape& shape,
                                                           const double r, const double g, const double b,
                                                           const double opacity,
                                                           const double lineWidth);
    // 临时存储原点坐标（用于创建长方体）
     double tempOriginX, tempOriginY, tempOriginZ;
     bool hasTempOrigin;
    // 点选择相关的临时显示对象
     vtkSmartPointer<vtkActor> hoverPointActor;  // 悬停时显示的点
    vtkSmartPointer<vtkActor> selectedPointActor;  // 选中的点（固定显示）
     vtkSmartPointer<vtkFollower> hoverTextActor;  // 悬停时的文字标签
     vtkSmartPointer<vtkFollower> selectedTextActor;  // 选中点的文字标签
     gp_Pnt selectedOriginPoint;  // 选中的原点坐标
    bool hasSelectedOriginPoint;  // 是否已选中原点

    // 指定点（原点）捕捉：用工具栏选择 snap 类型后，在模型中捕捉该类点作为原点
    enum class OriginDialogKind {
        None = 0,
        Cuboid,
        Cylinder,
        Cone,
        Sphere,
        Revolve,
        Pattern
    };
    OriginDialogKind pendingOriginDialogKind_ = OriginDialogKind::None;
    bool originSnapSelectionActive_ = false;
    int pendingOriginSnapKind_ = -1; // 0最近点/1端点/2中点/3交点/4圆心/5象限
    void startOriginSnapSelection(OriginDialogKind kind, int snapKind);
    void applyOriginFromSnap(const gp_Pnt& p, const QString& chosenLabel);

     // 新增：获取选中形状的函数
    TopoDS_Shape getSelectedOccShape();

     // 测试函数
     void testOpenCASCADE();

     // 选择相关的函数
    void setupInteractor();
     // 隐藏原始的目标体和工具体
     void hideOriginalBodies(int targetIndex,
                             const QList<int>& toolIndices,
                             bool keepTarget,
                             bool keepTool);
     // 更新历史列表的可见性显示
    void updateHistoryListVisibility();

     // 拉伸选择相关函数
     void handleExtrusionSelection(vtkActor* selectedActor);
     void updateExtrusionSelectionHighlight();
     void clearExtrusionSelectionHighlight();
     // 拉伸面选择相关函数（新版本）
     void handleExtrusionFaceHover(int x, int y);  // 处理鼠标悬停在面上
     void handleExtrusionFaceClick(int x, int y); // 处理点击选择面
    void updateExtrusionFaceHighlight();          // 更新选中面的高亮显示
     void updateExtrusionHoverHighlight();         // 更新悬停面的高亮显示
     void clearExtrusionFaceHighlight();           // 清除面的高亮

     // 倒圆角：边选择与高亮
    void handleFilletEdgeClick(int x, int y);
     void handleFilletEdgeHover(int x, int y);
     void clearFilletEdgeHighlight();
     void updateFilletEdgeHighlight();
     int filletTargetModelIndex = -1;
     QList<IVtk_IdType> filletSelectedEdgeSubIds_;
     QList<TopoDS_Edge> filletSelectedEdges_;
     QList<vtkSmartPointer<vtkActor>> filletEdgeHighlightActors_;
     TopoDS_Edge filletHoverEdge_;
     bool hasFilletHoverEdge_ = false;
     vtkSmartPointer<vtkActor> filletHoverEdgeActor_;
     IVtk_IdType filletHoverEdgeSubId_ = -1;
     int filletHoverModelIndex_ = -1;

    // 倒圆角：两侧半径手柄拖拽（拖动一侧，另一侧同步）
    void updateFilletRadiusHandles();
    void clearFilletRadiusHandles();
    void handleFilletRadiusHandleMouseDown(int x, int y);
    void handleFilletRadiusHandleMouseMove(int x, int y);
    void handleFilletRadiusHandleMouseUp(int x, int y);

    vtkSmartPointer<vtkActor> filletRadiusHandleSphereActor_ = nullptr;
    vtkSmartPointer<vtkActor> filletRadiusHandleSide1Actor_ = nullptr;
    vtkSmartPointer<vtkActor> filletRadiusHandleSide2Actor_ = nullptr;
    vtkSmartPointer<vtkActor> filletRadiusHandleLine1Actor_ = nullptr;
    vtkSmartPointer<vtkActor> filletRadiusHandleLine2Actor_ = nullptr;
    bool filletRadiusHandleDragging_ = false;
    int filletRadiusActiveSide_ = -1; // 0/1 -> 两侧箭头
    int filletRadiusHoverSide_ = -1;
    gp_Pnt filletRadiusEdgeMid_ = gp_Pnt(0, 0, 0);
    gp_Dir filletRadiusDir1_ = gp_Dir(0, 0, 1);
    gp_Dir filletRadiusDir2_ = gp_Dir(0, 0, -1);

     // 倒角（对称）：边选择与高亮
    void handleChamferEdgeClick(int x, int y);
     void handleChamferEdgeHover(int x, int y);
     void clearChamferEdgeHighlight();
     void updateChamferEdgeHighlight();
     int chamferTargetModelIndex_ = -1;
     QList<IVtk_IdType> chamferSelectedEdgeSubIds_;
     QList<TopoDS_Edge> chamferSelectedEdges_;
     QList<vtkSmartPointer<vtkActor>> chamferEdgeHighlightActors_;
     TopoDS_Edge chamferHoverEdge_;
     bool hasChamferHoverEdge_ = false;
     vtkSmartPointer<vtkActor> chamferHoverEdgeActor_;
     IVtk_IdType chamferHoverEdgeSubId_ = -1;
     int chamferHoverModelIndex_ = -1;

    // 倒角（非对称）：两侧距离手柄拖拽
    void updateChamferAsymHandles();
    void clearChamferAsymHandles();
    void handleChamferAsymHandleMouseDown(int x, int y);
    void handleChamferAsymHandleMouseMove(int x, int y);
    void handleChamferAsymHandleMouseUp(int x, int y);

    vtkSmartPointer<vtkActor> chamferAsymHandleSphereActor_ = nullptr;
    vtkSmartPointer<vtkActor> chamferAsymHandleSide1Actor_ = nullptr;
    vtkSmartPointer<vtkActor> chamferAsymHandleSide2Actor_ = nullptr;
    vtkSmartPointer<vtkActor> chamferAsymHandleLine1Actor_ = nullptr;
    vtkSmartPointer<vtkActor> chamferAsymHandleLine2Actor_ = nullptr;
    bool chamferAsymHandleDragging_ = false;
    int chamferAsymActiveSide_ = -1; // 0->distance1, 1->distance2
    int chamferAsymHoverSide_ = -1;
    int chamferAsymDragStartX_ = -1;
    int chamferAsymDragStartY_ = -1;
    double chamferAsymStartD1_ = 0.0;
    double chamferAsymStartD2_ = 0.0;
    gp_Pnt chamferAsymEdgeMid_ = gp_Pnt(0, 0, 0);
    gp_Dir chamferAsymDir1_ = gp_Dir(0, 0, 1);
    gp_Dir chamferAsymDir2_ = gp_Dir(0, 0, -1);
     // 拉伸操作函数
     void performExtrusion(ExtrusionDialog* dialog);
     void performExtrusionWithPreview(ExtrusionDialog* dialog);
    // VIS子形状拾取相关
    QList<IVtk_IdType> selectedSubShapeIds;  // 选中的子形状ID列表
    vtkSmartPointer<vtkActor> faceHighlightActor;
    vtkSmartPointer<vtkActor> edgeHighlightActor;
    // 当前拾取的模型索引
    int currentPickedModelIndex;
    // VIS子形状选择处理函数
    void highlightSubShapes(int modelIndex, const IVtk_ShapeIdList& subShapeIds);
    void clearSubShapeHighlight();
     // 添加这些私有辅助函数
     int findModelIndexByParamName(const QString& paramName);
     void regenerateModel(int index, bool triggerCascade = true);
     void updateModelShape(int index, const TopoDS_Shape& newShape, bool triggerCascade = true);  // 更新模型形状
     void updateModelShapeWithTypeInternal(int index, const TopoDS_Shape& newShape, ModelType newType, bool triggerCascade = true);
     //修改模型历史列表括号后面的内容
    void updateModelName(int index);
     void regenerateBooleanResult(int booleanResultIndex);
     // Undo/Redo 系统
     CommandManager commandManager_;

     // Undo/Redo 管理方法
     void executeCommand(Command* command);
     // 更新按钮状态的方法
     void updateUndoRedoButtons();

    // 视图/摄像机：定向视图同步
    void setupViewUiConnections();
    void syncViewSelectionInTree(const QString& viewName);
    /** 草图页两个 QStackedWidget 的翻页按钮（Designer 预览箭头不会进入可执行程序） */
    void wireSketchTabStackedPages();

    QStackedWidget* toolbarModeStack_ = nullptr;
    class SARibbonBar* appRibbonBar_ = nullptr;
    class SARibbonCategory* ribbonCatView_ = nullptr;
    class SARibbonCategory* ribbonCatModel_ = nullptr;
    class SARibbonContextCategory* sketchRibbonContext_ = nullptr;
    class SARibbonCategory* sketchRibbonCategory_ = nullptr;
    /** 进入/退出草图时切换 Ribbon 页（隐藏旧 dock） */
    void applySketchRibbonMode(bool on);
    bool inSketchEnvironment_ = false;
    int savedNormalTabIndex_ = 0;

 public:
     void updateDependentFeatures(int modelIndex);
     void updateDependentBooleanResults(int modelIndex);
     bool regenerateFeature(int index);
     void assignFeatureRecipe(int index, const FeatureRecipe& recipe);
     bool applyShapeToHistory(int index, const TopoDS_Shape& newShape, const QString& newName = QString());
     TopoDS_Shape rebuildBaseShapeFromExtrusionRecipe(const ExtrusionRecipeData& recipe);
     bool applyDialogBooleanToShape(int boolMode, int boolTargetIndex,
                                    const TopoDS_Shape& featureShape,
                                    TopoDS_Shape& outShape) const;

     // 创建带参数的几何
    void createCuboidWithParams(double length, double width, double height,
                                bool hasOrigin = false, double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                                bool axisReversed = false);  // 长方体创建函数
    void createConeWithParams(double radius1, double radius2, double height,
                               bool hasOrigin = false, double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                               bool axisReversed = false);  // 圆锥体（圆台）参数化创建函数
    void createSphereWithParams(double radius, int thetaResolution, int phiResolution,
                                bool hasOrigin = false, double originX = 0.0, double originY = 0.0, double originZ = 0.0);  // 球体参数化创建函数
    /** 基本体「显示结果」：正式创建入栈/历史树，并删除交互手柄；取消时撤销 */
    void enterPrimitiveResultShown();
    void leavePrimitiveResultShown();
    void createCylinderWithParams(double radius, double height,
                                   bool hasOrigin = false, double originX = 0.0, double originY = 0.0, double originZ = 0.0,
                                   bool axisReversed = false);// 圆柱体参数化创建函数
     gp_Pnt computePatternArrayOriginForIndices(const QList<int>& indices) const;
     gp_Pnt resolvePatternOrigin(PatternFeatureDialog* dialog, const QList<int>& indices) const;
     void performPattern(PatternFeatureDialog* dialog);
     PatternLayoutType patternLayoutType() const;
     bool patternAxisUsesAngularPitch(int axisIndex) const;
     double patternGizmoSpanLength(const gp_Dir& axisDir) const;
     void computePatternGizmoSegment(gp_Pnt& origin, gp_Pnt& tip, int axisIndex) const;
     void performBooleanOperation(int targetIndex,
                                  const QList<int>& toolIndices,
                                  int operationType,
                                  bool keepTarget,
                                  bool keepTool);
     // 模型管理方法
     int getHistorySize() const { return modelDocument_.size(); }
     void removeModel(int index);
     void showModel(int index);
     // 命令系统专用方法（供命令类调用）
     int createGeometryDirectly(const QString& name, const QColor& color,
                                const PrimitiveGeometry::PrimitiveBuildRequest& request);
     void removeModelByIndex(int index);
     void setActiveSketchGeometriesForCommand(const QList<TopoDS_Shape>& geometries);
    // 命令系统访问：恢复工作坐标系（无需 OCC 形状）
    void restoreWorkCsys(int index, const QString& name, const QColor& color,
                         double x, double y, double z);
    void restoreReferenceCsys(int index, const QString& name, const QColor& color);
     // 添加恢复模型的方法
     void restoreModel(int index, const QString& name, ModelType type, const QColor& color,
                      double param1, double param2, double param3, const TopoDS_Shape& occShape,
                      const GeometryPlacement::AxisPlacement& placement =
                          GeometryPlacement::AxisPlacement());
     // 命令系统访问方法（供命令类调用）
     TopoDS_Shape getShapeFromHistory(int index);
     void displayOccShape(const TopoDS_Shape& shape, const QString& name,
                          ModelType type, const QColor& color,
                          double param1 = 0, double param2 = 0, double param3 = 0,
                          const GeometryPlacement::AxisPlacement& placement =
                              GeometryPlacement::AxisPlacement());

     // 命令系统访问：用于撤销/重做时修改可见性、更新既有模
     void setModelVisibleForCommand(int index, bool visible);
     bool isModelVisibleForCommand(int index) const;
     void setModelVisibility(int index, bool visible);
     bool featureDependsOnModel(int featureIndex, int modelIndex) const;
     QList<int> collectDependentFeatureIndices(int parentIndex) const;
     int primaryParentIndex(const ModelingHistory& record) const;
     void applyModelStateForCommand(int index, const TopoDS_Shape& shape, ModelType type, const QString& name, bool triggerCascade = true);
     void setModelParametersForCommand(int index, double param1, double param2, double param3);
     void regenerateModelForCommand(int index, bool triggerCascade = true);

     ModelHistorySnapshot captureModelSnapshot(int index) const;
     QList<ModelHistorySnapshot> captureModelSnapshots(const QList<int>& indices) const;
     QList<int> collectCascadeAffectedIndices(int rootIndex) const;
     CascadeUndoRecord beginCascadeUndoCapture(int rootIndex) const;
     void finishCascadeUndoCapture(CascadeUndoRecord& record) const;
     void restoreModelSnapshot(int index, const ModelHistorySnapshot& snapshot);
     void restoreCascadeUndoStates(const CascadeUndoRecord& record, bool useBeforeStates);
     void restoreModelFromSnapshot(int index, const ModelHistorySnapshot& snapshot);
     void remapHistoryIndicesAfterRemoval(int removedIndex);
     void remapHistoryIndicesAfterInsertion(int insertedIndex);
     void applyHistoryMetadataFromJson(int index, const QJsonObject& obj);


 protected:
     // 重写鼠标事件
    void closeEvent(QCloseEvent* event) override;
     void mousePressEvent(QMouseEvent* event) override;
     void resizeEvent(QResizeEvent* event) override;
     bool eventFilter(QObject *obj, QEvent *event) override;

 private:
     // VIS 拾取
    vtkSmartPointer<IVtkTools_ShapePicker> shapePicker;
     // 形状ID计数器
     static int shapeIDCounter;
     vtkSmartPointer<vtkActor> previewActor;  // 预览actor
     // 拉伸/旋转交互手柄
     enum class ExtrusionHandlePart { None, StartSphere, EndArrow };
     enum class RevolveHandlePart { None, StartSphere, EndArrow };
     ExtrusionHandlePart extrusionHandleHover_ = ExtrusionHandlePart::None;
    ExtrusionHandlePart extrusionHandleSelected_ = ExtrusionHandlePart::None; // 仅按下但尚未进入拖拽的“选择态”
     ExtrusionHandlePart extrusionHandleDrag_ = ExtrusionHandlePart::None;
     RevolveHandlePart revolveHandleHover_ = RevolveHandlePart::None;
    RevolveHandlePart revolveHandleSelected_ = RevolveHandlePart::None; // 仅按下但尚未进入拖拽的“选择态”
     RevolveHandlePart revolveHandleDrag_ = RevolveHandlePart::None;
     bool extrusionHandleDragging_ = false;
     bool revolveHandleDragging_ = false;
     bool featureOperationGhostMode_ = false;
     /** 点击「预览」后的结果锁定态：冻结实时半透明预览与手柄交互 */
     bool featureResultPreviewActive_ = false;
     QList<int> featureResultPreviewHiddenModelIndices_;
     SelectionMode extrusionHandleSavedMode_ = None;
     SelectionMode revolveHandleSavedMode_ = None;
     double extrusionHandleDragGrab_ = 0.0;
     double revolveHandleDragGrabDeg_ = 0.0;
    int extrusionHandleSelectDownX_ = -1;
    int extrusionHandleSelectDownY_ = -1;
    int revolveHandleSelectDownX_ = -1;
    int revolveHandleSelectDownY_ = -1;
     vtkSmartPointer<vtkActor> extrusionHandleLineActor_;
     vtkSmartPointer<vtkActor> extrusionHandleSphereActor_;
     vtkSmartPointer<vtkActor> extrusionHandleArrowActor_;
     vtkSmartPointer<vtkActor> revolveHandleArcActor_;
     vtkSmartPointer<vtkActor> revolveHandleStartLineActor_;
     vtkSmartPointer<vtkActor> revolveHandleEndLineActor_;
     vtkSmartPointer<vtkActor> revolveHandleSphereActor_;
     vtkSmartPointer<vtkActor> revolveHandleArrowActor_;
     vtkSmartPointer<vtkActor> revolveHandleCenterActor_;
     QList<int> extrusionSelectedIndices;     // 存储拉伸选中的模型索引（旧版本，保留兼容性）
     
     // 拉伸几何选择相关数据结构（支持面、线框、边）
     struct ExtrusionFaceSelection {
        int modelIndex;           // 模型索引
        IVtk_IdType subShapeId;   // 子形状ID（面、线框或边）
        TopoDS_Shape shape;        // 子形状（面、线框或边）
        TopAbs_ShapeEnum shapeType; // 形状类型（FACE、WIRE、EDGE）
        // 便捷访问
    TopoDS_Face getFace() const {
            if (shapeType == TopAbs_FACE && !shape.IsNull()) {
                return TopoDS::Face(shape);
            }
            return TopoDS_Face();
        }
        
        TopoDS_Wire getWire() const {
            if (shapeType == TopAbs_WIRE && !shape.IsNull()) {
                return TopoDS::Wire(shape);
            }
            return TopoDS_Wire();
        }
        
        TopoDS_Edge getEdge() const {
            if (shapeType == TopAbs_EDGE && !shape.IsNull()) {
                return TopoDS::Edge(shape);
            }
            return TopoDS_Edge();
        }
        
        bool operator==(const ExtrusionFaceSelection& other) const {
            return modelIndex == other.modelIndex && subShapeId == other.subShapeId;
        }
    };

      QList<ExtrusionFaceSelection> extrusionSelectedFaces;  // 存储拉伸选中的面
     /** 递增以使已排队的选中后延迟刷新失效（关闭对话框时防复活高亮） */
     int extrudeRevolveSelectionEpoch_ = 0;
     ExtrusionFaceSelection hoveredFace;                    // 当前悬停的面
     bool hasHoveredFace;                                    // 是否有悬停的
    QList<vtkSmartPointer<vtkActor>> extrusionFaceHighlightActors; // 选中面高亮actors列表
     vtkSmartPointer<vtkActor> extrusionHoverHighlightActor; // 悬停面高亮actor
     vtkSmartPointer<vtkActor> extrusionHoverOutlineActor;   // 悬停面轮廓线actor（仅边界边）
      vtkRenderer* featureSelectionOverlay(); // 兼容旧名 → appearanceOverlay()

     // 悬浮“变暗遮挡源”临时状态（保证悬浮半透明高亮在遮挡视角下也能看清
    int extrusionHoverDimModelIndex_ = -1;
     QColor extrusionHoverDimOriginalColor_;
     double extrusionHoverDimOriginalOpacity_ = 1.0;
     
     // 添加交互样式成员变量
     vtkSmartPointer<MouseInteractorStyle> m_interactorStyle;

     // 恢复历史快照时抑制级联更新，避免撤销/重做递归触发
     int cascadeUpdateGuard_ = 0;

};


#endif // MAIN_WINDOW_H
