# Changelog

## v0.9.0

### Changes

-   Fixed our Linux builds. Now they're not statically built, and depend on glibc, xcb, wayland, and PulseAudio
-   Migrated from using QtMultimedia for audio output and device managment to `miniaudio` library. This mainly improves building process, but also may help with performance
-   Implemented Winamp-style visualizer using incredible `projectM` library. Information about getting presets can be found at [Visualizer Presets](./README.md#visualizer-presets) README section
-   Moved `Settings` menu bar option to `File` menu
-   Fixed `rap.log` file being created in current working directory, instead of in the root of the program
-   Fixed BPM metadata field always being empty
-   Added an ability to toggle fullscreen in cover window with F11 key
-   Tested support for different sample rates and channel layouts. Seamless playback guaranteed for formats up to 384 KHz and 8 (7.1) channels
-   Fixed not setting the current index to the correct item when adding tracks to playlists
-   Fixed inability to re-add deleted tracks to the playlist
-   Fixed `.ac3` files not opening in the program because `.ac3` format container demuxer was absent
-   Added Drag & Drop for playlist files
-   When enabling file associations in settings, playlist files now will also be associated with the player
-   Fixed context menu entry on Linux, so now it works only for directories
-   Fixed panic when opening directories through context menu entry
-   #2 Fixed - now selecting multiple directories via system context menu entry opens them all
-   More logs to `rap.log` file
-   From now on, binary executable files of the player will be compressed by UPX with `--best` flag, and thus the size will be reduced by ~2x times. It won't impact performance, although may trigger some antiviruses

### Broken

-   Peak visualization for formats with more than 2 channels
-   #1 Playback stutters still occur, it will be fully addressed in the next version

### Coming in v0.10.0

-   We'll try to completely fix playback stutters
-   Settings will be split into sections, allowing finer control over them
-   We'll fix visualization for >2 channel formats
-   Custom location for settings file
-   Hot reload for available output devices in settings
-   18 and 30 band default presets

## v0.8.0

-   Making moves to try to fix occasional playback stuttering
-   Added information about Qt and FFmpeg version to About window
-   Separated playlists from settings. Now, playlists are stored in `rap-playlists.json`, while settings are in `rap-settings.json`
-   Fixed incorrect display of progress time in some cases
-   CUE fixes:
    -   When switching tracks that are in the same playlist, the audio file now won't be reopened
    -   Fixed playing one more second than track needs
    -   Tracks are now properly saved/loaded
-   Fixed some tracks repeating their start twice, because of a flow in logic that was introduced with CUE playing implementation
-   More performance minmaxxing

## v0.7.0

-   Fix missing BPM metadata for different music formats
-   Fixed panic when deleting tabs to the left
-   Implemented `.cue` sheets playing
-   Implemented a menu for selecting a range to repeat. It's accessible via right mouse button when repeat mode is set to repeat a single track
-   Some performance minmaxxing

## v0.6.1

-   Fixed panic when deleting the last tab.
-   Changed behavior of dropping multiple folders to the application's window: now each of them is processed as a separate tab.
-   When searching a track in a tree, the program doesn't set table's index to the found match and doesn't interrupt playback order.
-   Fixed missing `.ac3` format association on Windows.
-   Possibly implemented file associations for Linux.
-   Fixed random panic related to peak visualizer when playing/switching tracks.
-   Maybe slightly improved peak visualizer's performance.
-   Fixed panic when finishing playing the last track in a playlist.

## v0.6.0

-   Implemented simple peak visualizer with presets.
-   When no track is selected and play button is pressed, the first track in the tree will be played now.
-   Fixed issues with adding files due to possible extension mismatches.
-   Removed retranslation of the version element in about window, which caused it to disappear.
-   When opening/dropping folders in the program, it correctly creates tab with these folders' names, instead of theirs parents names.
-   Added `rap.log` file, where some logs of the program is output.
-   Maybe improved the performance of playback and opening files in the table.
-   Fixed problems when deleting tabs to the left of the selected, and made tab removal even less error-prone.
-   Fixed not removing the old cover from the dock widget when switching to a new track that has no cover.

## v0.5.1

-   Made equalizer usable.
-   Added Dolby AC3/EAC3 formats support.

## v0.5.0

-   Fixed non-working track rewinding with `wav` or any other issues, for sure this time.
-   Fixed wrong "Disabled" equalizer menu button text when it's actually enabled.
-   Made the program update Windows registry associations on each run in case if binary was moved.
-   Fixed non-working equalization when playing different tracks.
-   Implemented drag and drop in track tables and `Order` column, which preserves the order of rearranged tracks.
-   Removed files drag and drop option in favor of dropping files directly into the playlist to add tracks or outside the playlist to add new playlist.
-   Implemented options to automatically set playlist background when playing a track and to prefer external covers instead of internal.
-   Added dragging delay to playlist tabs dragging to prevent false drags.
-   Fixed messed up styling of playlist context menus.
-   Next up: Peak visualizer and `.cue` support.

## v0.4.1

-   Fixed non-working `.wav` playback and skipping a couple of milliseconds when playing `.wav` files.
-   Fixed ugly appearance of cover window's context menu.
-   Reimplemented equalizer with FFmpeg's `libavfilter` instead of `JUCE` framework - something might sound different.
-   Started to ship static Linux builds.

## v0.4.0

-   FIXED NON-WORKING REPEAT.
-   Fix mismatching track properties/values.
-   Made playlist table's columns resize on selecting new language, or when adding new column.
-   Fixed saving incorrect dock position.
-   Fixed panic when background image for a playlist from settings doesn't exist.
-   Made opening directories kinda visible in the table.
-   Fixed panic when resizing dock widget, but no playlist tab is selected.
-   Made tab removal less error-prone.
-   Correctly implemented context menu entry and file associations for Windows.
-   Probably fixed messed up application icon for Windows?
-   Fixed non-working playlist importing.
-   Fixed opening files/directories when file associations are enabled and opened entries contained Unicode.
-   Removed retranslating tab labels when selecting new language.
-   Implemented tab bar scrolling and tab dragging.

## v0.3.0

-   Fixed non-working repeat.
-   Fixed cover window resize and fullscreen issues.
-   Revamped equalizer menu to preset-based approach.
-   Fixed non-working playing of variable-bitrate formats (ogg, opus) and wav format.
-   Implemented changeable audio output device (also made audio device automatically change, when it's changed in the system).
-   Fixed possible panic when removing playlist's tree rows.
-   Fixed leading `/` or `\` symbols in tracks' titles, when title is not defined in the metadata.

## v0.2.2

-   Another load of playback fixes.
-   Fixed adding the same tracks to the playlist which already has them.
-   Fixed tabs behavior when sometimes two tabs will be selected simultaneously.

## v0.2.1

-   Changed exported `.xspf` and `.m3u8` playlists to use relative paths.
-   Fixed crashing when interacting with settings.
-   Added arguments parsing when opening files through system's context menu.
-   Some fixes in playback toggling system.
-   Proper repositioning of dock widget.
-   Corrected the behavior of disabled/playlist repeat modes.
-   Fixed corrupted equalizer output.
-   Added the option to add "Open in RAP" item to Windows Explorer context menus.

## v0.2.0

-   Copied progress and volume sliders and labels to the tray icon menu.
-   Implemented `.xspf` and `.m3u8` playlists import/export.
-   Playlist tree headers' order and visibility are now stored in settings.
-   Fixed 20 MB memory allocation related to play button emoji usage.
-   Added a dock window with track metadata and cover.
-   Established a cool-ass hammer and sickle revolutionary :trollface: icon.

## v0.1.0

-   Initial release.
