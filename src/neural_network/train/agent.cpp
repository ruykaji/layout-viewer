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
    m_ActorCritics = DQN(16, 4);

    m_ActorCritics->train();
    m_ActorCritics->to(torch::Device("cuda:0"));
}

std::pair<torch::Tensor, torch::Tensor> Agent::action(std::vector<torch::Tensor> t_env, std::vector<torch::Tensor> t_state)
{
    torch::Tensor stakedEnv {};

    for (std::size_t i = 0; i < t_env.size(); ++i) {
        if (i == 0) {
            stakedEnv = sumWithDepth(t_env[i].clone());
        } else {
            stakedEnv = torch::cat({ stakedEnv, sumWithDepth(t_env[i].clone()) }, 1);
        }
    }

    auto [actionLogits, values] = m_ActorCritics->forward(stakedEnv.to(torch::Device("cuda:0")), true);

    actionLogits = actionLogits.to(torch::Device("cpu"));
    values = values.to(torch::Device("cpu"));

    return std::make_pair(actionLogits, values);
}

torch::Tensor Agent::learn(const std::vector<Frame>& t_frames)
{
    torch::Tensor discountedSum = torch::tensor({ 0.0 });
    torch::Tensor returns = torch::zeros({ 1, static_cast<int32_t>(t_frames.size()) });
    torch::Tensor values = torch::zeros({ 1, static_cast<int32_t>(t_frames.size()) });
    torch::Tensor actionProbs = torch::zeros({ 1, static_cast<int32_t>(t_frames.size()) });

    for (int32_t i = t_frames.size() - 1; i > -1; --i) {
        discountedSum = t_frames[i].rewards[0] + 0.99 * discountedSum;
        returns[0][i] = discountedSum[0];

        values[0][i] = t_frames[i].values[0][0];
        actionProbs[0][i] = t_frames[i].actionProbs[0][0];
    }

    returns = (returns - returns.mean()) / (returns.std() + 1e-9);

    auto actorLoss = -(torch::log(actionProbs) * (returns - values)).mean();
    auto criticsLoss = torch::nn::functional::huber_loss(returns, values);

    return actorLoss + criticsLoss;
}

std::vector<torch::Tensor> Agent::parameters()
{
    return m_ActorCritics->parameters(true);
};