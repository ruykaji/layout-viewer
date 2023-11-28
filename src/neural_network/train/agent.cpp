#include "agent.hpp"

Agent::Agent()
{
    m_QPredictionModel = DQN(12, 4);
    m_QTargetModel = DQN(12, 4);

    updateTargetModel();

    m_QPredictionModel->train();
    m_QPredictionModel->to(torch::Device("cuda:0"));

    m_QTargetModel->eval();
    m_QTargetModel->to(torch::Device("cuda:0"));
}

torch::Tensor Agent::replayAction(std::array<torch::Tensor, 3> t_env, std::array<torch::Tensor, 3> t_state)
{
    auto stakedEnv = torch::cat({ torch::sum(t_env[0], 1, true), torch::sum(t_env[1], 1, true), torch::sum(t_env[2], 1, true) }, 1);
    auto stakedState = torch::cat({ t_state[0], t_state[1], t_state[2] }, 1);

    return m_QPredictionModel->forward(stakedEnv.to(torch::Device("cuda:0")), stakedState.to(torch::Device("cuda:0")), true).detach().to(torch::Device("cpu"));
}

std::pair<torch::Tensor, torch::Tensor> Agent::trainAction(const ReplayBuffer::Data& t_replay)
{
    auto stakedEnv = torch::cat({ torch::sum(t_replay.env[0], 1, true), torch::sum(t_replay.env[1], 1, true), torch::sum(t_replay.env[2], 1, true) }, 1);
    auto stakedState = torch::cat({ t_replay.state[0], t_replay.state[1], t_replay.state[2] }, 1);
    auto w1 = m_QPredictionModel->forward(stakedEnv.to(torch::Device("cuda:0")), stakedState.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));

    auto stakedNextEnv = torch::cat({ torch::sum(t_replay.nextEnv[0], 1, true), torch::sum(t_replay.nextEnv[1], 1, true), torch::sum(t_replay.nextEnv[2], 1, true) }, 1);
    auto stakedNextState = torch::cat({ t_replay.nextState[0], t_replay.nextState[1], t_replay.nextState[2] }, 1);
    auto w2 = m_QTargetModel->forward(stakedNextEnv.to(torch::Device("cuda:0")), stakedNextState.to(torch::Device("cuda:0"))).to(torch::Device("cpu"));

    auto qPredicted = w1.gather(1, t_replay.actions);
    auto td = std::get<0>(w2.max(1, true)) * t_replay.terminals;

    auto qTarget = t_replay.rewards + 0.99 * td;

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

void Agent::eval()
{
    m_QPredictionModel->eval();
}

void Agent::train()
{
    m_QPredictionModel->train();
}