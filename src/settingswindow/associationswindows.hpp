// NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
// The reason for disabling this lint check is that all string views constructed
// from string literals always include null terminator at end upon calling
// `.data()`.
#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "log.hpp"

#include <windows.h>

constexpr wstring_view DEFAULT_KEY;

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    EXTENSION_ENTRIES = {
        LR"(Software\Classes\.mp3)",  LR"(Software\Classes\.flac)",
        LR"(Software\Classes\.opus)", LR"(Software\Classes\.aac)",
        LR"(Software\Classes\.wav)",  LR"(Software\Classes\.ogg)",
        LR"(Software\Classes\.m4a)",  LR"(Software\Classes\.mka)",
        LR"(Software\Classes\.alac)", LR"(Software\Classes\.ac3)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> PROG_IDS = {
    L"rap.mp3", L"rap.flac", L"rap.opus", L"rap.aac",  L"rap.wav",
    L"rap.ogg", L"rap.m4a",  L"rap.mka",  L"rap.alac", L"rap.ac3"
};

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    PROG_ID_ENTRIES = {
        LR"(Software\Classes\rap.mp3)",  LR"(Software\Classes\rap.flac)",
        LR"(Software\Classes\rap.opus)", LR"(Software\Classes\rap.aac)",
        LR"(Software\Classes\rap.wav)",  LR"(Software\Classes\rap.ogg)",
        LR"(Software\Classes\rap.m4a)",  LR"(Software\Classes\rap.mka)",
        LR"(Software\Classes\rap.alac)", LR"(Software\Classes\rap.ac3)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    ICON_ENTRIES = {
        LR"(Software\Classes\rap.mp3\DefaultIcon)",
        LR"(Software\Classes\rap.flac\DefaultIcon)",
        LR"(Software\Classes\rap.opus\DefaultIcon)",
        LR"(Software\Classes\rap.aac\DefaultIcon)",
        LR"(Software\Classes\rap.wav\DefaultIcon)",
        LR"(Software\Classes\rap.ogg\DefaultIcon)",
        LR"(Software\Classes\rap.m4a\DefaultIcon)",
        LR"(Software\Classes\rap.mka\DefaultIcon)",
        LR"(Software\Classes\rap.alac\DefaultIcon)",
        LR"(Software\Classes\rap.ac3\DefaultIcon)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    SHELL_ENTRIES = {
        LR"(Software\Classes\rap.mp3\shell)",
        LR"(Software\Classes\rap.flac\shell)",
        LR"(Software\Classes\rap.opus\shell)",
        LR"(Software\Classes\rap.aac\shell)",
        LR"(Software\Classes\rap.wav\shell)",
        LR"(Software\Classes\rap.ogg\shell)",
        LR"(Software\Classes\rap.m4a\shell)",
        LR"(Software\Classes\rap.mka\shell)",
        LR"(Software\Classes\rap.alac\shell)",
        LR"(Software\Classes\rap.ac3\shell)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    SHELL_OPEN_ENTRIES = {
        LR"(Software\Classes\rap.mp3\shell\open)",
        LR"(Software\Classes\rap.flac\shell\open)",
        LR"(Software\Classes\rap.opus\shell\open)",
        LR"(Software\Classes\rap.aac\shell\open)",
        LR"(Software\Classes\rap.wav\shell\open)",
        LR"(Software\Classes\rap.ogg\shell\open)",
        LR"(Software\Classes\rap.m4a\shell\open)",
        LR"(Software\Classes\rap.mka\shell\open)",
        LR"(Software\Classes\rap.alac\shell\open)",
        LR"(Software\Classes\rap.ac3\shell\open)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    SHELL_COMMAND_ENTRIES = {
        LR"(Software\Classes\rap.mp3\shell\open\command)",
        LR"(Software\Classes\rap.flac\shell\open\command)",
        LR"(Software\Classes\rap.opus\shell\open\command)",
        LR"(Software\Classes\rap.aac\shell\open\command)",
        LR"(Software\Classes\rap.wav\shell\open\command)",
        LR"(Software\Classes\rap.ogg\shell\open\command)",
        LR"(Software\Classes\rap.m4a\shell\open\command)",
        LR"(Software\Classes\rap.mka\shell\open\command)",
        LR"(Software\Classes\rap.alac\shell\open\command)",
        LR"(Software\Classes\rap.ac3\shell\open\command)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ac3\UserChoiceLatest)",
};

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_PROGIDS = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ac3\UserChoiceLatest\ProgId)",
};

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_OPEN_WITH_LIST_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ac3\UserChoiceLatest\OpenWithList)",
};

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_OPEN_WITH_PROGIDS_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ac3\UserChoiceLatest\OpenWithProgids)",
};

