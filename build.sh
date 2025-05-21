#!/usr/bin/env bash

set -e

show_help() {
    cat << EOF
Usage: ./build.sh [options]

Options:
    -c                      Configure only (no build)
    -r                      Build in Release configuration
    -m                      Build in MinSizeRel configuration
    --cflags <flags>        Specify flags when running 'cmake .' command
    --bflags <flags>        Specify flags when running 'cmake --build .' command
    --help                  Show this help message
EOF
}

build_config="Debug"
configure_only=false
cflags=""
bflags=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        -c)
            configure_only=true
            shift
            ;;
        -r)
            build_config="Release"
            shift
            ;;
        -m)
            build_config="MinSizeRel"
            shift
            ;;
        --cflags)
            if [[ -z "$2" ]]; then
                echo "Error: --cflags requires a value"
                exit 1
            fi
            cflags="$2"
            shift 2
            ;;
        --bflags)
            if [[ -z "$2" ]]; then
                echo "Error: --bflags requires a value"
                exit 1
            fi
            bflags="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1"
            show_help
            exit 1
            ;;
    esac
done

if pgrep rap >/dev/null 2>&1; then
    pkill -9 rap || true
fi

cmake_args="-B build"
if [[ -n "$cflags" ]]; then
    cmake_args="$cmake_args $cflags"
fi

cmake $cmake_args

if $configure_only; then
    exit 0
fi

cd build

build_args="--build . --config $build_config -j"
if [[ -n "$bflags" ]]; then
    build_args="$build_args $bflags"
fi

cmake $build_args

cd ..
