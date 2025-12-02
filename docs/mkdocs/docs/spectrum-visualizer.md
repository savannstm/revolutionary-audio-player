# Spectrum Visualizer

Revolutionary Audio Player ships Revolutionary Spectrum Visualizer™ and shows it by default.

![Spectrum Visualizer](assets/spectrum-visualizer-menu.png)

You can leave it be in the main window or double-click it to detach. You can later re-attach it by dropping it in the main window.

![Detached Spectrum Visualizer](assets/spectrum-visualizer-detached.png)

Its settings can be accessed by right-clicking it.

## Mode

You can change the mode controlling how spectrum is visualized:

-   Peak (Relative) - Relative peaks, with all peaks calculated according to the highest peak.
-   Peak (dBFS) - [dBFS](https://en.wikipedia.org/wiki/DBFS) peaks.
-   Peak (Equal) - dBFS peaks with normalization. If some peak is lower than 0.35, it gets boosted by 0.35.
-   Waveform - [Waveform](https://en.wikipedia.org/wiki/Waveform) of the sound.

## Peak Count

_This option is exclusive for peak visualization modes._

Allows you to set the amount of peaks that will be used when visualizing.

This option is both cosmetic and not: higher amount of peaks allows for better distinction between frequencies.

Currently has only two modes: `18` and `30`.

_This may change in v1.0._

## Change Peak Gap

_This option is exclusive for peak visualization modes._

Allows you to input custom padding between peaks in pixels, simple as-is.

## Set Gain Factor

_This option is exclusive for peak visualization modes._

Allows you to set gain factor in dB to boost peaks.

Peaks will be boosted by the specified value.

## Presets

180 gradients available for use with the visualizer.

_This will be expanded in v1.0._
