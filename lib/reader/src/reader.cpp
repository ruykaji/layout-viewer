#include <fstream>
#include <string>

#include "reader.hpp"

Reader::Reader(const std::string_view t_fileName)
{
    std::ifstream fin({ t_fileName.begin(), t_fileName.end() });

    if (fin.is_open()) {
        std::string line {};

        while (std::getline(fin, line)) {
            std::stringstream strStream { line };
            std::vector<std::string> tokens {};
            std::string token {};

            while (std::getline(strStream, token, ' ')) {
                if (token != ";") {
                    tokens.push_back(token);
                }
            }

            Reader::TokenKind tokenKind = defineToken(tokens);

            switch (tokenKind) {
            case Reader::TokenKind::UNITS: {
                def.units = std::stoi(tokens.at(3));
                break;
            }
            case Reader::TokenKind::DIEAREA: {
                def.dieAria = std::pair<uint32_t, uint32_t>(std::stoi(tokens.at(6)), std::stoi(tokens.at(7)));
                break;
            }

            default:
                break;
            }
        }
    }
}

Reader::TokenKind Reader::defineToken(const std::vector<std::string>& t_tokens)
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