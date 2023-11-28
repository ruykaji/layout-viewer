#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include <vector>

#include "replay_buffer.hpp"
#include "torch_include.hpp"

class Environment {
    std::array<torch::Tensor, 3> m_env {};
    std::array<torch::Tensor, 3> m_state {};

    std::vector<int64_t> m_totalActions {};
    std::vector<int8_t> m_terminals {};
    std::vector<int8_t> m_lastActions {};
    std::vector<double> m_totalLength {};

    double m_rewardDiscount { 0.99 };
    double m_epsStart { 1.0 };
    double m_epsEnd { 0.1 };
    double m_totalSteps { 2e3 };
    double m_steps {};

public:
    Environment() = default;
    ~Environment() = default;

    void set(const torch::Tensor& t_env, const torch::Tensor& t_state);

    std::array<torch::Tensor, 3> getEnv();

    std::array<torch::Tensor, 3> getState();

    std::pair<ReplayBuffer::Data, bool> replayStep(const torch::Tensor& t_actions);

    double getExpToExpl();

private:
    void shift(const torch::Tensor& t_env, const torch::Tensor& t_state);

    void copyTensors(std::array<torch::Tensor, 3>& t_lhs, const std::array<torch::Tensor, 3>& t_rhs);

    int32_t selectRandomAction(const int32_t& t_actionSpace);

    int32_t selectBestAction(const torch::Tensor& t_predictions);

    int32_t epsilonGreedyAction(double t_epsilon, const torch::Tensor& t_predictions, const int32_t& t_actionSpace);

    std::pair<double, int8_t> getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState, const double& t_totalLength);

    bool isOutOfBounds(const torch::Tensor& t_state);

    bool isInvalidState(const torch::Tensor& t_state);

    double isMovingToTarget(const torch::Tensor& t_newState, const torch::Tensor& t_oldState);

    bool isGoalState(const torch::Tensor& t_state);

    bool isStuck(const torch::Tensor& t_state, const double& t_lLength);
};

#endif