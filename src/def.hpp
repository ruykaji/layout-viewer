#ifndef __DEF_H__
#define __DEF_H__

#include <vector>

#include "types.hpp"

struct Def;
struct Polygon;
struct GCellGrid;

struct Polygon {
    std::vector<Point> points {};

    Polygon() = default;
    ~Polygon() = default;

    void append(const int32_t& t_x, const int32_t& t_y)
    {
        points.emplace_back(Point(t_x, t_y));
    }
};

struct GCellGrid {
    std::size_t numY {};
    int32_t offsetY {};
    int32_t stepY {};
    int32_t maxY {};

    std::size_t numX {};
    int32_t offsetX {};
    int32_t stepX {};
    int32_t maxX {};

    std::vector<Polygon> cells {};

    GCellGrid() = default;
    ~GCellGrid() = default;
};

struct Pin {
    std::vector<Polygon> bounds {};

    Pin() = default;
    ~Pin() = default;
};

struct Def {
    Polygon dieArea {};
    GCellGrid gCellGrid {};

    std::vector<Pin> pins {};

    Def() = default;
    ~Def() = default;
};

#endif