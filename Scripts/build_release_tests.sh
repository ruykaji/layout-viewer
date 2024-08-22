#!/bin/bash
mkdir -p build/Release
cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release -DEnableTests=ON ../..
cmake --build .