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

ReplayBuffer::Data Environment::replayStep(const torch::Tensor& t_actions, const int32_t& t_steps)
{
    ReplayBuffer::Data replay {};

    replay.env = m_env.clone();
    replay.state = m_state.clone();
    replay.actions = std::vector<int8_t>(m_state.size(0), 0);
    replay.rewards = std::vector<double>(m_state.size(0), 0.0);

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        double epsTrh = 0.05 + (0.9 - 0.05) * std::exp(-1.0 * t_steps / 1000);
        int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 4);
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

        auto pair = getRewardAndTerminal(newState, isChangeDirection);

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
        if (m_terminals[i] != 0) {
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

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const bool& t_isChangeDirection)
{
    double reward = -0.05;

    if (t_isChangeDirection) {
        reward -= 0.1;
    }

    // Checking for bounds collision
    if (t_newState[0].item<int32_t>() < 0 || t_newState[1].item<int32_t>() < 0 || t_newState[2].item<int32_t>() < 0 || t_newState[0].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[2].item<int32_t>() > (m_env.size(-3) - 1)) {
        return std::pair(reward - 0.8, 1);
    }

    // Checking for track is out of cell and it reached border
    if (t_newState[-1].item<int64_t>() == 1 && (t_newState[0].item<int32_t>() == 0 || t_newState[1].item<int32_t>() == 0 || t_newState[0].item<int32_t>() == (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() == (m_env.size(-1) - 1))) {
        m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 1.0;
        return std::pair(reward + 1.0, 2);
    }

    // Checking for obstacles collision
    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() == 0.5) {
        return std::pair(reward - 0.7, 1);
    }

    // Checking for end of track
    if (t_newState[2].item<int32_t>() == t_newState[5].item<int32_t>() && t_newState[1].item<int32_t>() == t_newState[4].item<int32_t>() && t_newState[0].item<int32_t>() == t_newState[3].item<int32_t>()) {
        m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 1.0;
        return std::pair(reward + 1.0, 2);
    }

    // Checking for collision between tracks
    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() == 1.0) {
        return std::pair(reward - 0.25, 1);
    }

    m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 1.0;
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

    for (std::size_t i = 0; i < w1.size(0); ++i) {
        rewards[i][0] = t_replay.rewards[i];
    }

    auto qTarget = rewards + 0.95 * std::get<0>(w2.max(1, true));

    return std::pair(qPredicted, qTarget);
}

std::vector<at::Tensor, std::allocator<at::Tensor>> Agent::getModelParameters()
{
    return m_QPredictionModel->parameters();
};

void Agent::softUpdateTargetModel(const double& t_tau)
{
    auto source_parameters = m_QPredictionModel->named_parameters(true);
    auto target_parameters = m_QTargetModel->named_parameters(true);

    for (const auto& source_param : source_parameters) {
        auto name = source_param.key();
        auto& target_param = target_parameters[name];

        target_param.data().mul_(1.0 - t_tau);
        target_param.data().add_(source_param.value().data(), t_tau);
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

double Train::episodeStep(torch::Tensor t_env, torch::Tensor t_state)
{
    Environment environment {};
    Agent agent {};

    torch::optim::AdamW optimizer(agent.getModelParameters(), torch::optim::AdamWOptions(1e-4));

    for (std::size_t epoch = 0; epoch < 1000; ++epoch) {
        std::size_t bufferSize = 1000;
        ReplayBuffer replayBuffer {};
        double episodeLoss {};
        double episodeReward {};

        // Create replay buffer
        // ============================================================================

        environment.set(t_env.cuda(), t_state.cuda());

        for (std::size_t i = 0; i < bufferSize; ++i) {
            if (environment.getState().size(0) > 1) {
                torch::Tensor qValues = agent.replayAction(environment.getEnv(), environment.getState());
                ReplayBuffer::Data replay = environment.replayStep(qValues, i + 1);

                replayBuffer.add(replay);

                double replayReward {};

                for (auto& reward : replay.rewards) {
                    replayReward += reward;
                }

                episodeReward += replayReward / replay.rewards.size();

                displayProgressBar(i + 1, bufferSize, 50, "Making replay buffer");
                createHeatMap(replay.env);
            } else {
                break;
            }
        }

        std::cout << '\n'
                  << std::flush;

        // Train on random samples from replay buffer
        // ============================================================================

        for (std::size_t i = 0; i < replayBuffer.size(); ++i) {
            optimizer.zero_grad();

            ReplayBuffer::Data replay = replayBuffer.get(i);
            auto pair = agent.trainAction(replay);

            auto loss = torch::nn::functional::huber_loss(pair.first, pair.second);

            loss.backward();
            episodeLoss += loss.item<double>();

            optimizer.step();
            agent.softUpdateTargetModel(0.005);

            displayProgressBar(i + 1, bufferSize, 50, "Episode training: ");
        }

        std::cout << "\n\n"
                  << std::flush;

        std::cout << "Episode epoch - " << epoch + 1 << '\n'
                  << "MSE Loss: " << episodeLoss / bufferSize << '\n'
                  << "Episode reward: " << episodeReward / bufferSize << "\n\n\n"
                  << std::flush;
    }

    return 0.0;
}