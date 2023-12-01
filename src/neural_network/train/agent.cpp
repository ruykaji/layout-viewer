#include <opencv2/opencv.hpp>

#include "agent.hpp"

inline static torch::Tensor sumWithDepth(torch::Tensor t_tensor)
{
    auto newTensor = torch::zeros({ 1, 4, 64, 64 });

    for (int i = 0; i < t_tensor.size(1); ++i) {
        int32_t n = t_tensor.size(1) - 1 - i;

        auto tensor = t_tensor[0][i] / std::pow(2, n - 1);

        cv::Mat matrix(cv::Size(tensor.size(-2), tensor.size(-1)), CV_32FC1, tensor.squeeze(0).data_ptr<float>());
        cv::resize(matrix, matrix, cv::Size(), 4, 4, cv::INTER_AREA);

        newTensor[0][i] = torch::from_blob(matrix.data, { matrix.rows, matrix.cols, 1 }, torch::kFloat32).squeeze();
    }

    newTensor = torch::sum(newTensor, 1, true);

    if (newTensor.max().item<double>() != 0.0) {
        newTensor /= newTensor.max();
    }

    return newTensor;
}

Agent::Agent()
{
    m_QPredictionModel = DQN(16, 4);
    m_QTargetModel = DQN(16, 4);

    updateTargetModel();

    m_QPredictionModel->train();
    m_QPredictionModel->to(torch::Device("cuda:0"));

    m_QTargetModel->eval();
    m_QTargetModel->to(torch::Device("cuda:0"));
}

torch::Tensor Agent::replayAction(std::vector<torch::Tensor> t_env, std::vector<torch::Tensor> t_state)
{
    torch::Tensor stakedEnv {};

    for (std::size_t i = 0; i < t_env.size(); ++i) {
        if (i == 0) {
            stakedEnv = sumWithDepth(t_env[i].clone());
        } else {
            stakedEnv = torch::cat({ stakedEnv, sumWithDepth(t_env[i].clone()) }, 1);
        }
    }

    torch::Tensor stakedState {};

    for (std::size_t i = 0; i < t_state.size(); ++i) {
        if (i == 0) {
            stakedState = t_state[i].clone();
        } else {
            stakedState = torch::cat({ stakedState, t_state[i] }, -1);
        }
    }

    return m_QPredictionModel->forward(stakedEnv.to(torch::Device("cuda:0")), stakedState.to(torch::Device("cuda:0")), true).detach().to(torch::Device("cpu"));
}

std::pair<torch::Tensor, torch::Tensor> Agent::trainAction(const ReplayBuffer::Data& t_replay)
{
    torch::Tensor stakedEnv {};

    for (std::size_t i = 0; i < t_replay.env.size(); ++i) {
        if (i == 0) {
            stakedEnv = sumWithDepth(t_replay.env[i].clone());
        } else {
            stakedEnv = torch::cat({ stakedEnv, sumWithDepth(t_replay.env[i].clone()) }, 1);
        }
    }

    torch::Tensor stakedState {};

    for (std::size_t i = 0; i < t_replay.state.size(); ++i) {
        if (i == 0) {
            stakedState = t_replay.state[i].clone();
        } else {
            stakedState = torch::cat({ stakedState, t_replay.state[i].clone() }, -1);
        }
    }

    auto w1 = m_QPredictionModel->forward(stakedEnv.to(torch::Device("cuda:0")), stakedState.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));

    torch::Tensor stakedNextEnv {};

    for (std::size_t i = 0; i < t_replay.nextEnv.size(); ++i) {
        if (i == 0) {
            stakedNextEnv = sumWithDepth(t_replay.nextEnv[i].clone());
        } else {
            stakedNextEnv = torch::cat({ stakedNextEnv, sumWithDepth(t_replay.nextEnv[i].clone()) }, 1);
        }
    }

    torch::Tensor stakedNextState {};

    for (std::size_t i = 0; i < t_replay.nextState.size(); ++i) {
        if (i == 0) {
            stakedNextState = t_replay.nextState[i].clone();
        } else {
            stakedNextState = torch::cat({ stakedNextState, t_replay.nextState[i].clone() }, -1);
        }
    }

    auto w2 = m_QTargetModel->forward(stakedNextEnv.to(torch::Device("cuda:0")), stakedNextState.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));
    auto w3 = m_QPredictionModel->forward(stakedNextEnv.to(torch::Device("cuda:0")), stakedNextState.to(torch::Device("cuda:0"))).to(torch::Device("cpu")).detach();

    auto qPredicted = w1.gather(1, t_replay.actions);
    auto qTarget = t_replay.rewards + 0.99 * w2.gather(1, w3.argmax(1, true)) * t_replay.terminals;

    return std::pair(qPredicted, qTarget);
}

std::vector<torch::Tensor> Agent::getModelParameters()
{
    return m_QPredictionModel->parameters(true);
};

void Agent::updateTargetModel()
{
    auto sourceParams = m_QPredictionModel->named_parameters(true);
    auto targetParams = m_QTargetModel->named_parameters(true);

    for (const auto& param : sourceParams) {
        auto& name = param.key();
        auto& sourceTensor = param.value();
        auto targetTensor = targetParams.find(name);

        if (targetTensor != nullptr) {
            targetTensor->requires_grad_(false);
            targetTensor->copy_(sourceTensor.detach());
        }
    }
};

void Agent::softUpdateTargetModel(const double& t_tau)
{
    auto sourceParams = m_QPredictionModel->named_parameters(true);
    auto targetParams = m_QTargetModel->named_parameters(true);

    for (const auto& param : sourceParams) {
        auto& name = param.key();
        auto& sourceTensor = param.value();
        auto targetTensor = targetParams.find(name);

        if (targetTensor != nullptr) {
            targetTensor->copy_(t_tau * sourceTensor.detach() + (1 - t_tau) * (*targetTensor));
        }
    }
};