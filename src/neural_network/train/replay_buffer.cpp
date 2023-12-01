#include <random>

#include "replay_buffer.hpp"

std::size_t ReplayBuffer::size() noexcept
{
    return m_data.size();
}

void ReplayBuffer::add(const ReplayBuffer::Data& t_data)
{
    if (m_data.size() + 1 > m_maxSize) {
        m_data.pop_front();
    };

    m_data.emplace_front(t_data);
}

ReplayBuffer::Data ReplayBuffer::get()
{
    static std::random_device rd;
    static std::mt19937 eng(rd());

    std::uniform_int_distribution<> distr(0, m_data.size() - 1);

    return m_data.front();
}