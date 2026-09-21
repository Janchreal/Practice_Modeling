/*
namespace { ... }               ← 内部实现（匿名命名空间）
namespace ModelShapePipeline {  ← 对外公开 API
    configureSolidShapePipeline
    createShapeDataSource
    copyShapeDataSourceOutput
    createHighlightFilter
    createHighlightActor
    rewireHighlightActor
}*/



#include "model_shape_pipeline.h"

#include <IVtk_Types.hxx>//把TopoDS_Shape+显示属性封装成一个句柄对象
#include <IVtkTools_DisplayModeFilter.hxx>//根据显示模式（线框/着色/着色+边【DM_Shading / DM_Wireframe / DM_ShadingWithEdges】）过滤输入，输出对应的polydata（面/边/顶点）
#include <IVtkTools_ShapeDataSource.hxx>//OCC形状→VTK多边形数据polydata的源头算法，内部驱动BRepMesh进行三角化
#include <IVtkTools_SubPolyDataFilter.hxx>//按IdFilter（子形状 ID）抽取polydata的子集，用于高亮/选中单个面或边

#include <Prs3d_Drawer.hxx>//OCC的显示属性容器，保存偏差、偏差角、线宽、颜色、材质等
//Actor（场景中的可渲染对象）、Mapper（数据到图元的映射）、PolyData（几何数据）、Property（颜色/透明度/材质等）
#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
//匿名命名空间
namespace {
//常量，constexpr + 匿名命名空间 ⇒ 编译期常量+内部链接，不产生外部符号，不污染其它TU
constexpr double kDisplayDeflection = 0.003;//弦高偏差（linear deflection）。三角化时，曲面上一条弦与真实曲面之间的最大允许距离。值越小，三角面越密、越精细，但内存/耗时也越大。
constexpr double kDisplayDeviationAngle = 0.05;//角度偏差，当曲面法线变化超过该角度时需进一步细分，用于控制曲率大区域的三角密度。

void configureDisplayTessellation(const Handle(IVtkOCC_Shape)& shapeWrapper)
{//先判断包装器本身为空，再判断其属性Prs3d_Drawer为空
    if (shapeWrapper.IsNull() || shapeWrapper->Attributes().IsNull()) {//Attributes()返回的是OCC的显示属性容器<Prs3d_Drawer.hxx>
        return;
    }
    //Handle是一个智能指针模板，用于专门用来管理继承自Standard_Transient的对象生命周期
    const Handle(Prs3d_Drawer)& drawer = shapeWrapper->Attributes();
    //设置偏差类型
    drawer->SetTypeOfDeflection(Aspect_TOD_RELATIVE);//Aspect_TOD_RELATIVE相对偏差。Aspect_TOD_ABSOLUTE绝对偏差。真实偏差 = 系数 × 包围盒特征尺寸（通常是包围盒对角线长度）
    //把最开始的两个常量交给OCC，后续IVtkTools_ShapeDataSource::SetShape()会读取这些属性，触发BRepMesh时按此精度三角化。
    drawer->SetDeviationCoefficient(kDisplayDeflection);
    drawer->SetDeviationAngle(kDisplayDeviationAngle);
    //打开自动三角化，允许数据源在没有三角网格时自动调用BRepMesh。如果关掉，就需要外部显式网格化，否则polydata为空。
    drawer->SetAutoTriangulation(Standard_True);
}
//vtkSmartPointer<T>  Handle(T)，都是引用计数智能指针，自动管理对象生命周期
//职责是从源数据里按"子形状 ID"过滤出需要的部分（用于高亮某个面/边）
vtkSmartPointer<IVtkTools_SubPolyDataFilter> createHighlightFilterImpl(IVtkTools_ShapeDataSource* source)
{
    auto filter = vtkSmartPointer<IVtkTools_SubPolyDataFilter>::New();
    if (source) {
        filter->SetInputConnection(source->GetOutputPort());//SetInputConnection：标准的VTK管线连接方式，把数据源输出端口连到filter输入
    }
    filter->SetIdsArrayName("SUBSHAPE_IDS");//告诉filter使用polydata中名为SUBSHAPE_IDS的数组来识别每个图元对应的子形状ID。IVtkTools_ShapeDataSource默认会写入这个数组。
    return filter;//源为空时也返回一个可用的filter，保证调用者不会拿到nullptr而崩溃。
}
//创建高亮actor
vtkSmartPointer<vtkActor> createHighlightActorImpl(IVtkTools_SubPolyDataFilter* filter)
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (filter) {
        mapper->SetInputConnection(filter->GetOutputPort());
    }
    mapper->ScalarVisibilityOff();//不按标量（颜色数组）着色，直接用 property 里的颜色，避免 polydata 里的标量数组影响外观

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 0.0);//颜色
    actor->GetProperty()->SetOpacity(0.6);//透明度
    actor->GetProperty()->SetRepresentationToSurface();//用面绘制
    actor->GetProperty()->EdgeVisibilityOff();//不显示边线，避免与实体管线的边重复
    actor->GetProperty()->SetLighting(true);//开启光照，让高亮也有明暗立体感
    actor->SetPickable(false);//不可拾取，避免高亮层遮挡交互
    actor->SetVisibility(false);//默认隐藏，只有需要高亮时才打开
    return actor;
}
//把已有的高亮actor重新接到一个新的filter上
/*
* 为何需要这么做
* 场景里模型可能被重建（换了一个新 IVtkTools_ShapeDataSource）
* 每次重建都重新创建actor/mapper，会造成频繁的GPU资源分配/释放，也可能打破外部对actor指针的引用（例如已经加入渲染器的actor列表）
* 因此这里采用"保留 actor、只换 mapper 输入"的策略
*/
//这段代码与 createHighlightActorImpl 的属性设置几乎重复，可以抽出一个applyHighlightStyle(vtkActor*)函数复用。
void rewireHighlightActorImpl(vtkActor* actor, IVtkTools_SubPolyDataFilter* filter)
{
    if (!actor) {
        return;
    }

    const int oldVisibility = actor->GetVisibility();//记住可见性
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (filter) {
        mapper->SetInputConnection(filter->GetOutputPort());
    }
    mapper->ScalarVisibilityOff();

    actor->SetMapper(mapper);//换新的mapper
    actor->GetProperty()->SetColor(1.0, 1.0, 0.0);
    actor->GetProperty()->SetOpacity(0.6);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOff();
    actor->GetProperty()->SetLighting(true);
    actor->SetPickable(false);
    actor->SetVisibility(oldVisibility);//修复可见性
}

} // namespace
//对外API
namespace ModelShapePipeline {
//实体渲染管线的装配函数
vtkSmartPointer<IVtkTools_DisplayModeFilter> configureSolidShapePipeline(IVtkTools_ShapeDataSource* source,
                            vtkDataSetMapper* mapper,
                            vtkActor* actor)
{
    if (!source || !mapper || !actor) {
        return nullptr;
    }

    auto filter = vtkSmartPointer<IVtkTools_DisplayModeFilter>::New();
    filter->SetInputConnection(source->GetOutputPort());
    filter->SetDisplayMode(DM_Shading);//着色模式（填充面）
    filter->SetSmoothShading(true);//Gouraud 平滑着色，让曲面看起来是光滑的，而不是一块一块的三角片
    mapper->SetInputConnection(filter->GetOutputPort());//VTK 标准管线连接方式
    mapper->ScalarVisibilityOff();//不使用polydata中可能带的标量数组着色，避免颜色被覆盖
    actor->SetMapper(mapper);
    return filter;
}
//整个管线的起点
vtkSmartPointer<IVtkTools_ShapeDataSource> createShapeDataSource(const Handle(IVtkOCC_Shape)& shapeWrapper)
{
    if (shapeWrapper.IsNull()) {
        return nullptr;
    }
    //先配置三角化参数，如果不设置会以默认参数三角化
    configureDisplayTessellation(shapeWrapper);

    auto source = vtkSmartPointer<IVtkTools_ShapeDataSource>::New();
    source->SetShape(shapeWrapper);//把OCC形状塞给数据源
    source->Modified();//手动标记数据已修改，强制下一次Update()重新计算，即使VTK认为输入没有变化
    source->Update();//立即执行三角化，产生 polydata
    return source;
}
/*拷贝数据源的输出 polydata。 使用场景通常是：
①想冻结当前形状的几何（例如做撤销/重做快照、做后台处理、上传到 GPU 缓存）
②要把polydata传给另一个管线，但不想让它随源shape变化
*/
vtkSmartPointer<vtkPolyData> copyShapeDataSourceOutput(IVtkTools_ShapeDataSource* source, bool deepCopy)
{
    auto copy = vtkSmartPointer<vtkPolyData>::New();
    if (!source) {
        return copy;
    }

    source->Update();
    vtkPolyData* output = source->GetOutput();
    if (!output) {
        return copy;
    }

    if (deepCopy) {
        copy->DeepCopy(output);//深拷贝，复制所有点、线、面、属性数组的内存，完全独立的一份数据，源释放也无所谓
    } else {
        copy->ShallowCopy(output);//只复制数据结构，底层数组仍与源共享，快、省内存，但源被改后这份数据也会跟着变
    }
    return copy;
}
//三个薄封装
vtkSmartPointer<IVtkTools_SubPolyDataFilter>
createHighlightFilter(IVtkTools_ShapeDataSource* source)
{
    return createHighlightFilterImpl(source);
}

vtkSmartPointer<vtkActor>
createHighlightActor(IVtkTools_SubPolyDataFilter* filter)
{
    return createHighlightActorImpl(filter);
}

void rewireHighlightActor(vtkActor* actor, IVtkTools_SubPolyDataFilter* filter)
{
    rewireHighlightActorImpl(actor, filter);
}

} // namespace ModelShapePipeline
