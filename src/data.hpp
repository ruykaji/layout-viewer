#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "geometry.hpp"
#include "torch_include.hpp"

struct WorkingCell;
struct Data;

struct WorkingCell {
    struct Connection {
        int32_t isOutOfCell {};
        Point start {};
        int32_t startLayer {};
        Point end {};
        int32_t endLayer {};

        Connection() = default;
        ~Connection() = default;
    };

    std::string name {};
    Rectangle originalPlace {};
    std::vector<std::shared_ptr<Rectangle>> geometries {};
    std::vector<std::string> pins {};
    std::unordered_map<std::string, std::vector<Connection>> nets {};

    WorkingCell() = default;
    ~WorkingCell() = default;
};

struct Data {
    std::string design {};

    int32_t cellSize {};
    int32_t numCellX {};
    int32_t numCellY {};
    int32_t cellOffsetX {};
    int32_t cellOffsetY {};
    int32_t totalNets {};

    std::vector<Point> dieArea {};
    std::vector<std::vector<std::shared_ptr<WorkingCell>>> cells {};

    std::unordered_map<std::string, std::shared_ptr<WorkingCell>> correspondingToPinCell {};
    std::unordered_map<std::string, std::vector<std::string>> signalNets {};

    std::unordered_map<std::string, std::shared_ptr<Rectangle>> pins {};

    Data() = default;
    ~Data();
};

#endif