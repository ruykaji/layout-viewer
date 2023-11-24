#include <opencv2/opencv.hpp>

#include "neural_network/train.hpp"

static void displayProgressBar(const int32_t& t_current, const int32_t& t_total, const int32_t& t_barWidth, const std::string& t_title)
{
    float progress = float(t_current) / t_total;
    int pos = t_barWidth * progress;

    std::cout << t_title << " [";
    for (int i = 0; i < t_barWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << t_current << "/" << t_total << " \r";
    std::cout.flush();
}

static void createHeatMap(const torch::Tensor& t_tensor)
{
    auto tensor = t_tensor.detach().to(torch::kCPU);
    tensor = torch::sum(tensor.squeeze().to(torch::kFloat32), 0);
    tensor = (tensor / tensor.max().item<float>()) * 255.0;

    cv::Mat matrix(cv::Size(tensor.size(0), tensor.size(1)), CV_32FC1, tensor.data_ptr<float>());

    cv::Mat normalized;
    matrix.convertTo(normalized, CV_8UC1, 1.0);

    cv::Mat heatmap;
    cv::applyColorMap(normalized, heatmap, cv::COLORMAP_HOT);

    cv::imwrite("./images/heatmap.png", heatmap);
}

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

ReplayBuffer::Data ReplayBuffer::get()
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

    m_terminals = std::vector<int8_t>(t_state.size(0), 0);
    m_lastActions = std::vector<int8_t>(t_state.size(0), 0);

    for (std::size_t i = 0; i < m_state.size(0); ++i) {
        m_env[0][m_state[i][2].item<int64_t>()][m_state[i][1].item<int64_t>()][m_state[i][0].item<int64_t>()] = 1.0;
    }
}

torch::Tensor Environment::getEnv()
{
    return m_env;
}

torch::Tensor Environment::getState()
{
    return m_state;
}

ReplayBuffer::Data Environment::replayStep(const torch::Tensor& t_actions, int32_t& t_steps)
{
    ReplayBuffer::Data replay {};

    replay.env = m_env.clone();
    replay.state = m_state.clone();
    replay.actions = std::vector<int8_t>(m_state.size(0), 0);
    replay.rewards = std::vector<double>(m_state.size(0), 0.0);

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        ++t_steps;

        double epsTrh = 0.05 + (0.9 - 0.05) * std::exp(-1.0 * t_steps / 1e4);
        int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 6);
        int32_t direction = (action == 0 || action == 2 || action == 4) ? -1 : 1;
        bool isChangeDirection = action != m_lastActions[i] && m_lastActions[i] < 4;
        torch::Tensor newState = m_state[i].clone();

        if (action == 0 || action == 1) {
            newState[0] += direction;
        } else if (action == 2 || action == 3) {
            newState[1] += direction;
        } else if (action == 4 || action == 5) {
            newState[2] += direction;
        }

        double oldDistance = ((m_state[i][0] - m_state[i][1]).abs() + (m_state[i][1] - m_state[i][2]).abs() + (m_state[i][2] - m_state[i][3]).abs()).item<double>();
        double newDistance = ((newState[0] - newState[1]).abs() + (newState[1] - newState[2]).abs() + (newState[2] - newState[3]).abs()).item<double>();
        bool isMoveAway = oldDistance < newDistance;

        auto pair = getRewardAndTerminal(newState, isChangeDirection, isMoveAway);

        if (pair.second == 0) {
            m_state[i] = newState;
        }

        replay.actions[i] = action;
        replay.rewards[i] = pair.first;

        m_terminals[i] = pair.second;
        m_lastActions[i] = action;
    }

    replay.nextEnv = m_env.clone();
    replay.nextState = m_state.clone();

    // Remove track that done or failed
    std::size_t terminalsSize = m_terminals.size();

    for (std::size_t i = 0; i < terminalsSize; ++i) {
        if (m_terminals[i] == 2) {
            auto first_part = m_state.slice(0, 0, i);
            auto second_part = m_state.slice(0, i + 1);

            m_state = torch::cat({ first_part, second_part }, 0);

            m_terminals.erase(m_terminals.begin() + i);
            m_lastActions.erase(m_lastActions.begin() + i);
            --terminalsSize;
            --i;
        }
    }

    return replay;
}

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const bool& t_isChangeDirection, const bool& t_isMoveAway)
{
    double reward = -0.05;

    if (t_isChangeDirection) {
        reward -= 0.1;
    }

    if (t_isMoveAway) {
        reward -= 0.15;
    }

    // Checking for bounds collision
    if (t_newState[0].item<int32_t>() < 0 || t_newState[1].item<int32_t>() < 0 || t_newState[2].item<int32_t>() < 0 || t_newState[0].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[2].item<int32_t>() > (m_env.size(-3) - 1)) {
        return std::pair(reward - 0.7, 1);
    }

    // Checking for track is out of cell and it reached border
    if (t_newState[-1].item<int64_t>() == 1 && (t_newState[0].item<int32_t>() == 0 || t_newState[1].item<int32_t>() == 0 || t_newState[0].item<int32_t>() == (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() == (m_env.size(-1) - 1))) {
        m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 0.75;
        return std::pair(1.0, 2);
    }

    // Checking for obstacles collision
    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() == 0.5) {
        return std::pair(reward - 0.6, 1);
    }

    // Checking for end of track
    if (t_newState[2].item<int32_t>() == t_newState[5].item<int32_t>() && t_newState[1].item<int32_t>() == t_newState[4].item<int32_t>() && t_newState[0].item<int32_t>() == t_newState[3].item<int32_t>()) {
        m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 0.75;
        return std::pair(1.0, 2);
    }

    // Checking for collision between tracks
    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() == 0.75) {
        return std::pair(reward - 0.25, 1);
    }

    m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 0.75;
    return std::pair(reward, 0);
};

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

