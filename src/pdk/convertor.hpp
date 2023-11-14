#ifndef __CONVERTOR_H__
#define __CONVERTOR_H__

#include <filesystem>
#include <iostream>
#include <lefrReader.hpp>
#include <string_view>

#include "config.hpp"
#include "pdk/pdk.hpp"

class Convertor {
    struct Data {
        Config config {};
        PDK pdk {};
        std::string lastMacro {};
    };

public:
    Convertor() = default;
    ~Convertor() = default;

    Convertor(const Convertor&) = delete;
    Convertor& operator=(const Convertor&) = delete;

    static void serialize(const std::string& t_directory, const std::string& t_libPath);
    static void deserialize(const std::string& t_libPath, PDK& t_pdk);

private:
    static int unitsCallback(lefrCallbackType_e t_type, lefiUnits* t_units, void* t_userData); 

    static int layerCallback(lefrCallbackType_e t_type, lefiLayer* t_layer, void* t_userData);

    static int macroCallback(lefrCallbackType_e t_type, const char* t_string, void* t_userData);

    static int macroSizeCallback(lefrCallbackType_e t_type, lefiNum t_numbers, void* t_userData);

    static int pinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData);

    static int obstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData);
};

#endif