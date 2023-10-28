#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <array>
#include <cstdint>

enum class MetalLayer {
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

enum class RectangleType {
    NONE = 0,
    DIEAREA,
    SIGNAL,
    CLOCK,
    POWER,
    GROUND,
    PIN
};

struct PointF;

struct Point {
    int32_t x {};
    int32_t y {};

    Point() = default;
    ~Point() = default;

    Point(const int32_t& t_x, const int32_t& t_y)
        : x(t_x)
        , y(t_y) {};
    Point(const PointF& t_point);
};

struct PointF {
    double x {};
    double y {};

    PointF() = default;
    ~PointF() = default;

    PointF(const double& t_x, const double& t_y)
        : x(t_x)
        , y(t_y) {};
};

struct RectangleF;

struct Rectangle {
    RectangleType type { RectangleType::NONE };
    MetalLayer layer { MetalLayer::NONE };
    std::array<Point, 4> vertex {};

    Rectangle() = default;
    ~Rectangle() = default;

    Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const RectangleType& t_type = RectangleType::NONE);
    Rectangle(const RectangleF& t_rect);

    void fixVertex();
};

struct RectangleF {
    RectangleType type { RectangleType::NONE };
    MetalLayer layer { MetalLayer::NONE };
    std::array<PointF, 4> vertex {};

    RectangleF() = default;
    ~RectangleF() = default;

    RectangleF(const double& t_xl, const double& t_yl, const double& t_xh, const double& t_yh, const MetalLayer& t_layer, const RectangleType& t_type = RectangleType::NONE);

    void fixVertex();
};

#endif