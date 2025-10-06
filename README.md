# revolutionary-audio-player/renaissance

[README на русском](./README-ru.md)

<p align="center">
    <img src="./icons/rap-logo.png" alt="Description" width="256"/>
</p>

The Revolutionary Audio Player, abbreviated RAP (which initially was named Rusty Audio Player and developed using gtk4-rs) and codenamed Renaissance is a **NEW GENERATION** :fire: _minimalistic_ and **_FULL FEATURED_** :trollface: audio player.

![Interface](./screenshots/gui.png)

## Features

-   [x] `mp3`, `flac`, `wav`, `ogg`, `opus`, `m4a`, `mp4`, `mkv`, `aac`, `alac`, `mov`, `mka`, `ac3` audio default support.
-   [x] `png`, `jpeg`, `bmp`, `webp`, `tiff` cover image support.
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
-   [x] Wave visualizer.
-   [x] `.cue` playing.
-   [x] You can select a **range to repeat**, when repeat mode is set to repeat a single track!

## Why a new audio player?

There's many audio players already out there, big amount of which is either filled with unnecessary things, or just legacy, like the extremely cool Windows `foobar2000` audio player.

This audio player emphasizes clean codebase, little amount of dependencies, simple interface, cross-platform distribution, and being easy overall.

## Releases

Linux builds are compiled statically on Alpine Linux with musl - they require **NO** system libraries.

Windows builds are also compiled statically - no `dll`s are shipped with them.

You can get those in Releases section.

## Build

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`. The project uses CMake as build system.

You'll need `Qt6` (>=6.8.0), `FFmpeg` (>=7.0) and `rapidhash` to build the project.

Clone the repository: `git clone https://github.com/savannstm/revolutionary-audio-player`.

From there you can use `build.ps1` (PowerShell) and `build.sh` (Bash) scripts to build the project into `build` directory.

Script supports `-r` argument for building in `Release` mode.

Build artifacts are output to `build/target` directory.

Default builds of the program include FFmpeg built with the following configuration: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --enable-avformat --enable-avcodec --enable-avfilter --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp,ac3,eac3 --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska --enable-filter=aformat,firequalizer,aresample,alimiter --enable-protocol=file --enable-zlib`

Path to the `rapidhash` headers can be passed as `-DRAPIDHASH_INCLUDE_DIRS`.
