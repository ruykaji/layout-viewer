#include "geometry.hpp"

Point::Point(const PointF& t_point)
{
    x = static_cast<int32_t>(t_point.x);
    y = static_cast<int32_t>(t_point.y);
};

Rectangle::Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const MetalLayer& t_layer, const RectangleType& t_type)
    : type(t_type)
    , layer(t_layer)
{
    int32_t minX = std::min(t_xh, t_xl);
    int32_t maxX = std::max(t_xh, t_xl);
    int32_t minY = std::min(t_yh, t_yl);
    int32_t maxY = std::max(t_yh, t_yl);

    vertex[0] = Point(minX, minY);
    vertex[1] = Point(maxX, minY);
    vertex[2] = Point(maxX, maxY);
    vertex[3] = Point(minX, maxY);
}

Rectangle::Rectangle(const RectangleF& t_rect)
{
    type = t_rect.type;
    layer = t_rect.layer;

    vertex[0] = Point(t_rect.vertex[0]);
    vertex[1] = Point(t_rect.vertex[1]);
    vertex[2] = Point(t_rect.vertex[2]);
    vertex[3] = Point(t_rect.vertex[3]);
}

void Rectangle::fixVertex()
{
    int32_t minX = std::min(vertex[0].x, vertex[2].x);
    int32_t maxX = std::max(vertex[0].x, vertex[2].x);
    int32_t minY = std::min(vertex[0].y, vertex[2].y);
    int32_t maxY = std::max(vertex[0].y, vertex[2].y);

    vertex[0] = Point(minX, minY);
    vertex[1] = Point(maxX, minY);
    vertex[2] = Point(maxX, maxY);
    vertex[3] = Point(minX, maxY);
}

RectangleF::RectangleF(const double& t_xl, const double& t_yl, const double& t_xh, const double& t_yh, const MetalLayer& t_layer, const RectangleType& t_type)
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

void RectangleF::fixVertex()
{
    double minX = std::min(vertex[0].x, vertex[2].x);
    double maxX = std::max(vertex[0].x, vertex[2].x);
    double minY = std::min(vertex[0].y, vertex[2].y);
    double maxY = std::max(vertex[0].y, vertex[2].y);

    vertex[0] = PointF(minX, minY);
    vertex[1] = PointF(maxX, minY);
    vertex[2] = PointF(maxX, maxY);
    vertex[3] = PointF(minX, maxY);
}