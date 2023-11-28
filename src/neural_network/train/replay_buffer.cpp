#include <random>

#include "replay_buffer.hpp"

std::size_t ReplayBuffer::size() noexcept
{
    return m_data.size();
}

void ReplayBuffer::add(const ReplayBuffer::Data& t_data)
{
    if (m_data.size() + 1 > m_maxSize) {
        m_data.erase(m_data.begin());
    };

    m_data.emplace_back(t_data);

    std::shuffle(m_data.begin(), m_data.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));
}

ReplayBuffer::Data ReplayBuffer::get()
{
    if (m_iter == m_data.size()) {
        m_iter = 0;
    }

    return m_data[m_iter++];
}