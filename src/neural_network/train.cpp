#include "neural_network/train.hpp"

// ReplayBuffer methods
// ============================================================================================

std::size_t ReplayBuffer::size() noexcept
{
    return m_data.size();
}

void ReplayBuffer::add(const ReplayBuffer::Data& t_data)
{
    m_data.emplace_back(t_data);
}

ReplayBuffer::Data ReplayBuffer::get(const std::size_t& t_index)
{
    if (m_iter == m_order.size() || m_iter == 0) {
        randomOrder();
        m_iter = 0;
    }

    return m_data[m_order[m_iter++]];
}

void ReplayBuffer::randomOrder()
{
    std::vector<std::size_t> indexes(m_data.size());

    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    m_order = indexes;
}

// Environment methods
// ============================================================================================

void Environment::set(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    m_env = t_env.clone();
    m_state = t_state.clone();
}

torch::Tensor Environment::getEnv()
{
    return m_env;
}

torch::Tensor Environment::getState()
{
    return m_state;
}

ReplayBuffer::Data Environment::replayStep(const torch::Tensor& t_actions)
{
    ReplayBuffer::Data replay {};

    replay.env = m_env.detach().clone();
    replay.state = m_state.detach().clone();

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        int32_t action = epsilonGreedyAction(0.5, t_actions[i], 6);
        int32_t direction = action % 2 == 0 ? -1 : 1;
        double reward {};

        torch::Tensor newState = m_state[i].detach().clone();

        newState[action] += direction;
        reward -= 0.05;

        // Checking bounds
        if (newState[0].item<int32_t>() < 0 || newState[1].item<int32_t>() < 0 || newState[2].item<int32_t>() < 0 || newState[0].item<int32_t>() > (m_env.size(-1) - 1) || newState[1].item<int32_t>() > (m_env.size(-1) - 1) || newState[2].item<int32_t>() > (m_env.size(-3) - 1)) {
            reward -= 0.8;
        } else if (m_env[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()].item<double>() >= 0.0) {
            if (newState[2].item<int32_t>() == newState[5].item<int32_t>() && newState[1].item<int32_t>() == newState[4].item<int32_t>() && newState[0].item<int32_t>() == newState[3].item<int32_t>()) {
                reward = 1.0;
                m_state[i] = newState;
                m_env[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 1.0;
            } else if (m_env[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()].item<double>() != 1.0) {
                m_state[i] = newState;
                m_env[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 1.0;
            } else {
                reward -= 0.25;
            }
        } else {
            reward -= 0.75;
        }

        replay.actions.emplace_back(action);
        replay.rewards.emplace_back(reward);

        if (reward == 1.0) {
            replay.isTerminal.emplace_back(true);
        } else {
            replay.isTerminal.emplace_back(false);
        }
    }

    replay.nextEnv = m_env.detach().clone();
    replay.nextState = m_state.detach().clone();

    return replay;
}

int32_t Environment::selectRandomAction(const int32_t& t_actionSpace)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, t_actionSpace - 1);
    return dis(gen);
}

int32_t Environment::selectBestAction(const torch::Tensor& t_predictions)
{
    torch::Tensor max = t_predictions.argmax(0);
    return max.item<int32_t>();
}

int32_t Environment::epsilonGreedyAction(double t_epsilon, const torch::Tensor& t_predictions, const int32_t& t_actionSpace)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    if (dis(gen) < t_epsilon) {
        return selectRandomAction(t_actionSpace);
    } else {
        return selectBestAction(t_predictions);
    }
}

// Agent methods
// ============================================================================================

Agent::Agent(const TRLM& t_model)
{
    m_QPredictionModel = t_model;
    m_QTargetModel = TRLM(7);

    copyModelWeights(m_QPredictionModel, m_QTargetModel);

    m_QPredictionModel->train();
    m_QTargetModel->to(torch::Device("cuda:0"));
}

torch::Tensor Agent::replayAction(torch::Tensor t_env, torch::Tensor t_state)
{
    return torch::sigmoid(m_QPredictionModel->forward(t_env, t_state).detach());
}

std::pair<torch::Tensor, torch::Tensor> Agent::trainAction(const ReplayBuffer::Data& t_replay)
{
    auto w1 = torch::sigmoid(m_QPredictionModel->forward(t_replay.env, t_replay.state));
    auto w2 = torch::sigmoid(m_QTargetModel->forward(t_replay.nextEnv, t_replay.nextState));

    auto indexes = torch::zeros({ w1.size(0), 1 }).to(torch::device(torch::kCUDA)).toType(torch::kLong);

    for (std::size_t i = 0; i < w1.size(0); ++i) {
        indexes[i][0] = t_replay.actions[i];
    }

    auto qPredicted = w1.gather(1, indexes);

    auto rewards = torch::zeros({ w2.size(0), 1 }).to(torch::device(torch::kCUDA));

    for (std::size_t i = 0; i < w1.size(0); ++i) {
        rewards[i][0] = t_replay.rewards[i];
    }

    auto qTarget = rewards + std::get<0>(w2.max(1, true));

    return std::pair(qPredicted, qTarget);
}

void Agent::updateTargetModel()
{
    copyModelWeights(m_QPredictionModel, m_QTargetModel);
};

void Agent::copyModelWeights(const TRLM& t_source, TRLM& t_target)
{
    auto sourceParams = t_source->named_parameters(true);
    auto targetParams = t_target->named_parameters(true);

    for (const auto& param : sourceParams) {
        auto& name = param.key();
        auto& sourceTensor = param.value();
        auto targetTensor = targetParams.find(name);

        if (targetTensor != nullptr) {
            targetTensor->requires_grad_(false);
            targetTensor->copy_(sourceTensor.detach());
        }
    }
}

// Train methods
// ============================================================================================

Train::Train()
{
    m_model = TRLM(7);
    m_model->to(torch::Device("cuda:0"));
}

double Train::episodeStep(torch::Tensor t_env, torch::Tensor t_state)
{
    Environment environment {};
    Agent agent(m_model);

    torch::optim::AdamW optimizer(m_model->parameters(), 1e-4);

    for (std::size_t epoch = 0; epoch < 25; ++epoch) {
        std::size_t bufferSize = 50;
        ReplayBuffer replayBuffer {};
        double episodeLoss {};
        double episodeReward {};

        // Create replay buffer
        // ============================================================================

        environment.set(t_env.cuda(), t_state.cuda());

        for (std::size_t i = 0; i < bufferSize; ++i) {
            torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
            ReplayBuffer::Data replay = environment.replayStep(qValues);

            replayBuffer.add(replay);

            double replayReward {};

            for (auto& reward : replay.rewards) {
                replayReward += reward;
            }

            episodeReward += replayReward / replay.rewards.size();
        }

        // Train on random samples from replay buffer
        // ============================================================================

        for (std::size_t i = 0; i < bufferSize; ++i) {
            optimizer.zero_grad();

            ReplayBuffer::Data replay = replayBuffer.get(i);
            auto pair = agent.trainAction(replay);

            auto loss = torch::nn::functional::mse_loss(pair.first, pair.second);

            loss.backward();
            episodeLoss += loss.item<double>();

            optimizer.step();
        }

        agent.updateTargetModel();

        std::cout << "Episode epoch - " << epoch + 1 << " MSE Loss: " << episodeLoss / bufferSize << " Episode reward: " << episodeReward / bufferSize << std::endl;
    }

    return 0.0;
}