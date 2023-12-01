#include <random>

#include "evironment.hpp"

void Environment::set(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    std::size_t frames = 4;

    m_env = std::vector<torch::Tensor>(frames - 1);

    for (std::size_t i = 0; i < frames - 1; ++i) {
        m_env[i] = torch::zeros(t_env.sizes());
    }

    m_env.emplace_back(t_env.clone());

    m_state = std::vector<torch::Tensor>(frames - 1);

    for (std::size_t i = 0; i < frames - 1; ++i) {
        m_state[i] = torch::zeros(t_state.sizes());
    }

    m_state.emplace_back(t_state.clone());

    m_totalActions = std::vector<int64_t>(t_state.size(0), 0);
    m_totalRewards = std::vector<double>(t_state.size(0), 0.0);

    for (std::size_t i = 0; i < m_state.back().size(0); ++i) {
        m_env.back()[0][m_state.back()[i][2].item<int64_t>()][m_state.back()[i][1].item<int64_t>()][m_state.back()[i][0].item<int64_t>()] = 1.0 / 2.0;
        m_env.back()[0][m_state.back()[i][5].item<int64_t>()][m_state.back()[i][4].item<int64_t>()][m_state.back()[i][3].item<int64_t>()] = 1.0 / 2.0;
    }
}

std::vector<torch::Tensor> Environment::getEnv()
{
    return m_env;
}

std::vector<torch::Tensor> Environment::getState()
{
    return m_state;
}

std::tuple<ReplayBuffer::Data, bool, bool> Environment::replayStep(const torch::Tensor& t_actions)
{
    ReplayBuffer::Data replay {};

    copyTensors(replay.env, m_env);
    copyTensors(replay.state, m_state);

    shift(m_env.back(), m_state.back());

    replay.actions = torch::zeros({ m_state.back().size(0), 1 }, torch::kLong);
    replay.rewards = torch::zeros({ m_state.back().size(0), 1 });
    replay.terminals = torch::zeros({ m_state.back().size(0), 1 });

    bool anyWin = false;

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        ++m_frames;

        double epsTrh = std::max(std::min(1.0, m_maxFrames / (m_frames + 1)), 0.01);
        int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 4);
        int32_t direction = action % 2 == 0 ? -1 : 1;
        torch::Tensor oldState = m_state.back()[i].clone();
        torch::Tensor newState = m_state.back()[i].clone();

        if (action == 0 || action == 1) {
            newState[0] += direction;
        } else if (action == 2 || action == 3) {
            newState[1] += direction;
        }
        // else if (action == 4 || action == 5) {
        //     newState[2] += direction;
        // }

        std::pair<double, int8_t> pair = getRewardAndTerminal(newState, oldState);

        m_totalActions[i] += 1;

        if (m_totalActions[i] < m_env.back().size(-1) * m_env.back().size(-1)) {
            if (pair.second != 1) {
                m_env.back()[0][oldState[2].item<int64_t>()][oldState[1].item<int64_t>()][oldState[0].item<int64_t>()] = 0.25;
                m_env.back()[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 0.5;

                m_state.back()[i] = newState.clone();
            }
        } else {
            if (pair.second != 2) {
                pair.second = 2;
                pair.first = -1.05;
            }
        }

        replay.terminals[i][0] = (pair.second >= 2) ? 0.0 : 1.0;
        replay.actions[i][0] = action;
        replay.rewards[i][0] = (pair.first - (-1.05)) / (0.95 - (-1.05));

        if (!anyWin) {
            anyWin = pair.second == 3;
        }
    }

    copyTensors(replay.nextEnv, m_env);
    copyTensors(replay.nextState, m_state);

    bool isDone = replay.terminals.mean().item<double>() == 0.0;

    return std::tuple(replay, isDone, anyWin);
}

double Environment::getExpToExpl()
{
    return (std::max(std::min(1.0, m_maxFrames / (m_frames + 1)), 0.01)) * 100.0;
};

void Environment::shift(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    std::vector<torch::Tensor> newEnv(m_env.size() - 1);

    for (std::size_t i = 0; i < newEnv.size(); ++i) {
        newEnv[i] = m_env[i + 1].clone();
    }

    newEnv.emplace_back(t_env.clone());
    m_env.clear();
    m_env = newEnv;

    std::vector<torch::Tensor> newState(m_state.size() - 1);

    for (std::size_t i = 0; i < newState.size(); ++i) {
        newState[i] = m_state[i + 1].clone();
    }

    newState.emplace_back(t_state.clone());
    m_state.clear();
    m_state = newState;
}

