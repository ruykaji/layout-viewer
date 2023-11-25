#include <random>

#include "evironment.hpp"

void Environment::set(const torch::Tensor& t_env, const torch::Tensor& t_state)
{
    m_env = t_env.clone();
    m_state = t_state.clone();

    m_terminals = std::vector<int8_t>(t_state.size(0), 0);
    m_lastActions = std::vector<int8_t>(t_state.size(0), 0);

    for (std::size_t i = 0; i < m_state.size(0); ++i) {
        m_env[0][m_state[i][2].item<int64_t>()][m_state[i][1].item<int64_t>()][m_state[i][0].item<int64_t>()] = 1.0;

        if (m_state[i][3].item<int64_t>() >= 0 && m_state[i][3].item<int64_t>() <= (m_env.size(-1) - 1) && m_state[i][4].item<int64_t>() >= 0 && m_state[i][4].item<int64_t>() <= (m_env.size(-1) - 1)) {
            m_env[0][m_state[i][5].item<int64_t>()][m_state[i][4].item<int64_t>()][m_state[i][3].item<int64_t>()] = 1.0;
        }
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

ReplayBuffer::Data Environment::replayStep(const torch::Tensor& t_actions)
{
    ReplayBuffer::Data replay {};

    replay.env = m_env.clone();
    replay.state = m_state.clone();
    replay.actions = torch::zeros({ m_state.size(0), 1 }, torch::kLong);
    replay.rewards = torch::zeros({ m_state.size(0), 1 });

    for (std::size_t i = 0; i < t_actions.size(0); ++i) {
        ++m_steps;

        double epsTrh = m_epsEnd + (m_epsStart - m_epsEnd) * std::exp(-1.0 * m_steps / m_totalSteps);
        int32_t action = epsilonGreedyAction(epsTrh, t_actions[i], 6);
        int32_t direction = action % 2 == 0 ? -1 : 1;
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
        bool isMovingAway = oldDistance < newDistance;

        auto pair = getRewardAndTerminal(newState, m_state[i], isMovingAway);

        if (pair.second == 0) {
            m_state[i] = newState;
        }

        replay.actions[i][0] = action;
        replay.rewards[i][0] = pair.first;

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

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState, const bool& t_isMovingAway)
{
    double reward = -0.1;

    // if (t_isMovingAway) {
    //     reward -= 0.1;
    // } else {
    //     reward += 0.1;
    // }

    // Checking for bounds collision
    if (t_newState[0].item<int32_t>() < 0 || t_newState[1].item<int32_t>() < 0 || t_newState[2].item<int32_t>() < 0 || t_newState[0].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() > (m_env.size(-1) - 1) || t_newState[2].item<int32_t>() > (m_env.size(-3) - 1)) {
        return std::pair(reward - 0.85, 1);
    }

    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() != 0.25) {
        reward -= 0.25;
    } else {
        reward += 0.25;
    }

    // Checking for track is out of cell and it reached border
    if (t_newState[-1].item<int64_t>() == 1 && (t_newState[0].item<int32_t>() == 0 || t_newState[1].item<int32_t>() == 0 || t_newState[0].item<int32_t>() == (m_env.size(-1) - 1) || t_newState[1].item<int32_t>() == (m_env.size(-1) - 1))) {
        m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()] = 0.75;
        return std::pair(1.0, 2);
    }

    // Checking for obstacles collision
    if (m_env[0][t_newState[2].item<int64_t>()][t_newState[1].item<int64_t>()][t_newState[0].item<int64_t>()].item<double>() == 0.5) {
        return std::pair(reward - 0.65, 1);
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
