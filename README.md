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
-   [x] Optimized audio processing of any sample rate, up to 8 channels.
-   [x] Native look and the same capabilities on **any** platform.
-   [x] Interface, that keeps it simple.
-   [x] Completely portable. Leaves **no** traces in OS by default!
-   [x] Default tray icon support, close button only hides the window.
    -   [x] Volume and progress sliders built **right inside** the tray icon menu!
-   [x] Built-in ready-to-use 3-, 5-, 10-, 18- and 30-band equalizers with presets.
-   [x] Flexible playlist system with:
    -   [x] Track searching.
    -   [x] Playlist background images system.
    -   [x] `.xspf`, `.m3u8`, `.m3u` and `.cue` playlists import/export.
    -   [x] Dock window with metadata.
-   [x] You can select a **range to repeat**, when repeat mode is set to repeat a single track.
-   [x] Peak visualizer.
-   [x] WinAMP-style incredible visualizer.

## Why a new audio player?

There's many audio players already out there, big amount of which is either filled with unnecessary things, or just legacy, like the extremely cool Windows `foobar2000` audio player.

This audio player emphasizes clean codebase, little amount of dependencies, simple interface, cross-platform distribution, and being easy overall.

## Releases

You can get those in **Releases** section.

### Visualizer Presets

We don't ship any presets for our visualizer.

Here's where you can get the presets, from [projectM repository](https://github.com/projectM-visualizer/projectm):

