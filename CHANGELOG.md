# Changelog

## v0.11.0

### Changes

#### Spectrum Visualizer

-   Spectrum visualizer now can be detached from the main window, and will behave as a separate top-level window.
-   Removed missing preset number from the preset list.
-   Implemented waveform mode for spectrum visualizer.
-   Implemented "Equal" mode for spectrum visualizer, which calculates dBFS peaks like in dBFS mode, but boosts peaks if they're too weak.
-   Implemented setting custom padding between peaks.
-   Implemented setting custom gain factor, which boosts the peaks for the given number of decibels.

#### Playlist Appearance

-   When setting playlist tab color, text color in the tab now will be changed accordingly.
-   Implemented changing playlist background opacity.
-   Fixed setting wrong color to the playlist background when opacity is changed.

#### Other

-   Fixed not showing visualizer dialog window on Windows.
-   Fixed replacing any non-ASCII text to question marks in visualizer window on Windows.
-   Implemented preset import/export for equalizer.

### Broken

-   Devices hot-reload in settings window on Windows.
-   Sliders in tray icon menu don't display on KDE (possibly other DEs).
-   Shell context menu entries in different Linux DEs (WIP).

### Coming in v0.12.0

-   Possible integration with last.fm API for music data scraping.
-   Linux desktop environments shell integration polishing.
-   MkDocs-based documentation.
-   Auto-updater.
-   Devices hot-reload fixes on Windows.

## v0.10.0 - Polishing, Linux improvements, Wayland fixes

### Changes

**Linux binaries are now linked dynamically to Qt, see [required libraries here](./docs/linux-deps.md)**

#### Playback Fixes

-   Fixed non-working repeat of a single track.
-   Fixed the situation where a track would continue playing even after playback was stopped, if play button is pressed again.
-   Fixed crashes when switching tracks while having the visualizer window opened.
-   Fixed not opening the track from association because of a logic flow that was introduced after switching to `miniaudio` library for audio playback.
-   Fixing #1 playback stutters.

#### Playlists

-   Fixed not showing scroll bar in tab bar when there's many playlists opened, and dock widget is minimized.
-   Fixed not filling the created playlist when files are dropped into the player.

#### Equalizer Updates

-   Added default 18- and 30-band presets for equalizer (these are just stretched versions of 10-band ones).
-   Fixed equalizer corrupting audio when playing audio with too high sample rate and many channels.

#### Visualizer Updates

-   Fixed peak (spectrum) visualizer for any audio formats.
-   Spectrum visualizer is now available with only either 18 or 30 bands.
-   Added feature to maximize visualizer to fullscreen by pressing F11.
-   Visualizer was rewritten from using Qt6 OpenGL to open a separate GLFW window, which ensures maximum portability and less pain in the ass.

#### Settings & Configuration

-   Settings were split into sections, and several additions were made:
    -   Added a setting to explicitly change color theme.
    -   Added ability to change settings/playlists files' location (paths are stored alongside `rap` executable; after reinstalling, directories need to be located again).
    -   Associations for file formats can now be enabled/disabled per format.
    -   Added ability to change default playlist columns and their ordering.
    -   Added "Default" application style option.
-   Implemented playback devices hot reload.

#### File & System Integration

-   Fixed check of enabled associations (now based on whether the file/shell associations actually exist in the system).
-   Linux:
    -   Fixed `Create "Open in RAP" directory context menu item` working for files and not directories on Linux.
    -   Fixed not opening files in the application if supplied as arguments while the executable is already opened.
    -   Fixed not opening for the first time on Linux if the application wasn't closed properly.

#### Localization

-   Fixed track search input not retranslating when the language is changed.
-   Removed retranslating of some widgets that shouldn't be retranslated.
-   Added retranslating of some widgets that should be retranslated.

#### Other Updates

-   Starting from v0.10.0, releases will be compiled with `-march=sandybridge` flag (Intel 3rd Gen with AVX support).
-   Removed blank entries from tray icon menu on Linux platform, which are actually supposed to be sliders. But sliders are not widely supported, so whatever.
-   Fixed system window bar not appearing after maximizing cover window.
-   Implemented skip sections for repeat menu. Now you can enter custom sections to skip.
-   Allowed using theme plugins by linking Qt dynamically.
-   Fixed some dialog window appearing in the center of the screen instead of where they should.

### Broken

-   Sliders in tray icon menu don't display on KDE (possibly other DEs).
-   Shell context menu entries in different Linux DEs (WIP).
-   Set Always On Top does not work on Wayland (and it shouldn't, because in Wayland the compositor decides which properties should window have).

### Coming in v0.11.0

-   Floating spectrum visualizer, and more spectrum visualizer customization
-   Import/export for visualizer presets
-   Possible integration with last.fm API for music data scraping

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
