#include <filesystem>

#include "dataset.hpp"

TopologyDataset::TopologyDataset(const std::size_t& t_batchSize)
    : m_batchSize(t_batchSize)
{
    if (std::filesystem::exists("./cache") && std::filesystem::is_directory("./cache")) {
        for (const auto& entry : std::filesystem::directory_iterator("./cache")) {
            if (std::filesystem::is_regular_file(entry) && entry.path().filename().string().find("source") == 0) {
                ++m_totalCells;
            }
        }
    } else {
        std::cerr << "The specified path is not a valid directory." << std::endl;
    }
}

void TopologyDataset::add(const std::shared_ptr<Data>& t_data) noexcept
{
    for (std::size_t y = 0; y < t_data->numCellY; ++y) {
        for (std::size_t x = 0; x < t_data->numCellX; ++x) {

            torch::Tensor newSource = torch::zeros({ 1, 0, t_data->cells[y][x]->source.size(2), t_data->cells[y][x]->source.size(3) });

            for (std::size_t i = 0; i < t_data->cells[y][x]->source.size(1); ++i) {
                newSource = torch::cat({ newSource, t_data->cells[y][x]->source[0][i].unsqueeze(0).repeat({ 3, 1, 1 }).unsqueeze(0) }, 1);
            }

            torch::Tensor newTarget = torch::zeros({ 1, 0, t_data->cells[y][x]->target.size(2), t_data->cells[y][x]->target.size(3) });

            for (std::size_t i = 0; i <  t_data->cells[y][x]->target.size(1); ++i) {
                newTarget = torch::cat({ newTarget,  t_data->cells[y][x]->target[0][i].unsqueeze(0).repeat({ 7, 1, 1 }).unsqueeze(0) }, 1);
            }

            newSource = newSource.index({ torch::indexing::Slice(), torch::indexing::Slice(0, 32), torch::indexing::Slice(), torch::indexing::Slice() });
            newTarget = newTarget.index({ torch::indexing::Slice(), torch::indexing::Slice(0, 32), torch::indexing::Slice(), torch::indexing::Slice() });

            torch::save(newSource, "./cache/source_" + std::to_string(m_totalCells) + ".pt");
            torch::save(newTarget, "./cache/target_" + std::to_string(m_totalCells) + ".pt");

            // auto pickledSource = torch::pickle_save(t_data->cells[y][x]->source);
            // std::ofstream foutSource("./cache/source_" + std::to_string(m_totalCells) + ".pt", std::ios::out | std::ios::binary);
            // foutSource.write(pickledSource.data(), pickledSource.size());
            // foutSource.close();

            // auto pickledTarget = torch::pickle_save(t_data->cells[y][x]->target);
            // std::ofstream foutTarget("./cache/target_" + std::to_string(m_totalCells) + ".pt", std::ios::out | std::ios::binary);
            // foutTarget.write(pickledTarget.data(), pickledTarget.size());
            // foutTarget.close();

            ++m_totalCells;
        }
    }
}

std::pair<torch::Tensor, torch::Tensor> TopologyDataset::get(const std::size_t& t_index) noexcept
{
    if (m_iter == m_batches.size() || m_iter == 0) {
        makeBatches();
        m_iter = 0;
    }

    torch::Tensor source {};
    torch::Tensor target {};
    torch::Tensor soursBatch {};
    torch::Tensor targetBatch {};

    for (const auto& index : m_batches[m_iter]) {
        torch::load(source, "./cache/source_" + std::to_string(index) + ".pt");
        torch::load(target, "./cache/target_" + std::to_string(index) + ".pt");

        if (soursBatch.defined()) {
            soursBatch = torch::cat({ soursBatch, source.unsqueeze(0) }, 0);
            targetBatch = torch::cat({ targetBatch, target.unsqueeze(0) }, 0);
        } else {
            soursBatch = source.unsqueeze(0);
            targetBatch = target.unsqueeze(0);
        }
    }

    ++m_iter;

    return std::pair(soursBatch, targetBatch);
}

std::size_t TopologyDataset::size() const noexcept
{
    return std::ceil(static_cast<double>(m_totalCells) / m_batchSize);
}

void TopologyDataset::makeBatches() noexcept
{
    std::vector<std::size_t> indexes(m_totalCells);

    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    auto it = indexes.begin();
    std::size_t _size = size();

    for (std::size_t i = 0; i < size(); ++i) {
        if (i == _size - 1 && m_totalCells % m_batchSize != 0) {
            std::vector<std::size_t> part(it, it + (m_totalCells % m_batchSize));
            m_batches.emplace_back(part);
        } else {
            std::vector<std::size_t> part(it, it + m_batchSize);
            m_batches.emplace_back(part);
            it += m_batchSize;
        }
    }
}