# Билдинг

Вам понадобится C++23-совместимый компилятор: `clang`, `gcc`, `msvc`. Проект использует CMake в качестве билд-системы.

## Необходимые библиотеки

-   Qt6 (>= 6.8.2)
    -   Core
    -   Concurrent
    -   Thread
    -   Network
    -   Harfbuzz
    -   Widgets
    -   GUI
    -   PNG (опционально, для поддержки PNG)
    -   JPEG (опционально, для поддержки JPEG)
    -   QtTools (опционально, для создания переводов; вам возможно придётся запатчить CMakeLists если вы не будете билдить тулы)
    -   QtImageFormats (опционально, для поддержки WEBP/TIFF)
    -   **Linux:** XCB (для плагина `qxcb`, необходимо для рисования окон в X11), Qt6Svg (для `.svg` иконок), Wayland (опционально, для плагина `qwayland`), fontconfig (опционально, для автоматического детектирования системных шрифтов)
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

### Получение заголовков для разработки на Linux

Заметьте, что `miniaudio`, `rapidhash` и `magic_enum` - библиотеки **ТОЛЬКО С ЗАГОЛОВКОМ** - вместо их установки через свой пакетный менеджер, вы должны скачать заголовок из соответствующего репозитория.

Учитывая, что `libprojectM` устарела в каждом возможном пакетном менеджере, вам нужно будет билдить её из исходников.

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

## Процесс билдинга

Клонируйте репозиторий: `git clone https://github.com/savannstm/revolutionary-audio-player`.

Мы предоставляем configure скрипты - `build.ps1` для PowerShell и `build.sh` для sh/bash.

Скрипт может перечислить все доступные опции через `./configure --help`.

После запуска скрипта с желаемыми аргументами, вы можете сделать `cd` в директорию билда и запустить `cmake --build . -j` оттуда, это забилдит плеер.

Артефакты билда выводятся в директорию `build/target`.

Стандартные билды программы включают:

-   FFmpeg построенный со следующей конфигурацией: `--enable-asm --enable-optimizations --enable-stripping --disable-debug --enable-static --disable-all --disable-autodetect --enable-avformat --enable-avcodec --enable-avfilter --enable-swresample --enable-decoder=mp3,flac,opus,aac,alac,vorbis,png,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,jpeg,mjpeg,bmp,webp,ac3,eac3 --enable-demuxer=mp3,flac,ogg,aac,wav,mov,matroska,ac3,mjpeg,eac3 --enable-filter=aformat,firequalizer,aresample,alimiter --enable-protocol=file --enable-zlib`
-   Qt6 построенный со следующей конфигурацией:
    -   Windows: `-static -release -nomake tests -nomake examples -nomake benchmarks -no-opengl -system-harfbuzz -system-freetype -system-libpng -system-libjpeg -system-webp -system-tiff -system-zlib -system-doubleconversion -system-pcre -no-emojisegmenter -no-icu -no-gif -gui -widgets -submodules qtbase,qtimageformats,qttools -qpa "windows" -disable-deprecated-up-to 0x068000`
