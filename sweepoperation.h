#ifndef SWEEPOPERATION_H
#define SWEEPOPERATION_H

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include "sweeppath.h"
#include "sketch.h"
#include <QString>

/**
 * @brief 通用扫描操作类
 * 
 * 这是扫描操作的核心实现，使用OpenCASCADE的几何内核。
 * 设计遵循开闭原则：
 * - 核心算法固定，通过SweepPath接口支持任意轨迹
 * - 添加新的扫描类型只需实现新的SweepPath子类
 * 
 * 支持的扫描类型：
 * - 拉伸（LinearSweepPath）
 * - 旋转（RevolveSweepPath）- 预留
 * - 沿曲线扫描（CurveSweepPath）- 未来扩展
 * - 螺旋扫描（HelicalSweepPath）- 未来扩展
 */
class SweepOperation {
public:
    /**
     * @brief 构造函数
     * @param profile 扫描轮廓（可以是Face、Wire或Edge）
     * @param path 扫描路径
     */
    SweepOperation(const TopoDS_Shape& profile, SweepPath* path);
    
    ~SweepOperation();

    /**
     * @brief 执行扫描操作
     * @return 生成的3D形状
     */
    TopoDS_Shape execute();

    /**
     * @brief 检查操作是否有效
     */
    bool isValid() const;

    /**
     * @brief 获取错误信息
     */
    QString getErrorMessage() const;

    /**
     * @brief 设置是否创建实体（true）或壳体（false）
     */
    void setMakeSolid(bool makeSolid);
    bool getMakeSolid() const;

    /**
     * @brief 从草图创建扫描操作
     * @param sketch 草图对象
     * @param path 扫描路径
     */
    static SweepOperation* fromSketch(const Sketch& sketch, SweepPath* path);

private:
    /**
     * @brief 使用OpenCASCADE的BRepPrimAPI_MakePrism进行线性拉伸
     */
    TopoDS_Shape performLinearSweep(const TopoDS_Shape& profile, const LinearSweepPath* path);

    /**
     * @brief 使用OpenCASCADE的BRepPrimAPI_MakeRevol进行旋转
     */
    TopoDS_Shape performRevolveSweep(const TopoDS_Shape& profile, const RevolveSweepPath* path);

    /**
     * @brief 通用扫描方法（用于未来扩展）
     */
    TopoDS_Shape performGeneralSweep(const TopoDS_Shape& profile, const SweepPath* path);

    TopoDS_Shape profileShape;  // 扫描轮廓
    SweepPath* sweepPath;        // 扫描路径（不拥有所有权，由调用者管理）
    bool makeSolid;              // 是否创建实体
    QString errorMessage;       // 错误信息
};

#endif // SWEEPOPERATION_H

