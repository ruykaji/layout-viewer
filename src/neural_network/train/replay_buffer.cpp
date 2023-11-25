#include <random>

#include "replay_buffer.hpp"

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
