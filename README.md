# revolutionary-audio-player/renaissance

The Revolutionary Audio Player, abbreviated RAP (which initially was named Rusty Audio Player and developed using gtk4-rs) and codenamed Renaissance is a **NEW GENERATION** :fire: _lightweight_ and **_FULL FEATURED_** :trollface: audio player.

## Why a new audio player?

There's many audio players already out there, big amount of which is either filled with unnecessary things, or just legacy, like the extremely cool Windows `foobar2000` audio player.

This audio player emphasizes clean codebase, little amount of dependencies, simple interface, cross-platform distribution, and being easy overall.

## Releases

Unfortunately, I don't have the patience to compile 50gs of libraries for a Linux build.

But I did so for Windows. And statically.

Program is a single `.exe` that's linked statically - no `.dll`s, folders, shit and other annoying things!

## Build

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`.

The project uses CMake as build system.

You'll need `Qt6`, `JUCE`, `FFmpeg`, and `rapidhash` to build the project.

Path to the `rapidhash` headers can be passed as `-DRAPIDHASH_INCLUDE_DIRS`.

For MinGW compatibility, you'll have to use `7.0.12` version of JUCE framework, as `8.0.0` deprecated MinGW support.

`build.ps1` PowerShell script and `build.sh` Bash script are used to build the project to `build` directory.

Script supports `-r` argument for building in `Release` mode.

Build artifacts are output to `build/target` directory.