constexpr wstring_view SHELL_ENTRY =
    LR"(Software\Classes\Directory\shell\Open directory in RAP)";
constexpr wstring_view SHELL_ENTRY_COMMAND =
    LR"(Software\Classes\Directory\shell\Open directory in RAP\command)";

constexpr auto arrayMissingEntry(
    const array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>& arr
) -> bool {
    return ranges::any_of(arr, &wstring_view::empty);
}

constexpr auto arraysMissingEntry() -> bool {
    constexpr array<
        array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>,
        11>
        arr = { EXTENSION_ENTRIES,
                PROG_IDS,
                PROG_ID_ENTRIES,
                ICON_ENTRIES,
                SHELL_ENTRIES,
                SHELL_OPEN_ENTRIES,
                SHELL_COMMAND_ENTRIES,
                EXPLORER_ENTRIES,
                EXPLORER_PROGIDS,
                EXPLORER_OPEN_WITH_LIST_ENTRIES,
                EXPLORER_OPEN_WITH_PROGIDS_ENTRIES };

    return ranges::any_of(arr, &arrayMissingEntry);
}

// Quick hack to check if some extensions were forgotten
static_assert(!arraysMissingEntry(), "Array missing entry!");

auto setRegistryValue(
    const HKEY root,
    const wstring_view entry,
    const wstring_view key,
    const wstring_view value,
    const u32 type
) -> bool {
    HKEY hkey;
    i32 result = RegCreateKeyExW(
        root,
        entry.data(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hkey,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        LOG_ERROR(u"Failed to open/create registry entry: %1. Error: %2"_s
                      .arg(entry)
                      .arg(result));
        return false;
    }

    result = RegSetValueExW(
        hkey,
        key.data(),
        0,
        type,
        ras<const u8*>(value.data()),
        (value.size() + 1) * sizeof(wchar)
    );

    RegCloseKey(hkey);

    if (result != ERROR_SUCCESS) {
        LOG_ERROR(u"Failed to set value for entry: %1. Error: %2"_s.arg(entry)
                      .arg(result));
        return false;
    }

    return true;
}

auto removeRegistryEntry(HKEY root, const wstring_view entry) -> bool {
    const i32 result = RegDeleteTreeW(root, entry.data());

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        LOG_ERROR(u"Failed to delete registry key: %1. Error: %2"_s.arg(entry)
                      .arg(result));
        return false;
    }

    return true;
}

