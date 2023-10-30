#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "geometry.hpp"
#include "pdk/pdk.hpp"
#include "torch_include.hpp"

struct Pin;
struct Net;
struct WorkingCell;

struct Pin : public Rectangle {
    int32_t netIndex {};
    std::string name {};
    std::shared_ptr<WorkingCell> parentCell {};

    Pin() = default;
    ~Pin() = default;

    explicit Pin(const std::string& t_name, const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const int32_t& t_netIndex = 0)
        : name(t_name)
        , netIndex(t_netIndex)
        , Rectangle(t_xl, t_yl, t_xh, t_yh, t_layer, RectangleType::PIN) {};
};

struct Net {
    int32_t index {};
    std::vector<int8_t> pins {};

    Net() = default;
    ~Net() = default;

    bool operator<(const Net& t_net) const;
};

struct WorkingCell {
    Rectangle originalPlace {};

    std::vector<std::shared_ptr<Rectangle>> geometries {};
    std::vector<std::shared_ptr<Rectangle>> routes {};
    std::vector<std::shared_ptr<Pin>> pins {};
    std::set<Net> nets {};

    torch::Tensor source {};

    WorkingCell() = default;
    ~WorkingCell() = default;
};

struct Data {
    int32_t cellSize {};
    int32_t numCellX {};
    int32_t numCellY {};
    int32_t cellOffsetX {};
    int32_t cellOffsetY {};
    int32_t totalNets {};

    std::vector<Point> dieArea {};
    std::vector<std::vector<std::shared_ptr<WorkingCell>>> cells {};
    std::unordered_map<std::string, std::shared_ptr<Pin>> pins {};
    std::unordered_map<std::string, std::vector<Rectangle>> vias {};

    PDK pdk {};

    Data() = default;
    ~Data();
};

#endif