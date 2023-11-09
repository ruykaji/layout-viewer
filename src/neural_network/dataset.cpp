#include "dataset.hpp"

void TopologyDataset::add(const std::shared_ptr<Data>& t_data) noexcept
{
    for (std::size_t y = 0; y < t_data->numCellY; ++y) {
        for (std::size_t x = 0; x < t_data->numCellX; ++x) {
            m_cells.emplace_back(t_data->cells[y][x]);
        }
    }
}

std::pair<torch::Tensor, torch::Tensor> TopologyDataset::get(const std::size_t& t_index) noexcept
{
    if (m_iter == m_batches.size() || m_iter == 0) {
        makeBatches();
        m_iter = 0;
    }

    torch::Tensor soursBatch {};
    torch::Tensor targetBatch {};

    for (const auto& index : m_batches[m_iter]) {
        if (soursBatch.defined()) {
            soursBatch = torch::cat({ soursBatch, m_cells[index]->source }, 0);
            targetBatch = torch::cat({ targetBatch, m_cells[index]->target }, 0);
        } else {
            soursBatch = m_cells[index]->source;
            targetBatch = m_cells[index]->target;
        }
    }

    ++m_iter;

    return std::pair(soursBatch, targetBatch);
}

std::size_t TopologyDataset::size() const noexcept
{
    return std::ceil(static_cast<double>(m_cells.size()) / m_batchSize);
}

void TopologyDataset::makeBatches() noexcept
{
    std::vector<std::size_t> indexes(m_cells.size());

    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    auto it = indexes.begin();
    std::size_t _size = size();
    std::size_t cellsSize = m_cells.size();

    for (std::size_t i = 0; i < size(); ++i) {
        if (i == _size - 1 && cellsSize % m_batchSize != 0) {
            std::vector<std::size_t> part(it, it + (cellsSize % m_batchSize));
            m_batches.emplace_back(part);
        } else {
            std::vector<std::size_t> part(it, it + m_batchSize);
            m_batches.emplace_back(part);
            it +=m_batchSize;
        }
    }
}