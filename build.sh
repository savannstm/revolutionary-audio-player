#!/usr/bin/env bash

set -e

cmake -B build

pkill -f rap || true

cd build

if [ "$1" == "-r" ]; then
    cmake --build . --config Release
elif [ "$1" == "-m" ]; then
    cmake --build . --config MinSizeRel
else
    cmake --build . --config Debug
fi

cd ..
