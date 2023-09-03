#ifndef __LEF_H__
#define __LEF_H__

#include <string>
#include <vector>

struct Rect {
    std::pair<double, double> topLeft { 0.0, 0.0 };
    std::pair<double, double> bottomRight { 0.0, 0.0 };

    Rect() = default;
    ~Rect() = default;
};

struct LayerGeometries {
    std::string name {};
    std::vector<Rect> geometries {};

    LayerGeometries() = default;
    ~LayerGeometries() = default;

    LayerGeometries(const std::string_view t_name)
        : name(t_name) {};
};

struct OBS {
    std::vector<Rect> layerGeometries {};

    OBS() = default;
    ~OBS() = default;
};

struct Port {
    enum class Class {
        NONE,
        CORE,
        BUMP
    };

    Class className { Class::NONE };
    std::vector<LayerGeometries> layer {};

    Port() = default;
    ~Port() = default;
};

struct Pin {
    enum class Use {
        SIGNAL,
        ANALOG,
        POWER,
        GROUND,
        CLOCK
    };

    std::string name {};
    // TAPERRULE
    // DIRECTION
    Use use { Use::SIGNAL };
    // NETEXPR
    // SUPPLYSENSITIVITY
    // GROUNDSENSITIVITY
    // SHAPE
    // MUSTJOIN
    std::vector<Port> ports {};
    // PROPERTY
    // ANTENNAPARTIALMETALAREA
    // ANTENNAPARTIALMETALSIDEAREA
    // ANTENNAPARTIALCUTAREA
    // ANTENNADIFFAREA
    // ANTENNAMODEL
    // ANTENNAGATEAREA
    // ANTENNAMAXAREACAR
    // ANTENNAMAXSIDEAREACAR
    // ANTENNAMAXCUTCAR

    Pin() = default;
    ~Pin() = default;

    Pin(const std::string_view t_name)
        : name(t_name) {};
};

struct Foreign {
    std::string name {};
    std::pair<double, double> offset {};
    char orient {};

    Foreign() = default;
    ~Foreign() = default;

    Foreign(const std::string t_name, const std::pair<double, double> t_offset = { 0.0, 0.0 }, const char t_orient = 'N')
        : name(t_name)
        , offset(t_offset)
        , orient(t_orient) {};
};

struct Macro {
    enum class Class {
        COVER,
        RING,
        BLOCK,
        PAD,
        CORE,
        ENDCAP
    };

    std::string name {};
    Class className {};
    // FIXEDMASK
    Foreign foreign {};
    std::pair<double, double> origin { 0.0, 0.0 };
    // EEQ
    std::pair<double, double> size { 0.0, 0.0 };
    // SYMMETRY
    // SITE
    std::vector<Pin> pins {};
    OBS obs {};
    // DENSITY
    // PROPERTY

    Macro() = default;
    ~Macro() = default;
};

struct Lef {
    std::string version {};
    bool nameCaseSensetive { true };
    std::string busBitChars { "[]" };
    char dividerChar { '/' };
    // UNITS
    // MANUFACTURINGGRID
    // USEMINSPACING
    // CLEARANCEMEASURE
    // PROPERTYDEFINITIONS
    // FIXEDMASK
    // LAYER
    // MAXVIASTACK
    // VIARULE
    // VIA
    // NONDEFAULTRULE
    // SITE
    Macro macro {};
    // BEGINNEXT
};

#endif