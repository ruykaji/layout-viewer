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
    Reader() = default;
    ~Reader() = default;

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(const std::string_view t_fileName);

private:
    TokenKind defineToken(const std::vector<std::string>& t_tokens);

    Def::Via makeVia(const std::stringstream& t_strStream);
    // Def::Component makeComponent(const std::stringstream& t_strStream);
    Def::Pin makePin(const std::stringstream& t_strStream);
};

#endif