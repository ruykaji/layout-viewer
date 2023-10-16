#include "def.hpp"

Rectangle::Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const RType& t_type, const ML& t_layer)
    : rType(t_type)
    , Geometry(t_layer)
{
    // Protection from improper declaration of rect
    int32_t minX = std::min(t_xh, t_xl);
    int32_t maxX = std::max(t_xh, t_xl);
    int32_t minY = std::min(t_yh, t_yl);
    int32_t maxY = std::max(t_yh, t_yl);

    vertex[0] = Point(minX, minY);
    vertex[1] = Point(maxX, minY);
    vertex[2] = Point(maxX, maxY);
    vertex[3] = Point(minX, maxY);
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