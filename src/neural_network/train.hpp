#ifndef __TRAIN_H__
#define __TRAIN_H__

#include <array>
#include <vector>

#include "dataset.hpp"
#include "model.hpp"

class ReplayBuffer {
public:
    struct Data {
        torch::Tensor env {};
        torch::Tensor state {};

        torch::Tensor nextEnv {};
        torch::Tensor nextState {};

        std::vector<int8_t> actions {};
        std::vector<double> rewards {};
        std::vector<bool> isTerminal {};

        Data() = default;
        ~Data() = default;
    };

    ReplayBuffer() = default;
    ~ReplayBuffer() = default;

    std::size_t size() noexcept;

    void add(const Data& t_data);

    Data get(const std::size_t& t_index);

private:
    std::vector<ReplayBuffer::Data> m_data {};
    std::vector<std::size_t> m_order {};
    std::size_t m_iter { 0 };

    void randomOrder();
};

class Environment {
    torch::Tensor m_env {};
    torch::Tensor m_state {};

public:
    Environment() = default;
    ~Environment() = default;

    void set(const torch::Tensor& t_env, const torch::Tensor& t_state);

    torch::Tensor getEnv();

    torch::Tensor getState();

    ReplayBuffer::Data replayStep(const torch::Tensor& t_actions);

private:
    int32_t selectRandomAction(const int32_t& t_actionSpace);

    int32_t selectBestAction(const torch::Tensor& t_predictions);

    int32_t epsilonGreedyAction(double t_epsilon, const torch::Tensor& t_predictions, const int32_t& t_actionSpace);
};

class Agent {
    TRLM m_QPredictionModel { nullptr };
    TRLM m_QTargetModel { nullptr };

public:
    Agent(const TRLM& t_model);
    ~Agent() = default;

    torch::Tensor replayAction(torch::Tensor t_env, torch::Tensor t_state);

    std::pair<torch::Tensor, torch::Tensor> trainAction(const ReplayBuffer::Data& t_replay);

    void updateTargetModel();

private:
    void copyModelWeights(const TRLM& t_source, TRLM& t_target);
};

class Train {
    TRLM m_model { nullptr };

public:
    Train();
    ~Train() = default;

    double episodeStep(torch::Tensor t_env, torch::Tensor t_state);
};

#endif