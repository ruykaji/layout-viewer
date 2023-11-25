#ifndef __AGENT_H__
#define __AGENT_H__

#include <vector>

#include "neural_network/model.hpp"
#include "replay_buffer.hpp"

class Agent {
    TRLM m_QPredictionModel { nullptr };
    TRLM m_QTargetModel { nullptr };

    std::size_t m_trainSteps {};

public:
    Agent();
    ~Agent() = default;

    torch::Tensor replayAction(torch::Tensor t_env, torch::Tensor t_state);

    std::pair<torch::Tensor, torch::Tensor> trainAction(const ReplayBuffer::Data& t_replay);

    std::vector<at::Tensor, std::allocator<at::Tensor>> getModelParameters();

    void updateTargetModel();

    void eval();

    void train();
};

#endif