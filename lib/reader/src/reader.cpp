#include "reader.hpp"

#include <fstream>
#include <string>

Lef Reader::parseLef(const std::string_view t_fileName)
{
    Lef lef;

    std::ifstream fin { std::string(t_fileName) };
    std::string line {};

    if (fin.is_open()) {
        while (std::getline(fin, line)) {
        }
    }
}