#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct Point;
struct Def;
struct Polygon;
struct GCellGrid;
struct Component;
struct Port;
struct Pin;
struct Via;
struct Path;

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
    enum class GType {
        RECTANGLE,
        LINE,
    };

    enum class ML {
        L1,
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

    GType gType { GType::RECTANGLE };
    ML layer { ML::NONE };

    Geometry() = default;
    ~Geometry() = default;

    Geometry(const GType& t_type, const ML& t_layer)
        : gType(t_type)
        , layer(t_layer) {};
};

struct Line : public Geometry {
    enum class LType {
        COMPONENT_ROUTE,
        CLK_ROUTE,
    };

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
    enum class RType {
        NONE,
        PIN,
        VIA
    };

    RType rType { RType::NONE };
    std::array<Point, 4> vertex {};

    Rectangle() = default;
    ~Rectangle() = default;

    Rectangle(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const RType& t_type, const ML& t_layer)
        : rType(t_type)
        , Geometry(GType::RECTANGLE, t_layer)
    {
        vertex[0] = Point(t_xl, t_yl);
        vertex[1] = Point(t_xh, t_yl);
        vertex[2] = Point(t_xh, t_yh);
        vertex[3] = Point(t_xl, t_yh);
    }
};

struct Via {
    std::vector<Rectangle> rects {};

    Via() = default;
    ~Via() = default;
};

struct Pin {
    std::vector<std::shared_ptr<Rectangle>> rects {};
    std::vector<std::shared_ptr<Pin>> targets {};

    Pin() = default;
    ~Pin() = default;
};

struct Matrix {
    Rectangle originalPlace {};

    std::vector<std::shared_ptr<Rectangle>> rects {};
    std::vector<std::shared_ptr<Pin>> pins {};
    std::vector<std::shared_ptr<Line>> paths {};
    std::vector<std::shared_ptr<Line>> routes {};
};

struct Def {
    uint32_t matrixSize {};
    uint32_t matrixStepX {};
    uint32_t matrixStepY {};
    uint32_t numMatrixX {};
    uint32_t numMatrixY {};
    int32_t matrixOffsetX {};
    int32_t matrixOffsetY {};

    std::vector<Point> dieArea {};
    std::vector<Matrix> matrixes {};

    std::vector<std::shared_ptr<Pin>> component {};
    std::string componentName {};

    std::vector<std::shared_ptr<Geometry>> geometries {};
    std::map<std::string, std::shared_ptr<Pin>> pins {};
    std::map<std::string, Via> vias {};

    Def() = default;
    ~Def() = default;
};

#endif