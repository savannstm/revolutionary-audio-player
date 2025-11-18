# Dependencies on Linux

-   GNU libc
    -   libc
    -   libstdc++
    -   libgcc_s
    -   libm
-   libX11
    -   libXau
    -   libXdmcp
-   libxcb
    -   libX11-xcb
-   libxkbcommon
-   libffi
-   libglfw3
    -   libOpenGL
    -   libGLX
    -   libGLdispatch
    -   libEGL (Wayland)
-   libwayland (Wayland)
    -   libwayland-client
    -   libwayland-cursor
    -   libwayland-egl
-   libQt6
    -   libQt6Core
    -   libQt6Widgets
    -   libQt6Gui
    -   libQt6Svg
    -   libQt6Network
    -   libQt6DBus
-   libpulse

Note that the application requires Qt6 of version 6.8.2 or higher - it's not present in Ubuntu 24.04 and 24.10. Also, you can skip installing Wayland libraries, if you only intend to use X11.

## Debian/Ubuntu

```bash
sudo apt install \
    libc6 libstdc++6 libgcc-s1 \
    libx11-6 libxau6 libxdmcp6 \
    libxcb1 libxkbcommon0 libffi8 \
    libglfw3 libopengl0 libglx0 libglvnd0 libegl1 \
    libwayland-client0 libwayland-cursor0 libwayland-egl1 \
    libqt6core6 libqt6widgets6 libqt6gui6 libqt6svg6 libqt6network6 libqt6dbus6 \
    libpulse0
```

## OpenSUSE

```bash
sudo zypper install \
    glibc libstdc++6 libgcc_s1 \
    libX11-6 libXau6 libXdmcp6 \
    libxcb1 libxkbcommon0 libffi8 \
    glfw3 Mesa-libOpenGL1 Mesa-libGLX0 libglvnd Mesa-libEGL1 \
    libwayland-client0 libwayland-cursor0 libwayland-egl1 \
    qt6-base qt6-svg \
    libpulse0
```

## Arch

```bash
sudo pacman -S \
    glibc gcc-libs \
    libx11 libxau libxdmcp \
    libxcb libxkbcommon libffi \
    glfw-x11 mesa libglvnd \
    wayland qt6-base qt6-svg \
    libpulse
```

## Alpine

```bash
sudo apk add \
    musl libstdc++ gcc \
    libx11 libxau libxdmcp \
    libxcb libxkbcommon libffi \
    glfw mesa mesa-glx mesa-egl libglvnd \
    wayland-libs-client \
    qt6-qtbase qt6-qtsvg \
    pulseaudio-libs
```
