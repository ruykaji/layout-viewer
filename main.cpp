#include <cstdio>
#include <iostream>

#include <defrReader.hpp>

int main(int argc, char* argv[])
{
    auto isInit = defrInit();

    if (isInit) {
        std::cerr << "Error: cant't initialize reader!\n"
                  << std::flush;
        return 2;
    }

    auto file = fopen("/home/alaie/projects/layout-viewer/__tests__/complete.5.8.def", "r");

    if (file != nullptr) {
        auto isRead = defrRead(file, "/home/alaie/projects/layout-viewer/layout-viewer/complete.5.8.def", nullptr, 0);

        if (isRead) {
            std::cerr << "Error: could not read the file!\n"
                      << std::flush;
            return 2;
        }
    }

    fclose(file);
}