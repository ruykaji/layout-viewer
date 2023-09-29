#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdio>
#include <defrReader.hpp>
#include <functional>
#include <memory>
#include <string_view>

#include "def.hpp"
#include "macro.hpp"

class DEFEncoder {
    std::shared_ptr<Def> m_def {};

public:
    explicit DEFEncoder() = default;
    ~DEFEncoder() = default;

    COPY_CONSTRUCTOR_REMOVE(DEFEncoder);
    ASSIGN_OPERATOR_REMOVE(DEFEncoder);

    std::shared_ptr<Def> read(const std::string_view t_fileName);

private:
    static int dieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData) noexcept;
};

#endif