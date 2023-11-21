#include <filesystem>

#include "dataset.hpp"

TrainTopologyDataset::TrainTopologyDataset()
{
    recursiveCollection("./cache");
    randomOrder();
}

std::pair<torch::Tensor, torch::Tensor> TrainTopologyDataset::get(const std::size_t& t_index) noexcept
{
    if (m_iter == m_cells.size()) {
        m_iter = 0;
    }

    torch::Tensor source {};
    torch::Tensor connections {};

    for (const auto& index : m_order[m_iter]) {
        torch::load(source, m_cells[index].first);
        torch::load(connections, m_cells[index].second);

        source = source.unsqueeze(0);
    }

    ++m_iter;

    return std::pair(source, connections);
}

std::size_t TrainTopologyDataset::size() const noexcept
{
    return m_cells.size();
}

void TrainTopologyDataset::recursiveCollection(const std::string& t_path)
{
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(t_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();

                if (filename.rfind("source", 0) == 0) {
                    m_cells.push_back(std::pair(entry.path().string(), (entry.path().parent_path() / ("connections" + filename.substr(6))).string()));
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error while searching files: " << e.what() << std::endl;
    }
};

void TrainTopologyDataset::randomOrder() noexcept
{
    std::vector<std::size_t> indexes(m_cells.size());

    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    auto it = indexes.begin();

    for (std::size_t i = 0; i < size(); ++i) {
        std::vector<std::size_t> part(it, it + 1);
        m_order.emplace_back(part);
        it += 1;
    }
}