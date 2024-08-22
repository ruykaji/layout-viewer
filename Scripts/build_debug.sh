#!/bin/bash
mkdir -p build/Debug
cd build/Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DEnableTests=OFF ../..
cmake --build .