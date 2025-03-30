#!/usr/bin/env bash

set -e

if [ "$1" == "-r" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release -B build
else
    cmake -DCMAKE_BUILD_TYPE=Debug -B build
fi

pkill -f "rap" || true

(
    cd build || exit 1
    cmake --build .
)
