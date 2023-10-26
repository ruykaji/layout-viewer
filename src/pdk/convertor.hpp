#ifndef __CONVERTOR_H__
#define __CONVERTOR_H__

#include <filesystem>
#include <iostream>
#include <lefrReader.hpp>
#include <string_view>

#include "macro.hpp"
#include "pdk/pdk.hpp"

class Convertor {

public:
    Convertor() = default;
    ~Convertor() = default;

    COPY_CONSTRUCTOR_REMOVE(Convertor);
    ASSIGN_OPERATOR_REMOVE(Convertor);

    void serialize(const std::string& t_directory);
    void deserialize(const std::string& t_fileName, PDK& t_pdk);

private:
    static int macroCallback(lefrCallbackType_e t_type, const char* t_string, void* t_userData);

    static int macroSizeCallback(lefrCallbackType_e t_type, lefiNum t_numbers, void* t_userData);

    static int pinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData);

    static int obstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData);
};

#endif