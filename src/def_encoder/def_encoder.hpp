#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <functional>
#include <memory>
#include <string_view>

#include "def.hpp"
#include "macro.hpp"

class Encoder {
    std::unique_ptr<Def> m_def { new Def() };

public:
    Encoder() = default;
    ~Encoder() = default;

    COPY_CONSTRUCTOR_REMOVE(Encoder);
    ASSIGN_OPERATOR_REMOVE(Encoder);

    void read(const std::string_view t_fileName);

private:
    static int dieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData) noexcept;
};

#endif