inline void
createFileAssociationsOS(const QString& appPath_, const QString& iconPath_) {
    const wstring_view appPath = { ras<wcstr>(appPath_.utf16()),
                                   as<usize>(appPath_.size()) };

    const wstring_view iconPath = { ras<wcstr>(iconPath_.utf16()),
                                    as<usize>(iconPath_.size()) };

    for (const u8 idx : range(0, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT)) {
        const QStringView extension = ALLOWED_MUSIC_FILE_EXTENSIONS[idx];
        const wstring_view wextension = { ras<wcstr>(extension.utf16()),
                                          as<usize>(extension.size()) };

        const wstring_view progId = PROG_IDS[idx];
        const wstring_view extensionEntry = EXTENSION_ENTRIES[idx];
        const wstring_view progIdEntry = PROG_ID_ENTRIES[idx];
        const wstring_view iconEntry = ICON_ENTRIES[idx];
        const wstring_view shellEntry = SHELL_ENTRIES[idx];
        const wstring_view shellOpenEntry = SHELL_OPEN_ENTRIES[idx];
        const wstring_view shellCommandEntry = SHELL_COMMAND_ENTRIES[idx];

        // Set HKEY_CURRENT_USER\Software\Classes\.ext
        setRegistryValue(
            HKEY_CURRENT_USER,
            extensionEntry,
            DEFAULT_KEY,
            progId,
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext
        setRegistryValue(
            HKEY_CURRENT_USER,
            progIdEntry,
            DEFAULT_KEY,
            wextension,
            REG_SZ
        );

        const wstring iconEntryValue = format(LR"("{}")", iconPath);
        setRegistryValue(
            HKEY_CURRENT_USER,
            iconEntry,
            DEFAULT_KEY,
            iconEntryValue,
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell
        constexpr wstring_view shellEntryValue = L"open";
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellEntry,
            DEFAULT_KEY,
            shellEntryValue,
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open

        constexpr wstring_view shellOpenValue = L"Open in RAP";
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellOpenEntry,
            DEFAULT_KEY,
            shellOpenValue,
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open\command
        const wstring shellCommandValue = format(LR"("{}" "%1")", appPath);
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellCommandEntry,
            DEFAULT_KEY,
            shellCommandValue,
            REG_SZ
        );

        // Set
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\ProgId
        const wstring_view explorerProgIdEntry = EXPLORER_PROGIDS[idx];
        constexpr wstring_view explorerProgIdKey = L"ProgId";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerProgIdEntry,
            explorerProgIdKey,
            progId,
            REG_SZ
        );

        // Set
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\OpenWithList
        const wstring_view explorerOpenWithListEntry =
            EXPLORER_OPEN_WITH_LIST_ENTRIES[idx];
        constexpr wstring_view explorerOpenWithListA = L"a";
        constexpr wstring_view explorerOpenWithListValue = L"rap.exe";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerOpenWithListEntry,
            explorerOpenWithListA,
            explorerOpenWithListValue,
            REG_SZ
        );

        constexpr wstring_view explorerOpenWithListMRU = L"MRUList";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerOpenWithListEntry,
            explorerOpenWithListMRU,
            explorerOpenWithListA,
            REG_SZ
        );

        const wstring_view ExplorerOpenWithProgidsEntry =
            EXPLORER_OPEN_WITH_PROGIDS_ENTRIES[idx];
        setRegistryValue(
            HKEY_CURRENT_USER,
            ExplorerOpenWithProgidsEntry,
            progId,
            DEFAULT_KEY,
            REG_BINARY
        );
    }
}

inline void removeFileAssociationsOS() {
    for (const u8 idx : range(0, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT)) {
        const wstring_view extensionEntry = EXTENSION_ENTRIES[idx];
        const wstring_view progIdEntry = PROG_ID_ENTRIES[idx];
        const wstring_view explorerEntry = EXPLORER_ENTRIES[idx];

        // Delete HKEY_CURRENT_USER\Software\Classes\.ext
        removeRegistryEntry(HKEY_CURRENT_USER, extensionEntry);

        // Delete HKEY_CURRENT_USER\Software\Classes\rap.ext (including subkeys)
        removeRegistryEntry(HKEY_CURRENT_USER, progIdEntry);

        // Delete
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest
        removeRegistryEntry(HKEY_CURRENT_USER, explorerEntry);
    }
}

inline void createContextMenuEntryOS(const QString& appPath_) {
    const wstring_view appPath = { ras<wcstr>(appPath_.utf16()),
                                   as<usize>(appPath_.size()) };

    constexpr wstring_view shellEntryNameKey = L"Open directory in RAP";
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY,
        DEFAULT_KEY,
        shellEntryNameKey,
        REG_SZ
    );

    constexpr wstring_view shellEntryIconKey = L"Icon";
    const wstring shellEntryIconValue = format(LR"("{}",0)", appPath);
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY,
        shellEntryIconKey,
        shellEntryIconValue,
        REG_SZ
    );

    const wstring shellEntryNameValue = format(LR"("{}" "%1")", appPath);
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY_COMMAND,
        DEFAULT_KEY,
        shellEntryNameValue,
        REG_SZ
    );
}

inline void removeContextMenuEntryOS() {
    removeRegistryEntry(HKEY_CURRENT_USER, SHELL_ENTRY);
}

// NOLINTEND(bugprone-suspicious-stringview-data-usage)