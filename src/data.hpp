#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "geometry.hpp"
#include "torch_include.hpp"

struct Pin;
struct Rout;
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

struct Route : public Rectangle {
    int32_t netIndex {};

    Route() = default;
    ~Route() = default;

    explicit Route(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const int32_t& t_netIndex = 0)
        : netIndex(t_netIndex)
        , Rectangle(t_xl, t_yl, t_xh, t_yh, t_layer, RectangleType::SIGNAL) {};
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
    std::vector<std::shared_ptr<Rectangle>> tracks {};
    std::vector<std::shared_ptr<Route>> routes {};
    std::vector<std::shared_ptr<Route>> maskedRoutes {};
    std::unordered_multimap<std::string, std::shared_ptr<Pin>> pins {};
    std::unordered_map<int32_t, Net> nets {};

    torch::Tensor source {};
    torch::Tensor cellInformation {};
    torch::Tensor target {};

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
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<WorkingCell>>> correspondingToPinCells {};
    // std::unordered_map<std::string, std::vector<Rectangle>> vias {};

    Data() = default;
    ~Data();
};

#endif