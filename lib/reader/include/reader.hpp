#ifndef __READER_H__
#define __READER_H__

#include <string_view>

#include "lef.hpp"

class Reader {

public:
    Reader() = default;
    ~Reader() = default;

    Lef parseLef(const std::string_view t_fileName);
};

#endif