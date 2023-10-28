#ifndef __PDK_H__
#define __PDK_H__

#include <string>
#include <unordered_map>
#include <vector>

#include "geometry.hpp"

struct PDK {
    struct Macro {
        struct Pin {
            std::string name {};
            std::string use { "SIGNAL" };
            std::vector<RectangleF> ports {};

            Pin() = default;
            ~Pin() = default;
        };

        struct OBS {
            std::vector<RectangleF> geometry {};

            OBS() = default;
            ~OBS() = default;
        };

        std::string name {};
        PointF size {};
        std::vector<Pin> pins {};
        OBS obstruction {};

        Macro() = default;
        ~Macro() = default;
    };

    double scale { INT32_MAX };
    std::unordered_map<std::string, Macro> macros {};

    PDK() = default;
    ~PDK() = default;
};

#endif