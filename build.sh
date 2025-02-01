#!/bin/bash

set -e
set -x

cd ./src/rusty-decoder
cargo b -r

mkdir ../../lib
mv ./target/release/librusty_decoder.so ../../lib/

cd ../../

cmake --build .

XDG_CURRENT_DESKTOP="KDE" ./target/bin/revolutionary-audio-player