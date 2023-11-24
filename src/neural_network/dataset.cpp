#include <filesystem>

#include "dataset.hpp"

TrainTopologyDataset::TrainTopologyDataset()
{
    recursiveCollection("./cache");
    randomOrder();
}

std::pair<torch::Tensor, torch::Tensor> TrainTopologyDataset::get() noexcept
{
    if (m_iter == m_cells.size()) {
        randomOrder();
        m_iter = 0;
    }

    torch::Tensor source {};
    torch::Tensor connections {};

    torch::load(source, m_cells[m_order[m_iter]].first);
    torch::load(connections, m_cells[m_order[m_iter]].second);

    source = source.unsqueeze(0);

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

    m_order = indexes;
}