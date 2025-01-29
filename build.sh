#!/bin/bash

set -e
set -x

cmake --build .

XDG_CURRENT_DESKTOP="KDE" ./target/bin/revolutionary-audio-player
