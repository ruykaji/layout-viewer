#include "agent.hpp"

Agent::Agent()
{
    m_QPredictionModel = TRLM(7);
    m_QTargetModel = TRLM(7);

    updateTargetModel();

    m_QPredictionModel->train();
    m_QPredictionModel->to(torch::Device("cuda:0"));

    m_QTargetModel->eval();
    m_QTargetModel->to(torch::Device("cuda:0"));
}

torch::Tensor Agent::replayAction(torch::Tensor t_env, torch::Tensor t_state)
{
    return m_QPredictionModel->forward(t_env.to(torch::Device("cuda:0")), t_state.to(torch::Device("cuda:0"))).detach().to(torch::Device("cpu"));
}

std::pair<torch::Tensor, torch::Tensor> Agent::trainAction(const ReplayBuffer::Data& t_replay)
{
    auto w1 = m_QPredictionModel->forward(t_replay.env.to(torch::Device("cuda:0")), t_replay.state.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));
    auto w2 = m_QTargetModel->forward(t_replay.nextEnv.to(torch::Device("cuda:0")), t_replay.nextState.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));

    auto qPredicted = w1.gather(1, t_replay.actions);
    auto td = std::get<0>(w2.max(1, true));

    for (std::size_t i = 0; i < w2.size(0); ++i) {
        if (t_replay.rewards[i][0].item<double>() == 1.0) {
            td[i][0] = 0.0;
        }
    }

    auto qTarget = t_replay.rewards + 0.95 * td;

    return std::pair(qPredicted, qTarget);
}

std::vector<at::Tensor, std::allocator<at::Tensor>> Agent::getModelParameters()
{
    return m_QPredictionModel->parameters();
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

void Agent::eval()
{
    m_QPredictionModel->eval();
}

void Agent::train()
{
    m_QPredictionModel->train();
}