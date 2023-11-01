#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "encoder/encoder.hpp"
#include "pdk/convertor.hpp"

int main(int argc, char const* argv[])
{
    // Convertor convertor {};
    // convertor.serialize("/home/alaie/stuff/skywater-pdk/libraries/sky130_fd_sc_hd/latest", "./skyWater130.bin");

    auto start = std::chrono::high_resolution_clock::now();

    Encoder encoder("./skyWater130.bin");
    std::shared_ptr<Data> data = std::make_shared<Data>();

    encoder.readDef("/home/alaie/stuff/circuits/aes.def", data);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << std::setprecision(6);
    std::cout << "Time taken by reader: " << (duration.count() / 1000000.0) << " seconds\n";
    std::cout << "Num cells - " << data->numCellX * data->numCellY << "\n";
    std::cout << "Total Nets - " << data->totalNets << "\n";
    std::cout << "Total pins to connect - " << data->pins.size() << "\n";
    std::cout << "Total tensor memory in mbytes - " << (data->numCellX * data->numCellY * 9 * 480 * 480 * 4 * 1e-6) << "\n";
    std::cout << std::flush;

    return 0;
}