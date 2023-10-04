#ifndef __DEF_H__
#define __DEF_H__

#include <vector>
#include <map>
#include <string>

#include "types.hpp"

struct Def;
struct Polygon;
struct GCellGrid;
struct Port;
struct Pin;
struct Via;

struct Polygon {
    std::vector<Point> points {};

    explicit Polygon() = default;
    ~Polygon() = default;

    Polygon(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh)
    {
        points.emplace_back(Point(t_xl, t_yl));
        points.emplace_back(Point(t_xh, t_yl));
        points.emplace_back(Point(t_xh, t_yh));
        points.emplace_back(Point(t_xl, t_yh));
    }

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

struct Port {
    std::vector<Polygon> polygons {};

    Port() = default;
    ~Port() = default;
};

struct Pin {
    std::vector<Port> ports {};

    Pin() = default;
    ~Pin() = default;
};

struct Via {
    std::vector<Polygon> polygons {};

    Via() = default;
    ~Via() = default;
};

struct Def {
    Polygon dieArea {};
    GCellGrid gCellGrid {};

    std::vector<Pin> pins {};
    std::map<std::string, Via> vias {};

    Def() = default;
    ~Def() = default;
};

#endif