# Playback

This section will cover details about the implementation of playback.

## Repeat

By clicking the repeat button, you can toggle repeat mode which will affect playback.

There's three default modes:

-   No repeat - after reaching the end of the current playlist start playing the next playlist.
-   Playlist repeat - after reaching the end of the current playlist, start over.
-   Track repeat - after reaching the end of the current track, start over.

### Track Repeat

When in a track repeat mode, you can open a track repeat menu.

![Track Repeat Menu](assets/track-repeat-menu.png)

This menu allows you to select some certain range in track to repeat, or add skip sections. If some song has a boring segment from 1:09 to 1:31, just add a skip segment that covers this time. You won't hear it!

## Shuffle

By clicking the shuffle button, you can toggle shuffle which will play the random track from the playlist at the end of the current one.

## Supported Formats

Supported sample formats are `pcm_s16le`, `pcm_s24le`, `pcm_s32le` and `pcm_f32le`. All supported encoded formats use those (or their planar versions) internally. We don't support `u8`, `s64` and `f64` formats since those are not in wide usage and effectively deprecated, the same can be said for big endian formats.

Up to 8 channels and infinite sample rate stably supported. More than 8 channels is not supported, but more than 8 channels aren't even widely supported by codecs - most commonly used `MP3` does not support more than two channels. This may be changed in v1.0.

Supported encoded formats are `Vorbis`, `Opus`, `FLAC`, `MP3`, `AAC`, `ALAC`, `AC3`.

Containers for `Vorbis`/`Opus` and `AAC`/`ALAC`/`AC3` are also supported, those include `OGG`/`OGA`/`OGV`/`OGX` and `MOV`/`M4A`/`MP4`/`MKV`.

`OGM` is not supported since it's not official Xiph.org format, so it may not work.

Playback is highly optimized, and, don't want to brag about it, but it might be the fastest among all other players. Not slower definitely.

## Buffering

First of all, default builds of the program prepare 8 buffers when starting a stream beforehand. If at some point I/O stutter happens, these 8 buffers ensure that the consumer will have enough data before actually stuttering the sound in the playback.

That also means that equalization of the audio will happen with a delay if it's toggled while playback is active.

## Parsing Encoded Formats

For fetching file metadata and decoding encoded formats, we use `FFmpeg`. It's highly efficient and fast, written with a lot of hand-tailored assembly, and uses known libraries underneath.

## Parsing Raw Formats

Single raw format we support is `WAV`, and parsing of this format is pretty efficient: is just a single syscall to read next chunk of bytes from the file.
