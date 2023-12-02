#ifndef __AGENT_H__
#define __AGENT_H__

#include <vector>

#include "neural_network/model.hpp"
#include "frame.hpp"

class Agent {
    DQN m_ActorCritics { nullptr };

public:
    Agent();
    ~Agent() = default;

    std::pair<torch::Tensor, torch::Tensor> action(std::vector<torch::Tensor> t_env, std::vector<torch::Tensor> t_state);

    torch::Tensor learn(const std::vector<Frame>& t_frames);

    std::vector<torch::Tensor> parameters();
};

#endif