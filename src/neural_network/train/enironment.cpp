#include <random>

#include "evironment.hpp"

void Environment::set(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    m_env[0] = torch::zeros(t_env.sizes());
    m_env[1] = torch::zeros(t_env.sizes());
    m_env[2] = t_env.clone();

    m_state[0] = torch::zeros(t_state.sizes());
    m_state[1] = torch::zeros(t_state.sizes());
    m_state[2] = t_state.clone();

    m_totalActions = std::vector<int64_t>(t_state.size(0), 0);
    m_terminals = std::vector<int8_t>(t_state.size(0), 0);
    m_lastActions = std::vector<int8_t>(t_state.size(0), 0);
    m_totalLength = std::vector<double>(t_state.size(0), 1.0);

    for (std::size_t i = 0; i < m_state.back().size(0); ++i) {
        // m_env.back()[0][m_state.back()[i][2].item<int64_t>()][m_state.back()[i][1].item<int64_t>()][m_state.back()[i][0].item<int64_t>()] = 0.75;
        // m_env.back()[0][m_state.back()[i][5].item<int64_t>()][m_state.back()[i][4].item<int64_t>()][m_state.back()[i][3].item<int64_t>()] = 1.0;
        m_env.back()[0][0][m_state.back()[i][1].item<int64_t>()][m_state.back()[i][0].item<int64_t>()] = 0.35;
        m_env.back()[0][0][m_state.back()[i][3].item<int64_t>()][m_state.back()[i][2].item<int64_t>()] = 1.0;
    }
}

std::array<torch::Tensor, 3> Environment::getEnv()
{
    return m_env;
}

std::array<torch::Tensor, 3> Environment::getState()
{
    return m_state;
}

std::pair<ReplayBuffer::Data, bool> Environment::replayStep(const torch::Tensor& t_actions)
{
    ReplayBuffer::Data replay {};

    copyTensors(replay.env, m_env);
    copyTensors(replay.state, m_state);

    shift(m_env.back(), m_state.back());

    replay.actions = torch::zeros({ m_state.back().size(0), 1 }, torch::kLong);
    replay.rewards = torch::zeros({ m_state.back().size(0), 1 });
    replay.terminals = torch::zeros({ m_state.back().size(0), 1 });

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        ++m_steps;

        double epsTrh = m_epsEnd + (m_epsStart - m_epsEnd) * std::exp(-1.0 * m_steps / m_totalSteps);
        // int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 6);
        int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 4);
        int32_t direction = action % 2 == 0 ? -1 : 1;
        torch::Tensor newState = m_state[2][i].clone();

        if (action == 0 || action == 1) {
            newState[0] += direction;
        } else if (action == 2 || action == 3) {
            newState[1] += direction;
        }
        // else if (action == 4 || action == 5) {
        //     newState[2] += direction;
        // }

        auto pair = getRewardAndTerminal(newState, m_state.back()[i], m_totalLength[i]);

        replay.terminals[i][0] = (pair.second == 2) ? 0.0 : 1.0;
        replay.actions[i][0] = action;
        replay.rewards[i][0] += std::pow(m_rewardDiscount, m_totalActions[i]) * pair.first;

        m_totalActions[i] += 1;

        if (pair.second != 0) {
            m_totalLength[i] += 1.0;
        } else {
            m_totalLength[i] = 0;
        }

        if (pair.second == 0) {
            m_state.back()[i] = newState.clone();
            // m_env.back()[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 0.75;
            m_env.back()[0][0][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 0.35;
        }
    }

    copyTensors(replay.nextEnv, m_env);
    copyTensors(replay.nextState, m_state);

    bool isDone = replay.terminals.mean().item<double>() == 0.0;

    return std::pair(replay, isDone);
}

double Environment::getExpToExpl()
{
    return (m_epsEnd + (m_epsStart - m_epsEnd) * std::exp(-1.0 * m_steps / m_totalSteps)) * 100.0;
};

void Environment::shift(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    std::array<torch::Tensor, 3> newEnv {};

    newEnv[0] = m_env[1].clone();
    newEnv[1] = m_env[2].clone();
    newEnv[2] = t_env.clone();
    m_env = newEnv;

    std::array<torch::Tensor, 3> newState {};

    newState[0] = m_state[1].clone();
    newState[1] = m_state[2].clone();
    newState[2] = t_state.clone();
    m_state = newState;
}

