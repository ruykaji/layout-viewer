#ifndef __TRAIN_H__
#define __TRAIN_H__

#include <array>
#include <vector>

#include "neural_network/dataset.hpp"
#include "neural_network/model.hpp"

class ReplayBuffer {
public:
    struct Data {
        torch::Tensor env {};
        torch::Tensor state {};

        torch::Tensor nextEnv {};
        torch::Tensor nextState {};

        std::vector<int8_t> actions {};
        std::vector<double> rewards {};

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
    std::size_t m_iter { 0 };

    void randomOrder();
};

class Environment {
    torch::Tensor m_env {};
    torch::Tensor m_state {};

    std::vector<int8_t> m_terminals {};
    std::vector<int8_t> m_lastActions {};

public:
    Environment() = default;
    ~Environment() = default;

    void set(const torch::Tensor& t_env, const torch::Tensor& t_state);

    torch::Tensor getEnv();

    torch::Tensor getState();

    ReplayBuffer::Data replayStep(const torch::Tensor& t_actions, int32_t& t_steps);

    std::pair<double, int8_t> getRewardAndTerminal(const torch::Tensor& t_newState, const bool& t_isChangeDirection, const bool& t_isMoveAway);

private:
    int32_t selectRandomAction(const int32_t& t_actionSpace);

    int32_t selectBestAction(const torch::Tensor& t_predictions);

    int32_t epsilonGreedyAction(double t_epsilon, const torch::Tensor& t_predictions, const int32_t& t_actionSpace);
};

class Agent {
    TRLM m_QPredictionModel { nullptr };
    TRLM m_QTargetModel { nullptr };

public:
    Agent();
    ~Agent() = default;

    torch::Tensor replayAction(torch::Tensor t_env, torch::Tensor t_state);

    std::pair<torch::Tensor, torch::Tensor> trainAction(const ReplayBuffer::Data& t_replay);

    std::vector<at::Tensor, std::allocator<at::Tensor>> getModelParameters();

    void updateTargetModel();

private:
    void copyModelWeights(const TRLM& t_source, TRLM& t_target);
};

class Train {
public:
    Train() = default;
    ~Train() = default;

    void train(TrainTopologyDataset& t_trainDataset);
};

#endif