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

std::tuple<Frame, bool, bool> Environment::step(const torch::Tensor& t_actionLogits)
{
    Frame frame {};
    bool anyWin = false;

    shift(m_env.back(), m_state.back());

    frame.actionProbs = torch::zeros({ m_state.back().size(0), 1 });
    frame.rewards = torch::zeros({ m_state.back().size(0), 1 });
    frame.terminals = torch::zeros({ m_state.back().size(0), 1 });

    for (std::size_t i = 0; i < t_actionLogits.size(0); ++i) {
        int32_t action = categoricalSample(t_actionLogits[i].detach().unsqueeze(0), 1).item<int32_t>();

        torch::Tensor oldState = m_state.back()[i].clone();
        torch::Tensor newState = m_state.back()[i].clone();

        if (action == 0 || action == 1) {
            newState[0] += action % 2 == 0 ? -1 : 1;
        } else if (action == 2 || action == 3) {
            newState[1] += action % 2 == 0 ? -1 : 1;
        } else if (action == 4 || action == 5) {
            newState[2] += action % 2 == 0 ? -1 : 1;
        }

        std::pair<double, int8_t> rewardTerminalPair = getRewardAndTerminal(newState, oldState);

        m_totalActions[i] += 1;

        if (m_totalActions[i] < 300) {
            if (rewardTerminalPair.second == 0 || rewardTerminalPair.second == 3) {
                m_env.back()[0][oldState[2].item<int64_t>()][oldState[1].item<int64_t>()][oldState[0].item<int64_t>()] = 0.25;
                m_env.back()[0][newState[2].item<int64_t>()][newState[1].item<int64_t>()][newState[0].item<int64_t>()] = 0.5;

                m_state.back()[i] = newState.clone();
            }
        } else {
            if (rewardTerminalPair.second == 0 || rewardTerminalPair.second == 1) {
                rewardTerminalPair.first -= 10.0;
                rewardTerminalPair.second = 2;
            }
        }

        frame.actionProbs[i][0] = torch::softmax(t_actionLogits[i], 0)[action];
        frame.rewards[i][0] = rewardTerminalPair.first;
        frame.terminals[i][0] = (rewardTerminalPair.second >= 2) ? 0.0 : 1.0;

        if (!anyWin) {
            anyWin = rewardTerminalPair.second == 3;
        }
    }

    bool isDone = frame.terminals.mean().item<double>() == 0.0;

    return std::tuple(frame, isDone, anyWin);
}

torch::Tensor Environment::categoricalSample(const torch::Tensor& t_logits, const int32_t& t_numSamples)
{
    torch::Tensor probabilities = torch::softmax(t_logits, -1);
    torch::Tensor cumulativeDistribution = probabilities.cumsum(-1);
    torch::Tensor randomNumbers = torch::rand({ t_numSamples, t_logits.size(0) });
    torch::Tensor samples = (cumulativeDistribution.unsqueeze(0) > randomNumbers.unsqueeze(-1)).to(torch::kInt64).argmax(-1);

    return samples;
}

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

std::pair<double, int8_t> Environment::getRewardAndTerminal(const torch::Tensor& t_newState, const torch::Tensor& t_oldState)
{
    double reward = 0;
    int32_t terminal = 0;

    reward -= 0.05;

    if (isInvalidState(t_newState)) {
        reward -= 0.75;
        terminal = 1;
        return std::pair(reward, terminal);
    }

    if (isGoalState(t_newState)) {
        reward += 10.0;
        terminal = 3;
        return std::pair(reward, terminal);
    }

    if (isStuck(t_newState, t_oldState)) {
        reward -= 10.0;
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