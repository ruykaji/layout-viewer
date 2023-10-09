#ifndef __DEF_H__
#define __DEF_H__

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

struct Polygon {
    enum class Type {
        NONE,
        VPWR,
        VGND,
        VIA,
        PIN,
        PIN_ROUTE,
        CLK_ROUTE,
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

    Type type { Type::NONE };
    ML layer { ML::NONE };
    std::vector<Point> points {};

    Polygon() = default;
    ~Polygon() = default;

    Polygon(const int32_t& t_xl, const int32_t& t_yl, const int32_t& t_xh, const int32_t& t_yh, const ML& t_layer)
        : layer(t_layer)
    {
        points.emplace_back(Point(t_xl, t_yl));
        points.emplace_back(Point(t_xh, t_yl));
        points.emplace_back(Point(t_xh, t_yh));
        points.emplace_back(Point(t_xl, t_yh));
    }

    void append(const int32_t& t_x, const int32_t& t_y)
    {
        points.emplace_back(Point(t_x, t_y));
    }
};

struct Via {
    std::vector<Polygon> polygons {};

    Via() = default;
    ~Via() = default;
};

struct Pin {
    std::vector<std::shared_ptr<Polygon>> polygons {};

    Pin() = default;
    ~Pin() = default;
};

struct Component {
    std::map<std::string, Pin> pins {};

    Component() = default;
    ~Component() = default;
};

struct Route {
    std::string pinA {};
    std::vector<std::string> pinB {};
    std::vector<std::shared_ptr<Polygon>> polygons {};

    Route() = default;
    ~Route() = default;

    Route(const std::string& t_pinA)
        : pinA(t_pinA) {};
};

struct Def {
    Polygon dieArea {};

    std::vector<std::shared_ptr<Polygon>> polygon {};

    std::map<std::string, Component> components {};
    std::map<std::string, Via> vias {};

    std::string lastComponentId {};

    Def() = default;
    ~Def() = default;
};

#endif