-   [Cream of the Crop Pack](https://github.com/projectM-visualizer/presets-cream-of-the-crop) - A collection of about 10K
    presets compiled by Jason Fletcher. Currently, projectM's default preset pack.
-   [Classic projectM Presets](https://github.com/projectM-visualizer/presets-projectm-classic) - A bit over 4K presets
    shipped with previous versions of projectM.
-   [Milkdrop 2 Presets](https://github.com/projectM-visualizer/presets-milkdrop-original) - The original preset
    collection shipped with Milkdrop and Winamp.
-   [En D Presets](https://github.com/projectM-visualizer/presets-en-d) - About 50 presets created by "En D".

Included with projectM are the bltc201, Milkdrop 1 and 2, projectM, tryptonaut and yin collections. You can grab these
presets [here](http://spiegelmc.com/pub/projectm_presets.zip).

You can also download an enormous 130k+ presets from the MegaPack [here](https://drive.google.com/file/d/1DlszoqMG-pc5v1Bo9x4NhemGPiwT-0pv/view) (4.08GB zipped, incl. textures).

### Linux

Unfortunately, we had to give up on building static Linux binaries from Alpine. The main reason for that is sheer vastness of Linux audio backends (ALSA, PulseAudio, PipeWire, OSS, etc.), and the impossibility of building all them statically and linking to the binary. We tried to link only those dynamically, and everything else statically, but that pulls Alpine's musl libc implementation into the linked libraries, and most of the distributions don't provide musl libc libraries.

You'll usually need the following shared libraries in your distribution to seamlessly run the player:

-   GNU libc
    -   libc
    -   libstdc++
    -   libgcc_s
    -   libm
    -   librt
    -   ld-linux
-   libX11
    -   libXau
    -   libXdmcp
-   libxcb
    -   libxcb-cursor
    -   libxcb-icccm
    -   libxcb-image
    -   libxcb-keysyms
    -   libxcb-randr
    -   libxcb-render-util
    -   libxcb-shm
    -   libxcb-sync
    -   libxcb-xfixes
    -   libxcb-render
    -   libxcb-shape
    -   libxcb-xkb
    -   libxcb-xinput
    -   libxcb-util
    -   libICE
    -   libSM
-   libxkbcommon
    -   libxkbcommon-x11
-   libffi
-   libOpenGL
    -   libGLX
    -   libEGL
    -   libGLdispatch
-   libwayland
    -   libwayland-client
    -   libwayland-cursor
    -   libwayland-egl

On different distros, you get them different ways:

```bash
# Debian, Debian-based (Ubuntu, Kubuntu etc.)
sudo apt update
sudo apt install -y \
    libxkbcommon-x11-0 \
    libxkbcommon0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-xkb1 \
    libxcb-xinput0 \
    libxcb1 \
    libxcb-util1 \
    libxau6 \
    libxdmcp6 \
    libsm6 \
    libice6 \
    libc6 \
    libstdc++6 \
    libgcc-s1 \
    libffi8 \
    libglx0 \
    libopengl0 \
    libegl1 \
    libgl1 \
    libwayland-client0 \
    libwayland-cursor0 \
    libwayland-egl1

# Arch, Arch-based (Endeavour, Cachy), Manjaro
sudo pacman -Syu --needed \
    libxkbcommon-x11 \
    libxkbcommon \
    libxcb \
    xcb-util-cursor \
    xcb-util-keysyms \
    xcb-util-wm \
    libxau \
    libxdmcp \
    gcc-libs \
    glibc \
    libffi \
    libglvnd \
    wayland

# Alpine
apk add \
    libxkbcommon-x11 \
    libxkbcommon \
    libxcb \
    xcb-util \
    xcb-util-cursor \
    xcb-util-keysyms \
    xcb-util-wm \
    xcb-util-image \
    libxau \
    libxdmcp \
    libc6-compat \
    libstdc++ \
    gcc \
    musl \
    libffi \
    libglvnd \
    wayland-libs-client \
    wayland-libs-cursor \
    wayland-libs-egl


# OpenSUSE
sudo zypper install -y \
    libxkbcommon-x11-0 \
    libxkbcommon0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-xkb1 \
    libxcb-xinput0 \
    libxcb1 \
    libxcb-util1 \
    libXau6 \
    libXdmcp6 \
    libsm6 \
    libice6 \
    glibc \
    libstdc++6 \
    libgcc_s1 \
    libffi8 \
    libGLX0 \
    libEGL1 \
    libOpenGL0 \
    libGLdispatch0 \
    libwayland-client0 \
    libwayland-cursor0 \
    libwayland-egl1
```

Also, you'll need PulseAudio-compatible audio backend. It's either PulseAudio directly or PipeWire-pulse.

On different distros, you get them different ways:

```bash
# PipeWire-pulse

## Debian, Debian-based (Ubuntu, Kubuntu etc.)
sudo apt install libpipewire-0.3-0 libpipewire-0.3-modules

## Arch, Arch-based (Endeavour, Cachy), Manjaro
sudo pacman -S pipewire pipewire-pulse

## Alpine
sudo apk add pipewire-libs pipewire-pulse

## OpenSUSE
sudo zypper install libpipewire-0_3 pipewire-pulseaudio

# PulseAudio

## Debian, Debian-based (Ubuntu, Kubuntu etc.)
sudo apt install pulseaudio pulseaudio-utils

## Arch, Arch-based (Endeavour, Cachy), Manjaro
sudo pacman -S pulseaudio pulseaudio-alsa

## Alpine
sudo apk add pulseaudio pulseaudio-utils

## OpenSUSE
sudo zypper install pulseaudio pulseaudio-utils
```

### Windows

Windows builds are built statically, you don't need any DLLs.

Minimum supported Windows version is Windows 10.

## Build

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`. The project uses CMake as build system.

Libraries you'll need:

-   Qt6 (>= 6.8.0)
    -   Core
    -   Concurrent
    -   Thread
    -   Network
    -   Harfbuzz
    -   Widgets
    -   GUI
    -   PNG (optional, for PNG support)
    -   JPEG (optional, for JPEG support)
    -   QtTools (optional, for translations creation; you may need to patch CMakeLists if you won't build qtlinguist)
    -   QtImageFormats (optional, for WEBP/TIFF support)
    -   **Linux:** XCB (for `qxcb` plugin, necessary for drawing windows in X11/Wayland), Qt6Svg (for `.svg` icons), Wayland (optional, for `qwayland` plugin), fontconfig (optional, for auto system font detection)
-   miniaudio (>=0.11.23)
-   rapidhash
-   FFmpeg (>=7.1.1)
    -   avformat
    -   avcodec
    -   avfilter
    -   swresample
-   libprojectM (optional, >= 4.1.4)
    -   GLEW (optional, Windows dependency)

Clone the repository: `git clone https://github.com/savannstm/revolutionary-audio-player`.

From there you can use `build.ps1` (PowerShell) and `build.sh` (Bash) scripts to build the project into `build` directory.

Script supports `-r` argument for building in `Release` mode.

Build artifacts are output to `build/target` directory.

Default builds of the program include:

-   FFmpeg built with the following configuration: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --enable-avformat --enable-avcodec --enable-avfilter --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp,ac3,eac3 --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska,ac3,mjpeg,eac3 --enable-filter=aformat,firequalizer,aresample,alimiter --enable-protocol=file --disable-autodetect --enable-zlib`
-   Qt6 built with the following configuration:
    -   Linux: `-static -release -nomake tests -nomake examples -nomake benchmarks -opengl desktop -system-harfbuzz -system-freetype -system-libpng -system-libjpeg -system-webp -system-tiff -system-zlib -system-doubleconversion -system-pcre -no-gtk -no-glib -no-ico -no-directfb -no-eglfs -no-gbm -no-kms -no-linuxfb -no-evdev -no-mtdev -no-tslib -no-emojisegmenter -no-cups -no-ssl -no-openssl -no-bundled-xcb-xinput -no-xcb-xlib -no-dbus -no-icu -xcb -fontconfig -gui -widgets -submodules qtbase,qtsvg,qtimageformats,qtwayland,qttools -qpa "xcb;wayland"`
    -   Windows: `-static -release -nomake tests -nomake examples -nomake benchmarks -opengl desktop -system-harfbuzz -system-freetype -system-libpng -system-libjpeg -system-webp -system-tiff -system-zlib -system-doubleconversion -system-pcre -no-emojisegmenter -no-icu -gui -widgets -submodules qtbase,qtimageformats,qttools -qpa "windows"`
