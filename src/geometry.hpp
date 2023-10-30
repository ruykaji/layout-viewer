#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <array>
#include <cstdint>

struct Point;
struct PointF;
struct Rectangle;
struct RectangleF;

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

struct Point {
    int32_t x {};
    int32_t y {};

    Point() = default;
    ~Point() = default;

    constexpr Point(const PointF& t_point) noexcept;
    constexpr Point(const int32_t& t_x, const int32_t& t_y) noexcept
        : x(t_x)
        , y(t_y) {};

    constexpr inline Point& operator+=(const Point& t_rhs) noexcept;
    constexpr inline Point& operator-=(const Point& t_rhs) noexcept;

    constexpr inline Point& operator*=(const int32_t& t_rhs) noexcept;
    constexpr inline Point& operator/=(const int32_t& t_rhs);

    friend constexpr inline Point operator+(const Point& t_lhs, const Point& t_rhs) noexcept
    {
        return Point(t_lhs.x + t_rhs.x, t_lhs.y + t_rhs.y);
    };
    friend constexpr inline Point operator-(const Point& t_lhs, const Point& t_rhs) noexcept
    {
        return Point(t_lhs.x - t_rhs.x, t_lhs.y - t_rhs.y);
    };

    friend constexpr inline Point operator+(const Point& t_lhs, const int32_t& t_rhs) noexcept
    {
        return Point(t_lhs.x + t_rhs, t_lhs.y + t_rhs);
    };
    friend constexpr inline Point operator-(const Point& t_lhs, const int32_t& t_rhs) noexcept
    {
        return Point(t_lhs.x - t_rhs, t_lhs.y - t_rhs);
    };
    friend constexpr inline Point operator*(const Point& t_lhs, const int32_t& t_rhs) noexcept
    {
        return Point(t_lhs.x * t_rhs, t_lhs.y * t_rhs);
    };
    friend constexpr inline Point operator/(const Point& t_lhs, const int32_t& t_rhs) noexcept
    {
        return Point(t_lhs.x / t_rhs, t_lhs.y / t_rhs);
    };
};

struct PointF {
    double x {};
    double y {};

    PointF() = default;
    ~PointF() = default;

    constexpr PointF(const double& t_x, const double& t_y) noexcept
        : x(t_x)
        , y(t_y) {};
};

constexpr Point::Point(const PointF& t_point) noexcept
{
    x = static_cast<int32_t>(t_point.x);
    y = static_cast<int32_t>(t_point.y);
};

constexpr inline Point& Point::operator+=(const Point& t_rhs) noexcept
{
    x += t_rhs.x;
    y += t_rhs.y;

    return *this;
};

constexpr inline Point& Point::operator-=(const Point& t_rhs) noexcept
{
    x -= t_rhs.x;
    y -= t_rhs.y;

    return *this;
};

constexpr inline Point& Point::operator*=(const int32_t& t_rhs) noexcept
{
    x *= t_rhs;
    y *= t_rhs;

    return *this;
};

constexpr inline Point& Point::operator/=(const int32_t& t_rhs)
{
    x /= t_rhs;
    y /= t_rhs;

    return *this;
};

struct Rectangle {
    RectangleType type { RectangleType::NONE };
    MetalLayer layer { MetalLayer::NONE };
    std::array<Point, 4> vertex {};

    Rectangle() = default;
    ~Rectangle() = default;

    constexpr Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const RectangleType& t_type = RectangleType::NONE) noexcept;
    constexpr Rectangle(const RectangleF& t_rect) noexcept;

    inline void fixVertex() noexcept;
};

struct RectangleF {
    RectangleType type { RectangleType::NONE };
    MetalLayer layer { MetalLayer::NONE };
    std::array<PointF, 4> vertex {};

    RectangleF() = default;
    ~RectangleF() = default;

    constexpr RectangleF(const double& t_xl, const double& t_yl, const double& t_xh, const double& t_yh, const MetalLayer& t_layer, const RectangleType& t_type = RectangleType::NONE) noexcept;

    inline void fixVertex() noexcept;
};

constexpr Rectangle::Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const RectangleType& t_type) noexcept
    : type(t_type)
    , layer(t_layer)
{
    int32_t minX = std::min(t_xh, t_xl);
    int32_t maxX = std::max(t_xh, t_xl);
    int32_t minY = std::min(t_yh, t_yl);
    int32_t maxY = std::max(t_yh, t_yl);

    vertex = {
        PointF(minX, minY),
        PointF(maxX, minY),
        PointF(maxX, maxY),
        PointF(minX, maxY)
    };
}

constexpr Rectangle::Rectangle(const RectangleF& t_rect) noexcept
    : type(t_rect.type)
    , layer(t_rect.layer)
{
    vertex = {
        Point(t_rect.vertex[0]),
        Point(t_rect.vertex[1]),
        Point(t_rect.vertex[2]),
        Point(t_rect.vertex[3])
    };
}

inline void Rectangle::fixVertex() noexcept
{
    int32_t minX = std::min(vertex[0].x, vertex[2].x);
    int32_t maxX = std::max(vertex[0].x, vertex[2].x);
    int32_t minY = std::min(vertex[0].y, vertex[2].y);
    int32_t maxY = std::max(vertex[0].y, vertex[2].y);

    vertex = {
        PointF(minX, minY),
        PointF(maxX, minY),
        PointF(maxX, maxY),
        PointF(minX, maxY)
    };
}

constexpr RectangleF::RectangleF(const double& t_xl, const double& t_yl, const double& t_xh, const double& t_yh, const MetalLayer& t_layer, const RectangleType& t_type) noexcept
    : type(t_type)
    , layer(t_layer)
{
    double minX = std::min(t_xh, t_xl);
    double maxX = std::max(t_xh, t_xl);
    double minY = std::min(t_yh, t_yl);
    double maxY = std::max(t_yh, t_yl);

    vertex[0] = PointF(minX, minY);
    vertex[1] = PointF(maxX, minY);
    vertex[2] = PointF(maxX, maxY);
    vertex[3] = PointF(minX, maxY);
}

inline void RectangleF::fixVertex() noexcept
{
    double minX = std::min(vertex[0].x, vertex[2].x);
    double maxX = std::max(vertex[0].x, vertex[2].x);
    double minY = std::min(vertex[0].y, vertex[2].y);
    double maxY = std::max(vertex[0].y, vertex[2].y);

    vertex = {
        PointF(minX, minY),
        PointF(maxX, minY),
        PointF(maxX, maxY),
        PointF(minX, maxY)
    };
}

#endif