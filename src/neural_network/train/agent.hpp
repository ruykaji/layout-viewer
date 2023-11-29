#ifndef __AGENT_H__
#define __AGENT_H__

#include <vector>

#include "neural_network/model.hpp"
#include "replay_buffer.hpp"

class Agent {
    DQN m_QPredictionModel { nullptr };
    DQN m_QTargetModel { nullptr };

    std::size_t m_trainSteps {};

public:
    Agent();
    ~Agent() = default;

    torch::Tensor replayAction(std::vector<torch::Tensor> t_env, std::vector<torch::Tensor> t_state);

    std::pair<torch::Tensor, torch::Tensor> trainAction(const ReplayBuffer::Data& t_replay);

    std::vector<torch::Tensor>  getModelParameters();

    void updateTargetModel();

    void softUpdateTargetModel(const double& t_tau);
};

#endif