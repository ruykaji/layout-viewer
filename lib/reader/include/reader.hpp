#ifndef __READER_H__
#define __READER_H__

#include <sstream>
#include <string_view>
#include <vector>

#include "def.hpp"

class Reader {
    enum class TokenKind {
        NONE,
        UNITS,
        DIEAREA,
        PORT,
        LAYER,
        VIAS,
        // COMPONENTS,
        PINS,
        SPECIALNETS,
        NETS,
    };

public:
    Def def {};

    Reader() = default;
    ~Reader() = default;

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(const std::string_view t_fileName);

private:
    std::vector<std::string> parseLine(const std::string_view t_str, const char ch = ' ') noexcept;

    TokenKind defineToken(const std::vector<std::string>& t_tokens) noexcept;
};

#endif