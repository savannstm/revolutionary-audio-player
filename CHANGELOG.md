# Changelog

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
