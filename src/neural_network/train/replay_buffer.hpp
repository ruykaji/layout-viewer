#ifndef __REPLAY_BUFFER_H__
#define __REPLAY_BUFFER_H__

#include <vector>

#include "torch_include.hpp"

class ReplayBuffer {
public:
    struct Data {
        torch::Tensor env {};
        torch::Tensor state {};

        torch::Tensor nextEnv {};
        torch::Tensor nextState {};

        torch::Tensor actions {};
        torch::Tensor rewards {};

        Data() = default;
        ~Data() = default;
    };

    ReplayBuffer() = default;
    ~ReplayBuffer() = default;

    std::size_t size() noexcept;

    void add(const Data& t_data);

    Data get();

private:
    std::vector<ReplayBuffer::Data> m_data {};
    std::vector<std::size_t> m_order {};
    std::size_t m_iter {};

    void randomOrder();
};

#endif