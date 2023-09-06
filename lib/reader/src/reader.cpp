#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "reader.hpp"

inline void trim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](uint8_t ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](uint8_t ch) { return !std::isspace(ch); }).base(), str.end());
};

Reader::Reader(const std::string_view t_fileName)
{
    std::ifstream fin({ t_fileName.begin(), t_fileName.end() });

    if (fin.is_open()) {
        std::string line {};

        while (std::getline(fin, line)) {
            trim(line);

            std::vector<std::string> tokens = parseLine(std::string_view(line));
            Reader::TokenKind tokenKind = defineToken(tokens);

            switch (tokenKind) {
            case Reader::TokenKind::UNITS: {
                def.units = std::stoi(tokens.at(3));
                break;
            }
            case Reader::TokenKind::DIEAREA: {
                def.dieAria = { std::stoi(tokens.at(6)), std::stoi(tokens.at(7)) };
                break;
            }
            case Reader::TokenKind::VIAS: {

                while (std::getline(fin, line) && line != "END VIAS") {
                    trim(line);

                    tokens = parseLine(std::string_view(line), '+');

                    Def::Via via;

                    via.name = tokens.at(1);
                    via.viaRule = tokens.at(4);
                    via.cutSize = { std::stoi(tokens.at(7)), std::stoi(tokens.at(8)) };
                    via.layers.push_back(tokens.at(11));
                    via.layers.push_back(tokens.at(12));
                    via.layers.push_back(tokens.at(13));
                    via.cutSpacing = { std::stoi(tokens.at(16)), std::stoi(tokens.at(17)) };
                    via.enclosure = {
                        static_cast<uint16_t>(std::stoi(tokens.at(20))),
                        static_cast<uint16_t>(std::stoi(tokens.at(21))),
                        static_cast<uint16_t>(std::stoi(tokens.at(22))),
                        static_cast<uint16_t>(std::stoi(tokens.at(23)))
                    };
                    via.rowCol = { std::stoi(tokens.at(26)), std::stoi(tokens.at(27)) };
                }

                break;
            }

            default:
                break;
            }
        }
    }
}

std::vector<std::string> Reader::parseLine(const std::string_view t_str, const char ch) noexcept
{
    std::stringstream strStream { { t_str.begin(), t_str.end() } };
    std::vector<std::string> tokens {};
    std::string token {};

    while (std::getline(strStream, token, ch)) {
        std::cout << token << '\n';
        tokens.push_back(token);
    }

    return tokens;
};

Reader::TokenKind Reader::defineToken(const std::vector<std::string>& t_tokens) noexcept
{
    if (t_tokens.size() != 0) {
        if (t_tokens[0] == "UNITS") {
            return Reader::TokenKind::UNITS;
        }
        if (t_tokens[0] == "DIEAREA") {
            return Reader::TokenKind::DIEAREA;
        }
        if (t_tokens[0] == "VIAS") {
            return Reader::TokenKind::VIAS;
        }
        if (t_tokens[0] == "PINS") {
            return Reader::TokenKind::PINS;
        }
    }

    return Reader::TokenKind::NONE;
};