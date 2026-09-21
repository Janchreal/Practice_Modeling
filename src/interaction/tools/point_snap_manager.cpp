#include "point_snap_manager.h"

PointSnapMode PointSnapManager::modeFromLegacySnapKind(int snapKind)
{
    PointSnapMode mode;
    switch (snapKind) {
    case -1:
        mode.endpoint = true;
        mode.midpoint = true;
        break;
    case 0:
        mode.nearest = true;
        mode.endpoint = true;
        mode.midpoint = true;
        mode.center = true;
        mode.quadrant = true;
        break;
    case 1:
        mode.endpoint = true;
        break;
    case 2:
        mode.midpoint = true;
        break;
    case 3:
        mode.intersection = true;
        break;
    case 4:
        mode.center = true;
        break;
    case 5:
        mode.quadrant = true;
        break;
    case 6:
        mode.arcMidpoint = true;
        break;
    default:
        mode.endpoint = true;
        break;
    }
    return mode;
}

PointSnapType PointSnapManager::typeFromLegacyCandidateKind(int kind)
{
    switch (kind) {
    case 1: return PointSnapType::Endpoint;
    case 2: return PointSnapType::Midpoint;
    case 3: return PointSnapType::Intersection;
    case 4: return PointSnapType::Center;
    case 5: return PointSnapType::Quadrant;
    case 6: return PointSnapType::ArcMidpoint;
    default: return PointSnapType::None;
    }
}

int PointSnapManager::legacyCandidateKind(PointSnapType type)
{
    switch (type) {
    case PointSnapType::Endpoint: return 1;
    case PointSnapType::Midpoint: return 2;
    case PointSnapType::Intersection: return 3;
    case PointSnapType::Center: return 4;
    case PointSnapType::Quadrant: return 5;
    case PointSnapType::ArcMidpoint: return 6;
    default: return 0;
    }
}

QString PointSnapManager::labelForType(PointSnapType type)
{
    switch (type) {
    case PointSnapType::Endpoint: return QStringLiteral("端点");
    case PointSnapType::Midpoint: return QStringLiteral("中点");
    case PointSnapType::Intersection: return QStringLiteral("交点");
    case PointSnapType::Center: return QStringLiteral("圆心");
    case PointSnapType::Quadrant: return QStringLiteral("象限点");
    case PointSnapType::ArcMidpoint: return QStringLiteral("圆弧中点");
    case PointSnapType::Projection: return QStringLiteral("投影点");
    case PointSnapType::Nearest: return QStringLiteral("最近点");
    case PointSnapType::Grid: return QStringLiteral("网格点");
    case PointSnapType::OnCurve: return QStringLiteral("点在曲线上");
    case PointSnapType::OnFace: return QStringLiteral("面上的点");
    default: return QStringLiteral("捕捉点");
    }
}

int PointSnapManager::priority(PointSnapType type)
{
    switch (type) {
    case PointSnapType::Intersection: return 0;
    case PointSnapType::Endpoint: return 1;
    case PointSnapType::Midpoint:
    case PointSnapType::ArcMidpoint: return 2;
    case PointSnapType::Center: return 3;
    case PointSnapType::Quadrant: return 4;
    case PointSnapType::Projection:
    case PointSnapType::OnCurve: return 5;
    case PointSnapType::OnFace: return 6;
    case PointSnapType::Nearest: return 7;
    case PointSnapType::Grid: return 8;
    default: return 10;
    }
}
