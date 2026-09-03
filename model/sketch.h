#ifndef SKETCH_H
#define SKETCH_H

#include "platformmath.h"

#include <TopoDS_Shape.hxx>
#include <gp_Pln.hxx>
#include <QString>
#include <QList>

// 前向声明
class ConstraintSolver;

/**
 * @brief 草图类 - 表示2D草图几何
 * 
 * 草图是参数化建模的基础，包含：
 * - 2D几何元素（点、线、圆弧等）
 * - 约束关系（距离、角度、平行、垂直等）
 * - 约束求解器（用于求解约束系统）
 * 
 * 注意：约束求解器是草图系统的核心，但实现较为复杂。
 * 这里先搭建框架，后续可以集成成熟的约束求解库（如SolverFoundation）。
 */
class Sketch {
public:
    Sketch();
    ~Sketch();

    /**
     * @brief 设置草图平面
     */
    void setPlane(const gp_Pln& plane);
    gp_Pln getPlane() const;

    /**
     * @brief 添加几何元素到草图
     * @param shape 2D形状（应在草图平面内）
     */
    void addGeometry(const TopoDS_Shape& shape);
    QList<TopoDS_Shape> getGeometries() const;
    void setGeometries(const QList<TopoDS_Shape>& shapes);

    /**
     * @brief 获取草图的2D形状
     * @return 草图的TopoDS_Shape（通常是Wire或Face）
     */
    TopoDS_Shape getShape() const;

    /**
     * @brief 检查草图是否有效（是否有几何元素）
     */
    bool isValid() const;

    /**
     * @brief 设置草图名称
     */
    void setName(const QString& name);
    QString getName() const;

    /**
     * @brief 清空草图
     */
    void clear();

    // TODO: 约束相关接口（待实现约束求解器后添加）
    // void addConstraint(ConstraintType type, ...);
    // bool solveConstraints();
    // bool isFullyConstrained() const;

private:
    gp_Pln sketchPlane;              // 草图平面
    QList<TopoDS_Shape> geometries;  // 草图几何元素列表
    TopoDS_Shape sketchShape;         // 组合后的草图形状
    QString sketchName;               // 草图名称
    bool valid;                       // 草图是否有效

    // 约束求解器（未来实现）
    // ConstraintSolver* solver;
};

#endif // SKETCH_H
