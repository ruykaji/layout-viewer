#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "encoder/encoder.hpp"
#include "pdk/convertor.hpp"

int main(int argc, char const* argv[])
{
    // Convertor convertor {};

    // convertor.serialize("/home/alaie/stuff/skywater-pdk", "./skyWater130.bin");

    auto start = std::chrono::high_resolution_clock::now();

    Encoder encoder {};
    std::shared_ptr<Data> data = std::make_shared<Data>();

    encoder.readDef("/home/alaie/spm.def", "./skyWater130.bin", data);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << std::setprecision(6);
    std::cout << "Time taken by reader: " << (duration.count() / 1000000.0) << " seconds\n";
    std::cout << "Num cells - " << data->numCellX * data->numCellY << "\n";
    std::cout << "Total Nets - " << data->totalNets << "\n";
    std::cout << "Total pins to connect - " << data->pins.size() << "\n";
    std::cout << std::flush;

    return 0;
}
