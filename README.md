# revolutionary-audio-player/renaissance

<p align="center">
  <img src="./icons/rap-logo.png" alt="Description" width="256"/>
</p>

The Revolutionary Audio Player, abbreviated RAP (which initially was named Rusty Audio Player and developed using gtk4-rs) and codenamed Renaissance is a **NEW GENERATION** :fire: _minimalistic_ and **_FULL FEATURED_** :trollface: audio player.

![Interface](./screenshots/gui.png)

## Features

-   [x] `mp3`, `flac`, `wav`, `ogg`, `opus`, `m4a`, `mp4`, `mkv`, `aac`, `alac`, `mov`, `mka` audio default support.
-   [x] `png`, `jpeg`, `bmp`, `webp` cover image support.
-   [x] Native look and the same capabilities on **any** platform.
-   [x] Interface, that keeps it simple.
-   [x] Completely portable. Leaves **no** traces in OS by default!
-   [x] Default tray icon support, close button only hides the window.
    -   [x] Volume and progress sliders built **right inside** the tray icon menu!
-   [x] Built-in ready-to-use 3-, 5-, 10-, 18- and 30-band equalizers with presets.
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

Can't compile this thing for Linux, keeps complaining about missing definitions in static FFmpeg build.

Windows build is compiled statically - you can get in Releases section, just a single .exe and a couple of icon/translation files, no .dlls, folders and other bloat.

## Build

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`.

The project uses CMake as build system.

You'll need `Qt6`, `JUCE`, `FFmpeg`, `CImg` and `rapidhash` to build the project.

Default builds of the program include FFmpeg built with the following configuration: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --enable-avformat --enable-avcodec --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska --enable-zlib --enable-protocol=file`

Path to the `rapidhash` headers can be passed as `-DRAPIDHASH_INCLUDE_DIRS`.

For MinGW compatibility, you'll have to use `7.0.12` (but it has issues when building with C++23 standard) version of JUCE framework, as `8.0.0` deprecated MinGW support.

`build.ps1` PowerShell script and `build.sh` Bash script are used to build the project to `build` directory.

Script supports `-r` argument for building in `Release` mode.

Build artifacts are output to `build/target` directory.
