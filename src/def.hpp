#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct Point;
struct Pin;
struct Via;
struct Def;

// Geometry type
enum class GType {
    RECTANGLE = 0,
    LINE,
};

// Line type
enum class LType {
    COMPONENT_ROUTE = 0,
    CLK_ROUTE,
};

// Rectangle type
enum class RType {
    NONE = 0,
    PIN
};

// Metal layers
enum class ML {
    L1 = 0,
    M1,
    M2,
    M3,
    M4,
    M5,
    M6,
    M7,
    M8,
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
    GType gType { GType::RECTANGLE };
    ML layer { ML::NONE };

    Geometry() = default;
    ~Geometry() = default;

    Geometry(const GType& t_type, const ML& t_layer)
        : gType(t_type)
        , layer(t_layer) {};
};

struct Line : public Geometry {
    LType lType { LType::COMPONENT_ROUTE };
    Point start {};
    Point end {};

    Line() = default;
    ~Line() = default;

    Line(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const LType& t_type, const ML& t_layer)
        : lType(t_type)
        , start(Point(t_xl, t_yl))
        , end(Point(t_xh, t_yh))
        , Geometry(GType::LINE, t_layer) {};
};

struct Rectangle : public Geometry {
    RType rType { RType::NONE };
    std::array<Point, 4> vertex {};

    Rectangle() = default;
    ~Rectangle() = default;

    Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const RType& t_type, const ML& t_layer)
        : rType(t_type)
        , Geometry(GType::RECTANGLE, t_layer)
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

    void fixVertex()
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
};

struct Pin : public Rectangle {
    std::string name {};

    Pin() = default;
    ~Pin() = default;

    Pin(const std::string& t_name, const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const ML& t_layer)
        : name(t_name)
        , Rectangle(t_xl, t_yl, t_xh, t_yh, RType::PIN, t_layer) {};
};

struct Matrix {
    Rectangle originalPlace {};

    std::vector<std::shared_ptr<Geometry>> geometries {};
};

struct Def {
    int32_t matrixSize {};
    int32_t matrixStepX {};
    int32_t matrixStepY {};
    int32_t numMatrixX {};
    int32_t numMatrixY {};
    int32_t matrixOffsetX {};
    int32_t matrixOffsetY {};

    std::vector<Point> dieArea {};
    std::vector<Matrix> matrixes {};

    std::vector<std::shared_ptr<Rectangle>> component {};
    std::string componentName {};

    std::vector<std::shared_ptr<Geometry>> geometries {};
    std::map<std::string, std::vector<std::string>> pinConnections {};
    std::map<std::string, std::vector<Rectangle>> vias {};

    Def() = default;
    ~Def() = default;
};

#endif