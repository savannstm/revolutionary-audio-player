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

Built releases require AVX instructions, which means the minimum supported processors are Sandy Bridge (3th Gen) for Intel and Bulldozer for AMD (FX-4xxx, FX-6xxx, FX-8xxx).

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

Unfortunately, we had to give up on building static Linux binaries from Alpine.

The main reason for that is sheer vastness of Linux audio backends (ALSA, PulseAudio, PipeWire, OSS, etc.), and the impossibility of building all them statically and linking to the binary. We tried to link only those dynamically, and everything else statically, but that pulls Alpine's musl libc implementation into the linked libraries, and most of the distributions don't provide musl libc libraries.

The second reason - is that static Qt is unable to load plugins on runtime.

[Dependencies you'll need](./docs/linux-deps.md).

### Windows

Windows builds are built statically, you don't need any DLLs.

Minimum supported Windows version is Windows 10.

## Build

See [BUILDING.MD](./docs/building.md).
