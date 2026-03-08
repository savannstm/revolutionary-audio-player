# Dependencies on Linux

- libQt6
  - libQt6Core
  - libQt6Widgets
  - libQt6Gui
  - libQt6Svg
  - libQt6Network
  - libQt6DBus
- FFmpeg
  - avformat
  - avcodec
  - avfilter
  - swresample
- libglfw3
- libpulse

Note that the application requires Qt6 of version 6.8.2 or higher - it's not present in Ubuntu 24.04 and 24.10.

## Debian/Ubuntu

```bash
sudo apt install \
    libglfw3 \
    libqt6core6 libqt6widgets6 libqt6gui6 libqt6svg6 libqt6network6 libqt6dbus6 \
    libpulse0 \
    ffmpeg
```

## OpenSUSE

```bash
sudo zypper install \
    glfw3 \
    qt6-base qt6-svg \
    libpulse0 \
    ffmpeg-8
```

## Arch

```bash
sudo pacman -S \
    glfw \
    qt6-base qt6-svg \
    libpulse \
    ffmpeg
```

## Alpine

```bash
sudo apk add \
    glfw \
    qt6-qtbase qt6-qtsvg \
    pulseaudio-libs \
    ffmpeg
```
