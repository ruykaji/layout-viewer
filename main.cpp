#include <cstdio>
#include <iostream>
#include <defrReader.hpp>

#include "def_encoder/def_encoder.hpp"

int main(int argc, char* argv[])
{
    auto fileName = std::string_view("/home/alaie/projects/layout-viewer/external/def/TEST/complete.5.8.def");

    Encoder encoder;

    encoder.initParser();
    encoder.setFile(fileName);
}