Agent::Agent()
{
    m_QPredictionModel = TRLM(7);
    m_QTargetModel = TRLM(7);

    copyModelWeights(m_QPredictionModel, m_QTargetModel);

    m_QPredictionModel->train();
    m_QPredictionModel->to(torch::Device("cuda:0"));

    m_QTargetModel->eval();
    m_QTargetModel->to(torch::Device("cuda:0"));
}

torch::Tensor Agent::replayAction(torch::Tensor t_env, torch::Tensor t_state)
{
    return m_QPredictionModel->forward(t_env, t_state).detach();
}

std::pair<torch::Tensor, torch::Tensor> Agent::trainAction(const ReplayBuffer::Data& t_replay)
{
    auto w1 = m_QPredictionModel->forward(t_replay.env, t_replay.state);
    auto w2 = m_QTargetModel->forward(t_replay.nextEnv, t_replay.nextState);

    auto indexes = torch::zeros({ w1.size(0), 1 }).to(torch::device(torch::kCUDA)).toType(torch::kLong);

    for (std::size_t i = 0; i < w1.size(0); ++i) {
        indexes[i][0] = t_replay.actions[i];
    }

    auto qPredicted = w1.gather(1, indexes);

    auto rewards = torch::zeros({ w2.size(0), 1 }).to(torch::device(torch::kCUDA));
    auto td = std::get<0>(w2.max(1, true));

    for (std::size_t i = 0; i < w2.size(0); ++i) {
        rewards[i][0] = t_replay.rewards[i];

        if (t_replay.rewards[i] == 1.0) {
            td[i][0] = 0.0;
        }
    }

    auto qTarget = rewards + 0.99 * td;

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
            targetTensor->copy_(sourceTensor.detach());
        }
    }
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

void Train::train(TrainTopologyDataset& t_trainDataset)
{
    std::size_t bufferSize = 600;
    std::size_t samples = 4;
    std::size_t batchSize = 12;

    Agent agent {};
    Environment environment {};
    torch::optim::AdamW optimizer(agent.getModelParameters(), torch::optim::AdamWOptions(1e-4));

    for (std::size_t epoch = 0; epoch < 100; ++epoch) {
        int32_t steps {};
        ReplayBuffer replayBuffer {};
        double epochReward {};
        double epochLoss {};

        for (std::size_t i = 0; i < samples; ++i) {
            std::pair<at::Tensor, at::Tensor> pair = t_trainDataset.get();

            environment.set(pair.first.cuda(), pair.second.cuda());

            for (std::size_t j = 0; j < bufferSize / samples; ++j) {
                if (environment.getState().size(0) <= 1) {
                    break;
                }

                torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
                ReplayBuffer::Data replay = environment.replayStep(qValues, steps);

                replayBuffer.add(replay);

                double replayReward {};

                for (auto& reward : replay.rewards) {
                    replayReward += reward;
                }

                epochReward += replayReward / replay.rewards.size();

                displayProgressBar(bufferSize / samples * i + j + 1, bufferSize, 50, "Epoch replay-buffer");
                createHeatMap(replay.env);
            }
        }

        std::cout << '\n'
                  << std::flush;

        for (std::size_t i = 0; i < replayBuffer.size() / batchSize; ++i) {
            optimizer.zero_grad();

            torch::Tensor loss {};

            for (std::size_t j = 0; j < batchSize; ++j) {
                ReplayBuffer::Data replay = replayBuffer.get();
                std::pair<at::Tensor, at::Tensor> pair = agent.trainAction(replay);

                if (loss.defined()) {
                    loss += torch::nn::functional::mse_loss(pair.first, pair.second);
                } else {
                    loss = torch::nn::functional::mse_loss(pair.first, pair.second);
                }

                displayProgressBar(replayBuffer.size() / batchSize * i + j + 1, replayBuffer.size(), 50, "Epoch training: ");
            }

            loss.backward();
            optimizer.step();

            epochLoss += loss.item<double>();
        }

        agent.updateTargetModel();

        std::cout << "\n\n"
                  << "Epoch - " << epoch + 1 << '\n'
                  << "MSE Loss: " << epochLoss / replayBuffer.size() << '\n'
                  << "Mean Reward: " << epochReward / replayBuffer.size() << "\n\n"
                  << std::flush;
    }
}