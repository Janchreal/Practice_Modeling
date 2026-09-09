#ifndef PATTERNFEATURETYPES_H
#define PATTERNFEATURETYPES_H

enum class PatternLayoutType {
    Linear = 0,
    Circular = 1,
    Polygonal = 2
};

/** 多边形间距：每边数目 / 沿边节距 */
enum class PolygonSpacingMode {
    CountPerSide = 0,
    PitchAlongEdge = 1
};

/** 点捕捉：-1 任意点（面射线求交），0~5 为 snap 捕捉类型 */
constexpr int kPatternPointSnapArbitrary = -1;

#endif // PATTERNFEATURETYPES_H
