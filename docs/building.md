You need a C++23 compatible compiler: `clang`, `gcc`, `msvc`. The project uses CMake as build system.

Libraries you'll need:

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

Clone the repository: `git clone https://github.com/savannstm/revolutionary-audio-player`.

We provide configure scripts - `build.ps1` for PowerShell and `build.sh` for sh/bash.

You can get the list of available options using `./configure --help`.

After running configure with the desired arguments, you can `cd` to the build directory and run `cmake --build . -j`, that will build the player.

Build artifacts are output to `build/target` directory.

Default builds of the program include:

-   FFmpeg built with the following configuration: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --disable-autodetect --enable-avformat --enable-avcodec --enable-avfilter --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp,ac3,eac3 --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska,ac3,mjpeg,eac3 --enable-filter=aformat,firequalizer,aresample,alimiter --enable-protocol=file --enable-zlib`
-   Qt6 built with the following configuration:
    -   Windows: `-static -release -nomake tests -nomake examples -nomake benchmarks -no-opengl -system-harfbuzz -system-freetype -system-libpng -system-libjpeg -system-webp -system-tiff -system-zlib -system-doubleconversion -system-pcre -no-emojisegmenter -no-icu -no-gif -gui -widgets -submodules qtbase,qtimageformats,qttools -qpa "windows" -disable-deprecated-up-to 0x068000`
