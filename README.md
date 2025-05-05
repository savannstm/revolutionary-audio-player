# revolutionary-audio-player/renaissance

<p align="center">
  <img src="./icons/rap-logo.png" alt="Description" width="256"/>
</p>

The Revolutionary Audio Player, abbreviated RAP (which initially was named Rusty Audio Player and developed using gtk4-rs) and codenamed Renaissance is a **NEW GENERATION** :fire: _minimalistic_ and **_FULL FEATURED_** :trollface: audio player.

![Interface](./screenshots/gui.png)

## Features

-   [x] `.mp3`, `.flac`, `.wav`, `.ogg`, `.opus`, `.m4a`, `.mp4`, `.mkv`, `.aac` support (other formats support with custom FFmpeg build).
-   [x] Native look and same capabilities on **any** platform.
-   [x] Interface, that keeps it simple.
-   [x] Completely portable. Leaves **no** traces in OS!
-   [x] Default tray icon support, close button only hides the window.
    -   [x] Volume and progress sliders built **right inside** the tray icon menu!
-   [x] **partially** Built-in ready-to-use 3-, 5-, 10-, 18- and 30-band equalizers.
    -   [ ] On top of that, you can change the **frequencies** of equalizer themselves.
-   [x] Playlist background images.
-   [x] Flexible playlist system, `.xspf` and `.m3u8` import/export.
-   [x] Playlist searching.
-   [x] Dock window with metadata.
-   [ ] `.cue` playing.
-   [ ] Wave visualizer.

## Why a new audio player?

There's many audio players already out there, big amount of which is either filled with unnecessary things, or just legacy, like the extremely cool Windows `foobar2000` audio player.

This audio player emphasizes clean codebase, little amount of dependencies, simple interface, cross-platform distribution, and being easy overall.

## Releases

Compile this shit for Linux yourself if you want. I'll probably start to make releases at 1.0.0 version.

But I did so for Windows. And statically.

Program is a single `.exe` that's linked statically - no `.dll`s, folders, shit and other annoying things!

## Build

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`.

The project uses CMake as build system.

You'll need `Qt6`, `JUCE`, `FFmpeg`, `CImg` and `rapidhash` to build the project.

Path to the `rapidhash` headers can be passed as `-DRAPIDHASH_INCLUDE_DIRS`.

For MinGW compatibility, you'll have to use `7.0.12` version of JUCE framework, as `8.0.0` deprecated MinGW support.

`build.ps1` PowerShell script and `build.sh` Bash script are used to build the project to `build` directory.

Script supports `-r` argument for building in `Release` mode.

Build artifacts are output to `build/target` directory.
