#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "encoder/encoder.hpp"
#include "pdk/convertor.hpp"

#include "neural_network/dataset.hpp"
#include "neural_network/train_module.hpp"

enum class WorkingMode {
    TRAIN = 0,
    MAKE_PDK,
};

struct Config {
    WorkingMode mode {};
};

int main(int argc, char const* argv[])
{
    Config config {};

    switch (config.mode) {
    case WorkingMode::MAKE_PDK: {
        Convertor::serialize("/home/alaie/stuff/skywater-pdk/libraries/sky130_fd_sc_hd/latest", "./skyWater130.bin");
        break;
    }
    case WorkingMode::TRAIN: {
        // Load pdk and create Encoder
        // ======================================================================================

        Encoder encoder;
        Convertor::deserialize("./skyWater130.bin", Encoder::s_pdk);

        Encoder::s_pdk.scale = static_cast<int32_t>(Encoder::s_pdk.scale * 1000.0);

        // Create(load) train and validation datasets
        // ======================================================================================

        std::vector<std::string_view> trainFiles = {
            {
                "/home/alaie/stuff/circuits/aes.def",
                // "/home/alaie/stuff/circuits/spm.def",
                // "/home/alaie/stuff/circuits/gcd.def",
            }
        };

        std::vector<std::string_view> validFiles = {
            {
                "/home/alaie/stuff/circuits/gcd.def",
                "/home/alaie/stuff/circuits/spm.def",
            }
        };

        TopologyDataset trainDataset {};

        for (auto& file : trainFiles) {
            std::shared_ptr<Data> data = std::make_shared<Data>();

            encoder.readDef(file, data);
            trainDataset.add(data);
        }

        TopologyDataset validDataset {};

        // for (auto& file : validFiles) {
        //     std::shared_ptr<Data> data = std::make_shared<Data>();

        //     encoder.readDef(file, data);
        //     validDataset.add(data);
        // }

        TrainModule trainModule(0);

        std::cout << std::setprecision(16);

        trainModule.train(trainDataset, validDataset, 100);

        // auto start = std::chrono::high_resolution_clock::now();

        // std::shared_ptr<Data> data = std::make_shared<Data>();

        // encoder.readDef("/home/alaie/stuff/circuits/aes.def", data);

        // auto stop = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        // std::cout << std::setprecision(12);
        // std::cout << "Time taken by reader: " << (duration.count() / 1000000.0) << " seconds\n";
        // std::cout << "Num cells - " << data->numCellX * data->numCellY << "\n";
        // std::cout << "Total Nets - " << data->totalNets << "\n";
        // std::cout << "Total pins to connect - " << data->correspondingToPinCells.size() << "\n";
        // std::cout << "Total tensor memory in mbytes - " << ((data->numCellX * data->numCellY / 1000000.0) * 22 * 480 * 480 * 4) << "\n";
        // std::cout << std::flush;

        break;
    }
    default:
        break;
    }

    return 0;
}