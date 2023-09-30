#ifndef __DEF_H__
#define __DEF_H__

#include <vector>

#include "types.hpp"

struct Polygon {
    std::vector<Point> points {};

    Polygon() = default;
    ~Polygon() = default;

    void append(const int32_t& t_x, const int32_t& t_y)
    {
        points.emplace_back(Point(t_x, t_y));
    }
};

#pragma pack(push, 1)
struct Def {
    Polygon dieArea {};
    std::vector<Polygon> gCellGridX {};
    std::vector<Polygon> gCellGridY {};

    Def() = default;
    ~Def() = default;
};
#pragma pack(push)

#endif