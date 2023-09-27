#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <memory>
#include <string_view>

#include "macro.hpp"

class Encoder {
    std::unique_ptr<FILE, int (*)(FILE*)> m_file { nullptr, &fclose };

public:
    Encoder() = default;
    ~Encoder() = default;

    COPY_CONSTRUCTOR_REMOVE(Encoder);
    ASSIGN_OPERATOR_REMOVE(Encoder);

    void initParser();
    void setFile(const std::string_view t_fileName);
    void setCallbacks();
};

#endif