#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "config.hpp"

#include "encoder/encoder.hpp"
#include "pdk/convertor.hpp"

#include "neural_network/dataset.hpp"
#include "neural_network/train/train.hpp"

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

        std::shared_ptr<PDK> pdkPtr = std::make_shared<PDK>(pdk);
        std::shared_ptr<Config> configPtr = std::make_shared<Config>(config);

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

        // for (auto& file : trainFiles) {
        //     std::shared_ptr<Data> dataPtr = std::make_shared<Data>();

        //     encoder.readDef(file, dataPtr, pdkPtr, configPtr);
        // }

        TrainTopologyDataset trainDataset {};
        Train train {};

        train.train(trainDataset);

        break;
    }
    case Config::Mode::TEST: {
        auto start = std::chrono::high_resolution_clock::now();

        PDK pdk;
        Encoder encoder;
        Convertor::deserialize("./skyWater130.bin", pdk);

        std::shared_ptr<Data> dataPtr = std::make_shared<Data>();
        std::shared_ptr<PDK> pdkPtr = std::make_shared<PDK>(pdk);
        std::shared_ptr<Config> configPtr = std::make_shared<Config>(config);

        encoder.readDef("/home/alaie/stuff/circuits/spm.def", dataPtr, pdkPtr, configPtr);

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << std::setprecision(12);
        std::cout << "Time taken by reader: " << (duration.count() / 1000000.0) << " seconds\n";
        std::cout << "Num cells - " << dataPtr->numCellX * dataPtr->numCellY << "\n";
        std::cout << "Total Nets - " << dataPtr->totalNets << "\n";
        std::cout << "Total pins to connect - " << dataPtr->correspondingToPinCell.size() << "\n";
        std::cout << "Total tensor memory in mbytes - " << ((dataPtr->numCellX * dataPtr->numCellY / 1000000.0) * 3 * config.getCellSize() * config.getCellSize() * 5) << "\n";
        std::cout << std::flush;

        break;
    }
    default:
        break;
    }

    return 0;
}