void Environment::copyTensors(std::vector<torch::Tensor>& t_lhs, const std::vector<torch::Tensor>& t_rhs)
{
    std::vector<torch::Tensor> newLhs(t_rhs.size());

    for (std::size_t i = 0; i < newLhs.size(); ++i) {
        newLhs[i] = t_rhs[i].clone();
    }

    t_lhs.clear();
    t_lhs = newLhs;
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

    if (dis(gen) < 1 - t_epsilon) {
        return selectBestAction(t_predictions);
    } else {
        return selectRandomAction(t_actionSpace);
    }
}

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState)
{
    double reward = 0;
    int32_t terminal = 0;

    reward -= 0.05;

    if (isInvalidState(t_newState)) {
        reward -= 0.5;
        terminal = 1;
        return std::pair(reward, terminal);
    }

    if (isGoalState(t_newState)) {
        reward += 1.0;
        terminal = 3;
        return std::pair(reward, terminal);
    }

    if (isStuck(t_newState, t_oldState)) {
        reward -= 1.0;
        terminal = 2;
        return std::pair(reward, terminal);
    }

    return std::pair(reward, terminal);
};

bool Environment::isInvalidState(const torch::Tensor& t_state)
{
    if (t_state[0].item<int32_t>() < 0) {
        return true;
    }

    if (t_state[1].item<int32_t>() < 0) {
        return true;
    }

    if (t_state[2].item<int32_t>() < 0) {
        return true;
    }

    if (t_state[0].item<int32_t>() > m_env.back().size(-1) - 1) {
        return true;
    }

    if (t_state[1].item<int32_t>() > m_env.back().size(-1) - 1) {
        return true;
    }

    if (t_state[2].item<int32_t>() > m_env.back().size(-3) - 1) {
        return true;
    }

    if (m_env.back()[0][t_state[2].item<int64_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.0) {
        return true;
    }

    if (m_env.back()[0][t_state[2].item<int64_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.25) {
        return true;
    }

    return false;
}

bool Environment::isGoalState(const torch::Tensor& t_state)
{
    if (t_state[2].item<int32_t>() != t_state[5].item<int32_t>()) {
        return false;
    }

    if (t_state[1].item<int32_t>() != t_state[4].item<int32_t>()) {
        return false;
    }

    if (t_state[0].item<int32_t>() != t_state[3].item<int32_t>()) {
        return false;
    }

    return true;
}

bool Environment::isStuck(const torch::Tensor& t_state, const torch::Tensor& t_oldState)
{
    bool left = true;
    bool right = true;
    bool backward = true;
    bool forward = true;
    bool up = true;
    bool down = true;

    if (t_state[0].item<int32_t>() == 0) {
        left = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>() - 1].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>() - 1].item<double>() == 0.25
        || t_state[0].item<int64_t>() - 1 == t_oldState[0].item<int64_t>()) {
        left = false;
    }

    if (t_state[1].item<int32_t>() == 0) {
        backward = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>() - 1][t_state[0].item<int64_t>()].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>() - 1][t_state[0].item<int64_t>()].item<double>() == 0.25
        || t_state[1].item<int64_t>() - 1 == t_oldState[1].item<int64_t>()) {
        backward = false;
    }

    if (t_state[2].item<int32_t>() == 0) {
        down = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>() - 1][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>() - 1][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.25
        || t_state[2].item<int64_t>() - 1 == t_oldState[2].item<int64_t>()) {
        down = false;
    }

    if (t_state[0].item<int32_t>() == m_env.back().size(-1) - 1) {
        right = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>() + 1].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>() + 1].item<double>() == 0.25
        || t_state[0].item<int64_t>() + 1 == t_oldState[0].item<int64_t>()) {
        right = false;
    }

    if (t_state[1].item<int32_t>() == m_env.back().size(-1) - 1) {
        forward = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>() + 1][t_state[0].item<int64_t>()].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>()][t_state[1].item<int64_t>() + 1][t_state[0].item<int64_t>()].item<double>() == 0.25
        || t_state[1].item<int64_t>() + 1 == t_oldState[1].item<int64_t>()) {
        forward = false;
    }

    if (t_state[2].item<int32_t>() == m_env.back().size(-3) - 1) {
        up = false;
    } else if (m_env.back()[0][t_state[2].item<int32_t>() + 1][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.0
        || m_env.back()[0][t_state[2].item<int32_t>() + 1][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.25
        || t_state[2].item<int64_t>() + 1 == t_oldState[2].item<int64_t>()) {
        up = false;
    }

    if (left || right || backward || forward || down || up) {
        return false;
    }

    return true;
}