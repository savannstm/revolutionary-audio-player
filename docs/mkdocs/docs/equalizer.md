# Equalizer

Revolutionary Audio Player ships built-in ready-to-use equalizer with seamless integration.

![Equalizer](assets/equalizer.png)

## Bands

Using the bands combobox in the layout, you can select the desired number of bands. That controls how many bands will participate in equalization.

There's 5 fixed band counts: 3, 5, 10, 18 and 30.

Band counts may change in v1.0.

## Presets

By default, program ships 25 default presets for different music genres, those cannot be deleted.

You can create new presets and rename them, and also you can import/export presets.

### Preset File Format

Presets are stored in binary format with `.rap` extension.

It may change in v1.0 - we can switch to the text format to allow changing presets by hand, and change extension from `.rap` to `.rape` (stands for RAP Equalizer), for compatibility with our `.rapm` media file extension.

## Gains

Each band allows settings gain from +20 dB to -20 dB (This may change in v1.0).

Our equalizer uses Finite Impulse Response (FIR) equalizer, meaning band gains won't overlap and boost nearby frequencies.

## Note

If you want to study how our equalizer works, everything is open inside `AudioStreamer.cpp` file in `equalizeBuffer` function.

In short: We use FFmpeg filters in order to perform equalization; more precisely we use `firequalizer` filter.

This could be benefitial if you want to study how to use FFmpeg filters.