void Environment::copyTensors(std::array<torch::Tensor, 3>& t_lhs, const std::array<torch::Tensor, 3>& t_rhs)
{
    std::array<torch::Tensor, 3> newLhs {};

    newLhs[0] = t_rhs[0].clone();
    newLhs[1] = t_rhs[1].clone();
    newLhs[2] = t_rhs[2].clone();
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

    if (dis(gen) < t_epsilon) {
        return selectRandomAction(t_actionSpace);
    } else {
        return selectBestAction(t_predictions);
    }
}

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState, const double& t_totalLength)
{
    double reward = 0;
    int32_t terminal = 0;

    if (isOutOfBounds(t_newState)) {
        reward = -0.75;
        terminal = 1;
        return std::pair(reward, terminal);
    }

    if (isGoalState(t_newState)) {
        reward = 1.0;
        terminal = 2;
        return std::pair(reward, terminal);
    }

    if (isInvalidState(t_newState)) {
        if (isStuck(t_newState, t_totalLength)) {
            reward = -1.0;
            terminal = 1;
            return std::pair(reward, terminal);
        }

        reward = -0.25;
        terminal = 1;
        return std::pair(reward, terminal);
    } else {
        reward -= 0.8 - 0.75 * std::exp(-M_PI * isMovingToTarget(t_newState, t_oldState));
        return std::pair(reward, terminal);
    }
};

bool Environment::isOutOfBounds(const torch::Tensor& t_state)
{
    if (t_state[0].item<int32_t>() < 0) {
        return true;
    }

    if (t_state[1].item<int32_t>() < 0) {
        return true;
    }

    // if (t_state[2].item<int32_t>() < 0) {
    //     return true;
    // }

    if (t_state[0].item<int32_t>() > m_env.back().size(-1) - 1) {
        return true;
    }

    if (t_state[1].item<int32_t>() > m_env.back().size(-1) - 1) {
        return true;
    }

    // if (t_state[2].item<int32_t>() > m_env.back().size(-3) - 1) {
    //     return true;
    // }

    if (m_env.back()[0][0][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.0) {
        return true;
    }

    return false;
}

bool Environment::isInvalidState(const torch::Tensor& t_state)
{
    // if (m_env.back()[0][t_state[2].item<int64_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.25) {
    //     return true;
    // }

    // if (m_env.back()[0][t_state[2].item<int64_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.75) {
    //     return true;
    // }

    // if (m_env.back()[0][t_state[2].item<int64_t>()][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 1.0 && !isGoalState(t_state)) {
    //     return true;
    // }

    // if (m_env.back()[0][0][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() == 0.0) {
    //     return true;
    // }

    if (m_env.back()[0][0][t_state[1].item<int64_t>()][t_state[0].item<int64_t>()].item<double>() > 0.25) {
        return true;
    }

    return false;
}

double Environment::isMovingToTarget(const torch::Tensor& t_newState, const torch::Tensor& t_oldState)
{
    // double newXPow = std::pow(t_newState[0].item<int64_t>() - t_newState[3].item<int64_t>(), 2.0);
    // double newYPow = std::pow(t_newState[1].item<int64_t>() - t_newState[4].item<int64_t>(), 2.0);

    // double oldXPow = std::pow(t_oldState[0].item<int64_t>() - t_oldState[3].item<int64_t>(), 2.0);
    // double oldYPow = std::pow(t_oldState[1].item<int64_t>() - t_oldState[4].item<int64_t>(), 2.0);

    double newXPow = std::pow(t_newState[0].item<int64_t>() - t_newState[2].item<int64_t>(), 2.0);
    double newYPow = std::pow(t_newState[1].item<int64_t>() - t_newState[3].item<int64_t>(), 2.0);

    double oldXPow = std::pow(t_oldState[0].item<int64_t>() - t_oldState[2].item<int64_t>(), 2.0);
    double oldYPow = std::pow(t_oldState[1].item<int64_t>() - t_oldState[3].item<int64_t>(), 2.0);

    return std::abs((newXPow + newYPow) - (oldXPow + oldYPow));
}

bool Environment::isGoalState(const torch::Tensor& t_state)
{
    // if (t_state[2].item<int32_t>() != t_state[5].item<int32_t>()) {
    //     return false;
    // }

    // if (t_state[1].item<int32_t>() != t_state[4].item<int32_t>()) {
    //     return false;
    // }

    // if (t_state[0].item<int32_t>() != t_state[3].item<int32_t>()) {
    //     return false;
    // }

    if (t_state[1].item<int32_t>() != t_state[3].item<int32_t>()) {
        return false;
    }

    if (t_state[0].item<int32_t>() != t_state[2].item<int32_t>()) {
        return false;
    }

    return true;
}

bool Environment::isStuck(const torch::Tensor& t_state, const double& t_length)
{
    if (t_length > 50) {
        return true;
    }

    return false;
}