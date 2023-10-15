#include <chrono>
#include <iostream>

#include "encoder.hpp"

int main(int argc, char const* argv[])
{
    auto start = std::chrono::high_resolution_clock::now();

    Encoder encoder {};
    std::shared_ptr<Def> def = std::make_shared<Def>();

    encoder.readDef("/home/alaie/spm.def", def);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << "Time taken by reader: " << duration.count() << " microseconds" << std::endl;

    return 0;
}
