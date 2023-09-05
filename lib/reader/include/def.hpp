#ifndef __DEF_H__
#define __DEF_H__

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct Def {
    struct Via {
        std::string name {};
        std::string viaRule {};
        std::pair<uint16_t, uint16_t> cutSize {};
        std::vector<std::string> layers {};
        std::pair<uint16_t, uint16_t> cutSpacing {};
        std::array<uint16_t, 4> enclosure {};
        std::pair<uint8_t, uint8_t> rowCol {};
    };

    // struct Component {
    //     enum class Source {
    //         NETLIST,
    //         DIST,
    //         USER,
    //         TIMING
    //     };

    //     std::string name {};
    //     std::string moduleName {};
    //     Source source {};
    //     std::pair<uint32_t, uint32_t> location {};
    //     char orientation {};
    // };

    struct Pin {
        enum class Direction {
            INPUT,
            OUTPUT,
            INOUT,
            FEEDTHRU
        };

        enum class Use {
            SIGNAL,
            POWER,
            GROUND,
            CLOCK,
            TIEOFF,
            ANALOG,
            SCAN,
            RESET
        };

        struct Port {
            struct Layer {
                std::string layer {};
                std::pair<uint32_t, uint32_t> topLeft {};
                std::pair<uint32_t, uint32_t> bottomRight {};
            };

            std::vector<Layer> layers {};
            std::pair<uint32_t, uint32_t> place {};
            char orientation {};
        };

        std::string name {};
        Direction direction {};
        Use use {};
        std::vector<Port> ports {};
    };

    uint16_t units {};
    std::pair<uint32_t, uint32_t> dieAria {};
    std::vector<Via> vias {};
    // std::vector<Component> components {};
};

#endif