#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct Point;
struct Pin;
struct WorkingCell;
struct Via;
struct Def;

// Rectangle type
enum class RType {
    NONE = 0,
    SIGNAL,
    PIN
};

// Metal layers
enum class ML {
    L1 = 0,
    L1M1_V,
    M1,
    M1M2_V,
    M2,
    M2M3_V,
    M3,
    M3M4_V,
    M4,
    M4M5_V,
    M5,
    M5M6_V,
    M6,
    M6M7_V,
    M7,
    M7M8_V,
    M8,
    M8M9_V,
    M9,
    NONE,
};

struct Point {
    int32_t x {};
    int32_t y {};

    Point() = default;
    ~Point() = default;

    Point(const int32_t& t_x, const int32_t& t_y)
        : x(t_x)
        , y(t_y) {};
};

struct Geometry {
    ML layer { ML::NONE };

    Geometry() = default;
    ~Geometry() = default;

    Geometry(const ML& t_layer)
        : layer(t_layer) {};
};

struct Rectangle : public Geometry {
    RType type { RType::NONE };
    std::array<Point, 4> vertex {};

    Rectangle() = default;
    ~Rectangle() = default;

    Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const RType& t_type, const ML& t_layer);

    void fixVertex();
};

struct Pin : public Rectangle {
    int32_t netIndex {};
    std::string name {};
    std::shared_ptr<WorkingCell> parentCell {};

    Pin() = default;
    ~Pin() = default;

    Pin(const std::string& t_name, const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const ML& t_layer)
        : name(t_name)
        , Rectangle(t_xl, t_yl, t_xh, t_yh, RType::PIN, t_layer) {};
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

    WorkingCell() = default;
    ~WorkingCell() = default;
};

struct Def {
    int32_t cellSize {};
    int32_t cellStepX {};
    int32_t cellStepY {};
    int32_t numCellX {};
    int32_t numCellY {};
    int32_t cellOffsetX {};
    int32_t cellOffsetY {};
    int32_t totalNets {};

    std::vector<Point> dieArea {};

    std::vector<std::vector<std::shared_ptr<WorkingCell>>> cells {};
    std::vector<std::shared_ptr<Geometry>> geometries {};

    std::vector<std::shared_ptr<Rectangle>> component {};
    std::string componentName {};

    std::unordered_map<std::string, std::shared_ptr<Pin>> pins {};
    std::unordered_map<std::string, std::vector<Rectangle>> vias {};

    Def() = default;
    ~Def() = default;
};

#endif