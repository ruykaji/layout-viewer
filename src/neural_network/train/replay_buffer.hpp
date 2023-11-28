#ifndef __REPLAY_BUFFER_H__
#define __REPLAY_BUFFER_H__

#include <array>
#include <vector>

#include "torch_include.hpp"

class ReplayBuffer {
public:
    struct Data {
        std::array<torch::Tensor, 3> env {};
        std::array<torch::Tensor, 3> state {};

        std::array<torch::Tensor, 3> nextEnv {};
        std::array<torch::Tensor, 3> nextState {};

        torch::Tensor actions {};
        torch::Tensor rewards {};
        torch::Tensor terminals {};

        Data() = default;
        ~Data() = default;
    };

    ReplayBuffer(const std::size_t& t_maxSize)
        : m_maxSize(t_maxSize) {};
    ~ReplayBuffer() = default;

    std::size_t size() noexcept;

    void add(const Data& t_data);

    Data get();

private:
    std::size_t m_maxSize {};
    std::vector<ReplayBuffer::Data> m_data {};
    std::size_t m_iter {};
};

#endif