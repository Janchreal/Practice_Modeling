/*
 * VIS ShapePicker 使用示例
 * 展示如何在您的项目中使用 IVtkTools_ShapePicker 进行拾取和高亮显示
 * 
 * 主要功能：
 * 1. 形状拾取
 * 2. 子形状（面、边、顶点）选择
 * 3. 高亮显示选中的子形状
 */

// VTK includes
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

// VIS includes
#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_ShapeObject.hxx>
#include <IVtkTools_ShapePicker.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>
#include <IVtkTools.hxx>

// OCCT includes
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TColStd_MapIteratorOfPackedMapOfInteger.hxx>
#include <TColStd_PackedMapOfInteger.hxx>

//-----------------------------------------------------------------------------
// 形状管线类 - 管理形状的显示和高亮
//-----------------------------------------------------------------------------

DEFINE_STANDARD_HANDLE(ShapePipeline, Standard_Transient)

class ShapePipeline : public Standard_Transient
{
public:
    DEFINE_STANDARD_RTTI_INLINE(ShapePipeline, Standard_Transient)

    // 构造函数
    ShapePipeline(const bool isHighlight = false) : m_bIsHighlight(isHighlight)
    {
        this->Build();
        
        if (m_bIsHighlight) {
            // 高亮显示的样式
            m_actor->GetProperty()->SetColor(1.0, 1.0, 0.0); // 黄色
            m_actor->GetProperty()->SetLineWidth(4.0);
            m_actor->GetProperty()->SetOpacity(0.8);
        }
    }

    // 构建管线
    void Build()
    {
        m_subDataFilter = vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
        m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_actor = vtkSmartPointer<vtkActor>::New();
        m_actor->SetMapper(m_mapper);
    }

    // 初始化子多边形过滤器
    void InitSubPolyFilter(const TColStd_PackedMapOfInteger& mask)
    {
        IVtk_IdTypeMap dataToKeep;
        for (TColStd_MapIteratorOfPackedMapOfInteger it(mask); it.More(); it.Next()) {
            dataToKeep.Add(it.Key());
        }
        
        m_subDataFilter->SetData(dataToKeep);
        m_subDataFilter->Modified();
    }

    // 初始化形状
    void Init(const TopoDS_Shape& shape, int shapeID)
    {
        // 创建 VIS 形状包装器
        Handle(IVtkOCC_Shape) shapeWrapper = new IVtkOCC_Shape(shape);
        shapeWrapper->SetId(shapeID);

        // 创建数据源
        vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDS = 
            vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
        shapeDS->SetShape(shapeWrapper);

        // 建立过滤器连接
        if (m_bIsHighlight) {
            m_subDataFilter->SetInputConnection(shapeDS->GetOutputPort());
            m_mapper->SetInputConnection(m_subDataFilter->GetOutputPort());
            m_mapper->ScalarVisibilityOff();
        } else {
            m_mapper->SetInputConnection(shapeDS->GetOutputPort());
            IVtkTools::InitShapeMapper(m_mapper);
        }

        // 将数据源绑定到 actor
        IVtkTools_ShapeObject::SetShapeSource(shapeDS, m_actor);
    }

    // 添加到渲染器
    void AddToRenderer(vtkRenderer* renderer) const
    {
        renderer->AddActor(m_actor);
    }

    // 更新
    void Update()
    {
        m_mapper->Update();
    }

    // 获取 Actor
    vtkActor* Actor() const { return m_actor; }
    
    // 获取 Mapper
    vtkPolyDataMapper* Mapper() const { return m_mapper; }

private:
    bool m_bIsHighlight;
    vtkSmartPointer<IVtkTools_SubPolyDataFilter> m_subDataFilter;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;
};

//-----------------------------------------------------------------------------
// 全局上下文
//-----------------------------------------------------------------------------

namespace Context
{
    TopoDS_Shape MainShape;
    Handle(ShapePipeline) MainShapePL;
    Handle(ShapePipeline) HighlightPL;
    vtkSmartPointer<vtkRenderWindow> RenderWindow;
    vtkSmartPointer<IVtkTools_ShapePicker> ShapePicker;
}

//-----------------------------------------------------------------------------
// 自定义交互器样式 - 处理拾取
//-----------------------------------------------------------------------------

class PickerInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static PickerInteractorStyle* New();
    vtkTypeMacro(PickerInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetRenderer(const vtkSmartPointer<vtkRenderer>& renderer) 
    { 
        m_renderer = renderer; 
    }
    
    void SetPicker(const vtkSmartPointer<IVtkTools_ShapePicker>& picker) 
    { 
        m_picker = picker; 
    }

    virtual void OnLeftButtonDown() override
    {
        // 设置选择模式（可以选择：SM_Vertex, SM_Edge, SM_Face, SM_Solid, SM_Shell）
        m_picker->SetSelectionMode(SM_Face);

        // 调用基类方法
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

        // 获取鼠标位置
        Standard_Integer pos[2] = { 
            this->Interactor->GetEventPosition()[0],
            this->Interactor->GetEventPosition()[1] 
        };

        // 执行拾取
        m_picker->Pick(pos[0], pos[1], 0);

        // 处理拾取结果
        vtkSmartPointer<vtkActorCollection> actorCollection = m_picker->GetPickedActors();
        
        if (actorCollection && actorCollection->GetNumberOfItems() > 0) {
            actorCollection->InitTraversal();
            
            while (vtkActor* actor = actorCollection->GetNextActor()) {
                // 获取形状数据源
                IVtkTools_ShapeDataSource* dataSource = 
                    IVtkTools_ShapeObject::GetShapeSource(actor);
                    
                if (!dataSource) continue;

                // 获取形状包装器
                Handle(IVtkOCC_Shape) shapeWrapper = dataSource->GetShape();
                if (shapeWrapper.IsNull()) continue;

                // 获取形状ID
                IVtk_IdType shapeID = shapeWrapper->GetId();
                
                // 获取拾取的子形状ID列表
                IVtk_ShapeIdList subShapeIds = m_picker->GetPickedSubShapesIds(shapeID);

                // 获取单元格ID用于高亮显示
                TColStd_PackedMapOfInteger cellMask;
                
                for (IVtk_ShapeIdList::Iterator sIt(subShapeIds); sIt.More(); sIt.Next()) {
                    IVtk_IdType subShapeId = sIt.Value();
                    cellMask.Add((int)subShapeId);
                    
                    const TopoDS_Shape& subShape = shapeWrapper->GetSubShape(subShapeId);
                    (void)subShape;
                }

                // 更新高亮显示
                Context::HighlightPL->InitSubPolyFilter(cellMask);
                Context::HighlightPL->Update();
                Context::RenderWindow->Render();
                
                break; // 只处理第一个拾取到的actor
            }
        }
    }

private:
    PickerInteractorStyle() : vtkInteractorStyleTrackballCamera() {}
    ~PickerInteractorStyle() {}

    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<IVtkTools_ShapePicker> m_picker;
};

vtkStandardNewMacro(PickerInteractorStyle);

//-----------------------------------------------------------------------------
// 主函数
//-----------------------------------------------------------------------------

int main(int, char**)
{
    // 创建渲染器
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->LightFollowCameraOn();
    renderer->SetBackground(0.2, 0.2, 0.2);

    // 创建渲染窗口
    Context::RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    Context::RenderWindow->AddRenderer(renderer);
    Context::RenderWindow->SetSize(800, 600);

    // 创建 VIS ShapePicker
    Context::ShapePicker = vtkSmartPointer<IVtkTools_ShapePicker>::New();
    Context::ShapePicker->SetTolerance(0.025);
    Context::ShapePicker->SetRenderer(renderer);

    // 创建测试形状（立方体）
    Context::MainShape = BRepPrimAPI_MakeBox(60, 80, 90).Shape();

    // 创建主形状管线
    Context::MainShapePL = new ShapePipeline(false);
    Context::MainShapePL->Init(Context::MainShape, 1);
    Context::MainShapePL->AddToRenderer(renderer);

    // 创建高亮管线
    Context::HighlightPL = new ShapePipeline(true);
    Context::HighlightPL->Init(Context::MainShape, 1);
    Context::HighlightPL->AddToRenderer(renderer);

    // 创建交互器样式
    vtkSmartPointer<PickerInteractorStyle> style = 
        vtkSmartPointer<PickerInteractorStyle>::New();
    style->SetRenderer(renderer);
    style->SetPicker(Context::ShapePicker);

    // 创建交互器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = 
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(Context::RenderWindow);
    interactor->SetInteractorStyle(style);

    // 开始渲染
    Context::RenderWindow->Render();
    interactor->Start();

    return EXIT_SUCCESS;
}






