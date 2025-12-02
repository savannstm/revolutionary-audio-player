# Building

You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`. The project uses CMake as build system.

## Required libraries

-   Qt6 (>= 6.8.2)
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
-   miniaudio (>= 0.11.23)
-   rapidhash
-   FFmpeg (>= 7.1.1)
    -   avformat
    -   avcodec
    -   avfilter
    -   swresample
-   libprojectM (optional, >= 4.1.4)
    -   GLEW (Windows dependency)
    -   GLFW (>= 3.4, because Qt OpenGL sucks)
-   magic_enum (>= 0.9.4, dev-dependency)

### Getting development headers on Linux

Note that `miniaudio`, `rapidhash` and `magic_enum` are **HEADER ONLY** libraries - rather than installing them from your package manager, you should manually acquire a header.

Since `libprojectM` is outdated in every possible package manager, you'll have to build it from source.

#### Debian

```bash
sudo apt update
sudo apt install \
  qt6-base-dev qt6-tools-dev qt6-svg-dev qt6-imageformats-plugins \
  libavformat-dev libavcodec-dev libavfilter-dev libswresample-dev \
  glfw3-dev
```

#### OpenSUSE

```bash
sudo zypper install \
    libqt6-qtbase-devel \
    libqt6-qttools-devel \
    libqt6-qtsvg-devel \
    libqt6-qtimageformats \
    libavformat-devel libavcodec-devel libavfilter-devel libswresample-devel \
    glfw-devel
```

#### Arch

```bash
sudo pacman -S --needed \
    qt6-base qt6-tools qt6-svg qt6-imageformats \
    ffmpeg \
    glfw
```

#### Alpine

```bash
sudo apk add \
    qt6-qtbase-dev \
    qt6-qttools-dev \
    qt6-qtsvg-dev \
    qt6-qtimageformats-dev \
    ffmpeg-dev \
    glfw-dev
```

## Building Process

Clone the repository: `git clone https://github.com/savannstm/revolutionary-audio-player`.

We provide configure scripts - `build.ps1` for PowerShell and `build.sh` for sh/bash.

You can get the list of available options using `./configure --help`.

After running configure with the desired arguments, you can `cd` to the build directory and run `cmake --build . -j`, that will build the player.

Build artifacts are output to `build/target` directory.

Default builds of the program include:

-   FFmpeg built with the following configuration: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --disable-autodetect --enable-avformat --enable-avcodec --enable-avfilter --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp,ac3,eac3 --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska,ac3,mjpeg,eac3 --enable-filter=aformat,firequalizer,aresample,alimiter --enable-protocol=file --enable-zlib`
-   Qt6 built with the following configuration:
    -   Windows: `-static -release -nomake tests -nomake examples -nomake benchmarks -no-opengl -system-harfbuzz -system-freetype -system-libpng -system-libjpeg -system-webp -system-tiff -system-zlib -system-doubleconversion -system-pcre -no-emojisegmenter -no-icu -no-gif -gui -widgets -submodules qtbase,qtimageformats,qttools -qpa "windows" -disable-deprecated-up-to 0x068000`
