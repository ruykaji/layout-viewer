#ifndef __REPLAY_BUFFER_H__
#define __REPLAY_BUFFER_H__

#include <vector>

#include "torch_include.hpp"

struct Frame {
    torch::Tensor actionProbs {};
    torch::Tensor values {};
    torch::Tensor rewards {};
    torch::Tensor terminals {};

    Frame() = default;
    ~Frame() = default;
};

#endif