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
    if (m_iter == m_batches.size() - 1 || m_iter == 0) {
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

    while (it != indexes.end()) {
        std::vector<std::size_t> part(it, it + m_batchSize);

        m_batches.emplace_back(part);
        it += m_batchSize;
    }
}