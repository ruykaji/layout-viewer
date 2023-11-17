#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "config.hpp"

#include "encoder/encoder.hpp"
#include "pdk/convertor.hpp"

#include "neural_network/dataset.hpp"
#include "neural_network/encoders/resnet50.hpp"
#include "neural_network/train_module.hpp"

int main(int argc, char const* argv[])
{

    Config config(argc, argv);

    switch (config.getMode()) {
    case Config::Mode::MAKE_PDK: {
        Convertor::serialize("/home/alaie/stuff/skywater-pdk/libraries/sky130_fd_sc_hd/latest", "./skyWater130.bin");
        break;
    }
    case Config::Mode::TRAIN: {
        // Load pdk and create Encoder
        // ======================================================================================

        PDK pdk;
        Encoder encoder;
        Convertor::deserialize("./skyWater130.bin", pdk);

        // Create(load) train and validation datasets
        // ======================================================================================

        std::vector<std::string_view> trainFiles = {
            {
                "/home/alaie/stuff/circuits/picorv32.def",
                "/home/alaie/stuff/circuits/mem_1r1w.def",
                "/home/alaie/stuff/circuits/APU.def",
                "/home/alaie/stuff/circuits/PPU.def",
                "/home/alaie/stuff/circuits/aes.def",
                "/home/alaie/stuff/circuits/spm.def",
                "/home/alaie/stuff/circuits/gcd.def",
            }
        };

        // TopologyDataset trainDataset(8);

        for (auto& file : trainFiles) {
            std::cout << file << std::endl;

            std::shared_ptr<Data> data = std::make_shared<Data>();

            encoder.readDef(file, data, pdk, config);
            // trainDataset.add(data);
        }

        // std::vector<std::string_view> validFiles = {
        //     {
        //         "/home/alaie/stuff/circuits/gcd.def",
        //         "/home/alaie/stuff/circuits/spm.def",
        //     }
        // };

        // TopologyDataset validDataset {};

        // for (auto& file : validFiles) {
        //     std::shared_ptr<Data> data = std::make_shared<Data>();

        //     encoder.readDef(file, data);
        //     validDataset.add(data);
        // }

        // TrainModule trainModule(0);

        // std::cout << std::setprecision(16);

        // trainModule.train(trainDataset, 1000);

        break;
    }
    case Config::Mode::TEST: {
        auto start = std::chrono::high_resolution_clock::now();

        PDK pdk;
        Encoder encoder;
        Convertor::deserialize("./skyWater130.bin", pdk);

        std::shared_ptr<Data> data = std::make_shared<Data>();

        encoder.readDef("/home/alaie/stuff/circuits/picorv32.def", data, pdk, config);

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << std::setprecision(12);
        std::cout << "Time taken by reader: " << (duration.count() / 1000000.0) << " seconds\n";
        std::cout << "Num cells - " << data->numCellX * data->numCellY << "\n";
        std::cout << "Total Nets - " << data->totalNets << "\n";
        std::cout << "Total pins to connect - " << data->correspondingToPinCells.size() << "\n";
        std::cout << "Total tensor memory in mbytes - " << ((data->numCellX * data->numCellY / 1000000.0) * 3 * config.getCellSize() * config.getCellSize() * 5) << "\n";
        std::cout << std::flush;

        break;
    }
    default:
        break;
    }

    return 0;
}