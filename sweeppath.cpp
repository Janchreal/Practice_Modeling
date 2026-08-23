#include "sweeppath.h"
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <cmath>

// ==================== LinearSweepPath 实现 ====================

LinearSweepPath::LinearSweepPath(const gp_Pnt& startPoint, const gp_Dir& direction, double distance)
    : startPoint(startPoint), direction(direction), distance(distance)
{
    // 计算终点
    gp_Vec vec(direction);
    vec.Scale(distance);
    endPoint = startPoint.Translated(vec);
}

QString LinearSweepPath::getTypeName() const
{
    return "Linear";
}

gp_Vec LinearSweepPath::getTranslationAt(double t) const
{
    // t在[0,1]范围内，0表示起点，1表示终点
    gp_Vec vec(direction);
    vec.Scale(distance * t);
    return vec;
}

gp_Dir LinearSweepPath::getDirectionAt(double t) const
{
    // 线性路径方向不变
    return direction;
}

gp_Pnt LinearSweepPath::getStartPoint() const
{
    return startPoint;
}

gp_Pnt LinearSweepPath::getEndPoint() const
{
    return endPoint;
}

bool LinearSweepPath::isValid() const
{
    return distance > 0.0 && !direction.IsEqual(gp_Dir(0, 0, 0), 1e-9);
}

double LinearSweepPath::getLength() const
{
    return distance;
}

gp_Pnt LinearSweepPath::getPointAt(double t) const
{
    gp_Vec translation = getTranslationAt(t);
    return startPoint.Translated(translation);
}

void LinearSweepPath::setDirection(const gp_Dir& dir)
{
    direction = dir;
    // 重新计算终点
    gp_Vec vec(direction);
    vec.Scale(distance);
    endPoint = startPoint.Translated(vec);
}

gp_Dir LinearSweepPath::getDirection() const
{
    return direction;
}

void LinearSweepPath::setDistance(double dist)
{
    distance = dist;
    // 重新计算终点
    gp_Vec vec(direction);
    vec.Scale(distance);
    endPoint = startPoint.Translated(vec);
}

double LinearSweepPath::getDistance() const
{
    return distance;
}

// ==================== RevolveSweepPath 实现（预留） ====================

RevolveSweepPath::RevolveSweepPath(const gp_Ax1& axis, double angle)
    : axis(axis), angle(angle)
{
    // TODO: 实现旋转路径的初始化
}

QString RevolveSweepPath::getTypeName() const
{
    return "Revolve";
}

gp_Vec RevolveSweepPath::getTranslationAt(double t) const
{
    // TODO: 实现旋转路径的平移计算
    // 这需要根据旋转轴和角度计算每个参数点的位置
    return gp_Vec(0, 0, 0);
}

gp_Dir RevolveSweepPath::getDirectionAt(double t) const
{
    // TODO: 实现旋转路径的方向计算
    return gp_Dir(0, 0, 1);
}

gp_Pnt RevolveSweepPath::getStartPoint() const
{
    // TODO: 实现旋转路径起点计算
    return axis.Location();
}

gp_Pnt RevolveSweepPath::getEndPoint() const
{
    // TODO: 实现旋转路径终点计算
    return axis.Location();
}

bool RevolveSweepPath::isValid() const
{
    // TODO: 实现有效性检查
    return std::abs(angle) > 1e-9;
}

double RevolveSweepPath::getLength() const
{
    // TODO: 计算旋转路径的弧长
    return std::abs(angle);
}

gp_Pnt RevolveSweepPath::getPointAt(double t) const
{
    // TODO: 实现旋转路径上点的计算
    return axis.Location();
}

