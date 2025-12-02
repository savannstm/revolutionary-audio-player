# Settings

Right now, the settings window is split to three sections: core settings, shell integration settings and playlists settings.

## Core

Core settings contain options altering the core aspects of the program.

![Core Settings](assets/core-settings.png)

-   Style - You can change style to the one available on your system. Windows static builds always ship only 4 styles: Windows, Windows Vista, Windows 11 and Fusion. On Linux, player picks up available plugins (such as Breeze) automatically.
-   Theme - You can change theme from dark to light and vice versa. Currently only has effect on Windows.
-   Output Device - You can change default output device to something specific, e. g. from headphones to speakers.
-   Settings Directory - Location of the settings directory, settings file will be stored there between runs.
-   Playlists Directory - Location of the settings directory, settings file will be stored there between runs.

## Shell Integration

This section allows you to create directory context menu entry or set associations for file formats.

![Shell Integration Settings](assets/shell-settings.png)

-   Create "Open in RAP" directory context menu item - When right-clicking a directory, opened context menu will have "Open in RAP" button to open it in the player.
-   Associations for file formats - You can toggle different formats, and then set the associations using "Set" button. Unchecked associations will be removed entrirely from the system, don't worry about any leftovers.

## Playlists

These settings are for the playlists and mostly cosmetical.

![Playlist Settings](assets/playlist-settings.png)

-   Automatically set playlist background when playing a track - When playing a track in the program, it will set playlist background if track's cover is available.
-   Prioritize external cover over embedded - When setting playlist background automatically, program takes the embedded cover by default. If this option is checked, program will search for higher-quality `cover.ext` file to use.
-   Playlist Naming - this option controls how opened playlist will be named.
    -   Directory Name - will use the directory name if directory is opened in the program. If file is opened, it will use the name of the parent directory of that file.
    -   Track Metadata - will use the album name from album field in track metadata. If it's not present, will revert to directory name.
    -   Numbered - playlists will be named by their number. If you have two playlist opened, the third one will be called "Playlist 3". Playlist naming is numbered when creating empty playlists by default.
