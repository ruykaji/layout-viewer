#ifndef __REPLAY_BUFFER_H__
#define __REPLAY_BUFFER_H__

#include <array>
#include <vector>

#include "torch_include.hpp"

class ReplayBuffer {
public:
    struct Data {
        std::vector<torch::Tensor> env {};
        std::vector<torch::Tensor> state {};

        std::vector<torch::Tensor> nextEnv {};
        std::vector<torch::Tensor> nextState {};

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
};

#endif