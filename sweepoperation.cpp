#include "sweepoperation.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>

SweepOperation::SweepOperation(const TopoDS_Shape& profile, SweepPath* path)
    : profileShape(profile), sweepPath(path), makeSolid(true)
{
    if (sweepPath == nullptr) {
        errorMessage = "扫描路径为空";
    } else if (profileShape.IsNull()) {
        errorMessage = "扫描轮廓为空";
    } else if (!sweepPath->isValid()) {
        errorMessage = "扫描路径无效";
    } else {
        errorMessage = "";
    }
}

SweepOperation::~SweepOperation()
{
    // 注意：不删除sweepPath，因为它由调用者管理
}

TopoDS_Shape SweepOperation::execute()
{
    if (!isValid()) {
        return TopoDS_Shape();
    }

    // 根据路径类型选择不同的扫描方法
    QString pathType = sweepPath->getTypeName();

    if (pathType == "Linear") {
        const LinearSweepPath* linearPath = dynamic_cast<const LinearSweepPath*>(sweepPath);
        if (linearPath) {
            return performLinearSweep(profileShape, linearPath);
        }
    } else if (pathType == "Revolve") {
        const RevolveSweepPath* revolvePath = dynamic_cast<const RevolveSweepPath*>(sweepPath);
        if (revolvePath) {
            return performRevolveSweep(profileShape, revolvePath);
        }
    } else {
        // 通用扫描方法（用于未来扩展）
        return performGeneralSweep(profileShape, sweepPath);
    }

    errorMessage = "不支持的扫描路径类型: " + pathType;
    return TopoDS_Shape();
}

bool SweepOperation::isValid() const
{
    return sweepPath != nullptr && 
           !profileShape.IsNull() && 
           sweepPath->isValid() &&
           errorMessage.isEmpty();
}

QString SweepOperation::getErrorMessage() const
{
    return errorMessage;
}

void SweepOperation::setMakeSolid(bool solid)
{
    makeSolid = solid;
}

bool SweepOperation::getMakeSolid() const
{
    return makeSolid;
}

TopoDS_Shape SweepOperation::performLinearSweep(const TopoDS_Shape& profile, const LinearSweepPath* path)
{
    if (!path || !path->isValid()) {
        errorMessage = "线性扫描路径无效";
        return TopoDS_Shape();
    }

    // 获取拉伸向量
    gp_Vec extrudeVec = path->getTranslationAt(1.0); // t=1.0表示终点

    // 使用OpenCASCADE的BRepPrimAPI_MakePrism进行拉伸
    BRepPrimAPI_MakePrism prism(profile, extrudeVec);

    if (!prism.IsDone()) {
        errorMessage = "拉伸操作失败";
        return TopoDS_Shape();
    }

    return prism.Shape();
}

TopoDS_Shape SweepOperation::performRevolveSweep(const TopoDS_Shape& profile, const RevolveSweepPath* path)
{
    if (!path || !path->isValid()) {
        errorMessage = "旋转扫描路径无效";
        return TopoDS_Shape();
    }

    // TODO: 实现旋转扫描
    // 需要使用BRepPrimAPI_MakeRevol
    // 注意：RevolveSweepPath需要先实现完整逻辑
    
    errorMessage = "旋转扫描功能尚未完全实现";
    return TopoDS_Shape();
}

TopoDS_Shape SweepOperation::performGeneralSweep(const TopoDS_Shape& profile, const SweepPath* path)
{
    // 通用扫描方法（用于未来扩展）
    // 可以用于沿曲线扫描、螺旋扫描等复杂路径
    
    // TODO: 实现通用扫描算法
    // 可能需要使用BRepOffsetAPI_MakePipe或自定义算法
    
    errorMessage = "通用扫描功能尚未实现";
    return TopoDS_Shape();
}

SweepOperation* SweepOperation::fromSketch(const Sketch& sketch, SweepPath* path)
{
    if (!sketch.isValid()) {
        return nullptr;
    }

    TopoDS_Shape sketchShape = sketch.getShape();
    
    // 如果草图是Wire，尝试创建Face
    if (sketchShape.ShapeType() == TopAbs_WIRE) {
        TopoDS_Wire wire = TopoDS::Wire(sketchShape);
        BRepBuilderAPI_MakeFace faceMaker(wire);
        if (faceMaker.IsDone()) {
            sketchShape = faceMaker.Face();
        }
    }

    return new SweepOperation(sketchShape, path);
}

