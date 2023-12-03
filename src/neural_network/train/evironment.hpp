#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include <vector>

#include "frame.hpp"
#include "torch_include.hpp"

class Environment {
    std::vector<torch::Tensor> m_env {};
    std::vector<torch::Tensor> m_state {};

    std::vector<double> m_totalRewards {};

public:
    Environment() = default;
    ~Environment() = default;

    void set(const torch::Tensor& t_env, const torch::Tensor& t_state);

    std::vector<torch::Tensor> getEnv();

    std::vector<torch::Tensor> getState();

    std::tuple<Frame, bool, bool> step(torch::Tensor t_actionLogits);

private:
    torch::Tensor categoricalSample(const torch::Tensor& t_logits, const int32_t& t_numSamples = 1);

    void shift(const torch::Tensor& t_env, const torch::Tensor& t_state);

    std::pair<double, int8_t> getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState);

    bool isInvalidState(const torch::Tensor& t_state);

    bool isGoalState(const torch::Tensor& t_state);

    bool isStuck(const torch::Tensor& t_state, const torch::Tensor& t_oldState);
};

#endif