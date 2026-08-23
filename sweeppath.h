#ifndef SWEEPPATH_H
#define SWEEPPATH_H

#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <QString>

/**
 * @brief 扫描路径抽象基类
 * 
 * 这是扫描操作的核心抽象，遵循开闭原则：
 * - 对扩展开放：可以轻松添加新的扫描类型（如螺旋扫描、沿曲线扫描等）
 * - 对修改封闭：核心扫描算法不需要修改，只需添加新的路径类
 * 
 * 设计模式：策略模式 + 模板方法模式
 */
class SweepPath {
public:
    virtual ~SweepPath() = default;

    /**
     * @brief 获取扫描路径的类型名称
     */
    virtual QString getTypeName() const = 0;

    /**
     * @brief 获取路径在参数t处的变换矩阵（用于扫描）
     * @param t 参数值，通常在[0, 1]范围内
     * @return 变换向量（平移）和方向
     */
    virtual gp_Vec getTranslationAt(double t) const = 0;
    virtual gp_Dir getDirectionAt(double t) const = 0;

    /**
     * @brief 获取路径起点
     */
    virtual gp_Pnt getStartPoint() const = 0;

    /**
     * @brief 获取路径终点
     */
    virtual gp_Pnt getEndPoint() const = 0;

    /**
     * @brief 检查路径是否有效
     */
    virtual bool isValid() const = 0;

    /**
     * @brief 获取路径长度（用于参数化）
     */
    virtual double getLength() const = 0;

    /**
     * @brief 获取路径在参数t处的点
     */
    virtual gp_Pnt getPointAt(double t) const = 0;
};

/**
 * @brief 线性扫描路径 - 用于拉伸操作
 * 
 * 这是最简单的扫描路径，沿直线方向拉伸。
 * 作为第一个实现，便于调试和验证架构。
 */
class LinearSweepPath : public SweepPath {
public:
    /**
     * @brief 构造函数
     * @param startPoint 起点
     * @param direction 拉伸方向
     * @param distance 拉伸距离
     */
    LinearSweepPath(const gp_Pnt& startPoint, const gp_Dir& direction, double distance);

    QString getTypeName() const override;
    gp_Vec getTranslationAt(double t) const override;
    gp_Dir getDirectionAt(double t) const override;
    gp_Pnt getStartPoint() const override;
    gp_Pnt getEndPoint() const override;
    bool isValid() const override;
    double getLength() const override;
    gp_Pnt getPointAt(double t) const override;

    /**
     * @brief 设置拉伸方向
     */
    void setDirection(const gp_Dir& direction);
    gp_Dir getDirection() const;

    /**
     * @brief 设置拉伸距离
     */
    void setDistance(double distance);
    double getDistance() const;

private:
    gp_Pnt startPoint;
    gp_Dir direction;
    double distance;
    gp_Pnt endPoint;
};

/**
 * @brief 旋转扫描路径 - 用于旋转操作（预留接口）
 * 
 * 这个类为未来的旋转功能预留接口。
 * 实现时只需改变轨迹生成部分，核心扫描算法可以重用。
 */
class RevolveSweepPath : public SweepPath {
public:
    /**
     * @brief 构造函数
     * @param axis 旋转轴
     * @param angle 旋转角度（弧度）
     */
    RevolveSweepPath(const gp_Ax1& axis, double angle);

    QString getTypeName() const override;
    gp_Vec getTranslationAt(double t) const override;
    gp_Dir getDirectionAt(double t) const override;
    gp_Pnt getStartPoint() const override;
    gp_Pnt getEndPoint() const override;
    bool isValid() const override;
    double getLength() const override;
    gp_Pnt getPointAt(double t) const override;

private:
    gp_Ax1 axis;
    double angle;
    // TODO: 实现旋转路径的具体逻辑
};

#endif // SWEEPPATH_H

