#ifndef __DEF_H__
#define __DEF_H__

#include <vector>

struct Polygon {
    std::vector<std::pair<int32_t, int32_t>> points {};

    Polygon() = default;
    ~Polygon() = default;

    void addPoint(const int32_t& t_x, const int32_t& t_y)
    {
        points.emplace_back(std::pair<int32_t, int32_t>(t_x, t_y));
    }
};

#pragma pack(push, 1)
struct Def {
    Polygon dieArea {};

    Def() = default;
    ~Def() = default;
};
#pragma pack(push)

#endif