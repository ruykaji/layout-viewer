#ifndef __DATASET_H__
#define __DATASET_H__

#include <algorithm>
#include <numeric>
#include <random>

#include "data.hpp"

class TrainTopologyDataset {
    std::vector<std::pair<std::string, std::string>> m_cells {};
    std::vector<std::size_t> m_order {};
    std::size_t m_iter {};

public:
    TrainTopologyDataset();
    ~TrainTopologyDataset() = default;

    void add(const std::shared_ptr<Data>& t_data) noexcept;

    std::pair<torch::Tensor, torch::Tensor> get(const std::size_t& t_index) noexcept;

    std::size_t size() const noexcept;

private:
    void recursiveCollection(const std::string& t_path);
    void randomOrder() noexcept;
};

#endif