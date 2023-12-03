#ifndef __AGENT_H__
#define __AGENT_H__

#include <vector>

#include "frame.hpp"
#include "neural_network/model.hpp"

class Agent {
    ActorCritic m_ActorCritic { nullptr };

public:
    Agent();
    ~Agent() = default;

    std::pair<torch::Tensor, torch::Tensor> action(std::vector<torch::Tensor> t_env, std::vector<torch::Tensor> t_state);

    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> learn(const std::vector<Frame>& t_frames);

    std::vector<torch::Tensor> parameters();
};

#endif