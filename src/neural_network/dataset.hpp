#ifndef __DATASET_H__
#define __DATASET_H__

#include <algorithm>
#include <numeric>
#include <random>

#include "data.hpp"

class TopologyDataset {
    std::vector<std::shared_ptr<Data>> m_data {};

    std::vector<std::shared_ptr<WorkingCell>> m_cells {};
    std::vector<std::vector<std::size_t>> m_batches {};
    std::size_t m_batchSize {};
    std::size_t m_iter {};

public:
    ~TopologyDataset() = default;

    TopologyDataset(const std::size_t& t_batchSize)
        : m_batchSize(t_batchSize) {};

    void add(const std::shared_ptr<Data>& t_data) noexcept;

    std::pair<torch::Tensor, torch::Tensor> get(const std::size_t& t_index) noexcept;

    std::size_t size() const noexcept;

private:
    void makeBatches() noexcept;
};

#endif