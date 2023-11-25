#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include <vector>

#include "replay_buffer.hpp"
#include "torch_include.hpp"

class Environment {
    torch::Tensor m_env {};
    torch::Tensor m_state {};

    std::vector<int8_t> m_terminals {};
    std::vector<int8_t> m_lastActions {};

    double m_epsStart { 0.9 };
    double m_epsEnd { 0.05 };
    double m_totalSteps { 5e5 };
    double m_steps {};

public:
    Environment() = default;
    ~Environment() = default;

    void set(const torch::Tensor& t_env, const torch::Tensor& t_state);

    torch::Tensor getEnv();

    torch::Tensor getState();

    ReplayBuffer::Data replayStep(const torch::Tensor& t_actions);

    std::pair<double, int8_t> getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState, const bool& t_isMovingAway);

private:
    int32_t selectRandomAction(const int32_t& t_actionSpace);

    int32_t selectBestAction(const torch::Tensor& t_predictions);

    int32_t epsilonGreedyAction(double t_epsilon, const torch::Tensor& t_predictions, const int32_t& t_actionSpace);
};

#endif