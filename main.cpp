#include <iostream>
#include <cstdio>

#include <def/defrReader.hpp>

int main(int argc, char* argv[])
{
    auto isInit = defrInit();

    if(isInit){
        std::cerr << "Error: cant't initialize reader!\n" << std::flush;
        return 0;
    }

    auto file = fopen("/home/alaie/lefdef/def/TEST/complete.5.8.def", "r");
    auto isReaded = defrRead(file, "/home/alaie/lefdef/def/TEST/complete.5.8.def", nullptr, 0);
    auto data = defrGetUserData();

    fclose(file);

    if(isReaded){
        std::cerr << "Error: could not read the file!\n" << std::flush;
        return 0;
    }
}