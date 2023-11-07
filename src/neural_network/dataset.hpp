#ifndef __DATASET_H__
#define __DATASET_H__

#include "data.hpp"

class TopologyDataset {
    std::vector<std::shared_ptr<Data>> m_data;

public:
    TopologyDataset() = default;
    ~TopologyDataset() = default;

    void add(const std::shared_ptr<Data>& t_data)
    {
        m_data.emplace_back(t_data);
    }

    std::shared_ptr<Data> get(const std::size_t& t_index)
    {
        return m_data.at(t_index);
    }

    std::size_t size() const noexcept
    {
        return m_data.size();
    }
};

#endif