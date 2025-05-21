#include "aliases.hpp"
#include "constants.hpp"

#include <QDebug>
#include <windows.h>

constexpr wcstr DEFAULT_KEY = L"";

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    EXTENSION_ENTRIES = {
        LR"(Software\Classes\.mp3)",  LR"(Software\Classes\.flac)",
        LR"(Software\Classes\.opus)", LR"(Software\Classes\.aac)",
        LR"(Software\Classes\.wav)",  LR"(Software\Classes\.ogg)",
        LR"(Software\Classes\.m4a)",  LR"(Software\Classes\.mka)",
        LR"(Software\Classes\.alac)",
    };

constexpr array<wstring_view, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> PROG_IDS = {
    L"rap.mp3", L"rap.flac", L"rap.opus", L"rap.aac", L"rap.wav",
    L"rap.ogg", L"rap.m4a",  L"rap.mka",  L"rap.alac"
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> PROG_ID_ENTRIES = {
    LR"(Software\Classes\rap.mp3)",  LR"(Software\Classes\rap.flac)",
    LR"(Software\Classes\rap.opus)", LR"(Software\Classes\rap.aac)",
    LR"(Software\Classes\rap.wav)",  LR"(Software\Classes\rap.ogg)",
    LR"(Software\Classes\rap.m4a)",  LR"(Software\Classes\rap.mka)",
    LR"(Software\Classes\rap.alac)",
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> ICON_ENTRIES = {
    LR"(Software\Classes\rap.mp3\DefaultIcon)",
    LR"(Software\Classes\rap.flac\DefaultIcon)",
    LR"(Software\Classes\rap.opus\DefaultIcon)",
    LR"(Software\Classes\rap.aac\DefaultIcon)",
    LR"(Software\Classes\rap.wav\DefaultIcon)",
    LR"(Software\Classes\rap.ogg\DefaultIcon)",
    LR"(Software\Classes\rap.m4a\DefaultIcon)",
    LR"(Software\Classes\rap.mka\DefaultIcon)",
    LR"(Software\Classes\rap.alac\DefaultIcon)",
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> SHELL_ENTRIES = {
    LR"(Software\Classes\rap.mp3\shell)",
    LR"(Software\Classes\rap.flac\shell)",
    LR"(Software\Classes\rap.opus\shell)",
    LR"(Software\Classes\rap.aac\shell)",
    LR"(Software\Classes\rap.wav\shell)",
    LR"(Software\Classes\rap.ogg\shell)",
    LR"(Software\Classes\rap.m4a\shell)",
    LR"(Software\Classes\rap.mka\shell)",
    LR"(Software\Classes\rap.alac\shell)",
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
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
    };

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
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
    };

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest)"
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_PROGIDS = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\ProgId)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\ProgId)"
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_OPEN_WITH_LIST_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\OpenWithList)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\OpenWithList)"
};

constexpr array<wcstr, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT> EXPLORER_OPEN_WITH_PROGIDS_ENTRIES = {
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp3\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.flac\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.opus\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.aac\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.wav\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ogg\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.m4a\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mka\UserChoiceLatest\OpenWithProgids)",
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.alac\UserChoiceLatest\OpenWithProgids)"
};

auto setRegistryValue(
    const HKEY root,
    wcstr entry,  // NOLINT
    wcstr key,
    wcstr value,
    const u32 valueSize,
    const u32 type
) -> bool {
    HKEY hKey;
    i32 result = RegCreateKeyExW(
        root,
        entry,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hKey,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        qDebug() << format(
            L"Failed to open/create registry entry: {}. Error: {}",
            entry,
            result
        );
        return false;
    }

    result = RegSetValueExW(
        hKey,
        key,
        0,
        type,
        ras<const u8*>(value),
        (valueSize + 1) * sizeof(wchar)
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        qDebug() << format(
            L"Failed to set value for entry: {}. Error: {}",
            entry,
            result
        );
        return false;
    }

    return true;
}

auto removeRegistryEntry(HKEY root, wcstr entry) -> bool {
    i32 result = RegDeleteTreeW(root, entry);

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        qDebug() << format(
            L"Failed to delete registry key: {}. Error: {}",
            entry,
            result
        );
        return false;
    }

    return true;
}

void createFileAssociations(const QString& appPath, const QString& iconPath) {
    for (const u8 idx : range(0, ALLOWED_MUSIC_FILE_EXTENSIONS.size())) {
        const QStringView extension = ALLOWED_MUSIC_FILE_EXTENSIONS[idx];
        const auto* wextension = ras<wcstr>(extension.utf16());

        const wstring_view progId = PROG_IDS[idx];
        const wcstr extensionEntry = EXTENSION_ENTRIES[idx];
        const wcstr progIdEntry = PROG_ID_ENTRIES[idx];
        const wcstr iconEntry = ICON_ENTRIES[idx];
        const wcstr shellEntry = SHELL_ENTRIES[idx];
        const wcstr shellOpenEntry = SHELL_OPEN_ENTRIES[idx];
        const wcstr shellCommandEntry = SHELL_COMMAND_ENTRIES[idx];

        // Set HKEY_CURRENT_USER\Software\Classes\.ext
        setRegistryValue(
            HKEY_CURRENT_USER,
            extensionEntry,
            DEFAULT_KEY,
            progId.data(),
            progId.size(),
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext
        setRegistryValue(
            HKEY_CURRENT_USER,
            progIdEntry,
            DEFAULT_KEY,
            wextension,
            extension.size(),
            REG_SZ
        );

        const wstring iconEntryValue =
            format(LR"("{}")", ras<wcstr>(iconPath.utf16()));
        setRegistryValue(
            HKEY_CURRENT_USER,
            iconEntry,
            DEFAULT_KEY,
            iconEntryValue.data(),
            iconEntryValue.size(),
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell
        constexpr wstring_view shellEntryValue = L"open";
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellEntry,
            DEFAULT_KEY,
            shellEntryValue.data(),
            shellEntryValue.size(),
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open

        constexpr wstring_view shellOpenValue = L"Open in RAP";
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellOpenEntry,
            DEFAULT_KEY,
            shellOpenValue.data(),
            shellOpenValue.size(),
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open\command
        const wstring shellCommandValue =
            format(LR"("{}" "%1")", ras<wcstr>(appPath.utf16()));
        setRegistryValue(
            HKEY_CURRENT_USER,
            shellCommandEntry,
            DEFAULT_KEY,
            shellCommandValue.c_str(),
            shellCommandValue.size(),
            REG_SZ
        );

        // Set
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\ProgId
        const wcstr explorerProgIdEntry = EXPLORER_PROGIDS[idx];
        constexpr wcstr explorerProgIdKey = L"ProgId";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerProgIdEntry,
            explorerProgIdKey,
            progId.data(),
            progId.size(),
            REG_SZ
        );

        // Set
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\OpenWithList
        const wcstr explorerOpenWithListEntry =
            EXPLORER_OPEN_WITH_LIST_ENTRIES[idx];
        constexpr wstring_view explorerOpenWithListA = L"a";
        constexpr wstring_view explorerOpenWithListValue = L"rap.exe";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerOpenWithListEntry,
            explorerOpenWithListA.data(),  // NOLINT
            explorerOpenWithListValue.data(),
            explorerOpenWithListValue.size(),
            REG_SZ
        );

        constexpr wcstr explorerOpenWithListMRU = L"MRUList";
        setRegistryValue(
            HKEY_CURRENT_USER,
            explorerOpenWithListEntry,
            explorerOpenWithListMRU,
            explorerOpenWithListA.data(),
            explorerOpenWithListA.size(),
            REG_SZ
        );

        const wcstr ExplorerOpenWithProgidsEntry =
            EXPLORER_OPEN_WITH_PROGIDS_ENTRIES[idx];
        setRegistryValue(
            HKEY_CURRENT_USER,
            ExplorerOpenWithProgidsEntry,
            progId.data(),  // NOLINT
            DEFAULT_KEY,
            0,
            REG_BINARY
        );
    }
}

void removeFileAssociations() {
    for (const u8 idx : range(0, ALLOWED_MUSIC_FILE_EXTENSIONS.size())) {
        const wcstr extensionEntry = EXTENSION_ENTRIES[idx];
        const wcstr progIdEntry = PROG_ID_ENTRIES[idx];
        const wcstr explorerEntry = EXPLORER_ENTRIES[idx];

        // Delete HKEY_CURRENT_USER\Software\Classes\.ext
        removeRegistryEntry(HKEY_CURRENT_USER, extensionEntry);

        // Delete HKEY_CURRENT_USER\Software\Classes\rap.ext (including subkeys)
        removeRegistryEntry(HKEY_CURRENT_USER, progIdEntry);

        // Delete
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest
        removeRegistryEntry(HKEY_CURRENT_USER, explorerEntry);
    }
}

constexpr wcstr SHELL_ENTRY =
    LR"(Software\Classes\Directory\shell\Open directory in RAP)";
constexpr wcstr SHELL_ENTRY_COMMAND =
    LR"(Software\Classes\Directory\shell\Open directory in RAP\command)";

void createContextMenuDirectoryEntry(const QString& appPath_) {
    const auto* appPath = ras<wcstr>(appPath_.utf16());

    constexpr wcstr shellEntryNameKey = L"Open directory in RAP";
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY,
        shellEntryNameKey,
        DEFAULT_KEY,
        2,
        REG_SZ
    );

    constexpr wcstr shellEntryIconKey = L"Icon";
    const wstring shellEntryIconValue = format(LR"("{}",0)", appPath);
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY,
        shellEntryIconKey,
        shellEntryIconValue.c_str(),
        shellEntryIconValue.size(),
        REG_SZ
    );

    const wstring shellEntryNameValue = format(LR"("{}" "%1")", appPath);
    setRegistryValue(
        HKEY_CURRENT_USER,
        SHELL_ENTRY_COMMAND,
        shellEntryNameKey,
        shellEntryNameValue.c_str(),
        shellEntryNameValue.size(),
        REG_SZ
    );
}

void removeContextMenuDirectoryEntry() {
    removeRegistryEntry(HKEY_CURRENT_USER, SHELL_ENTRY);
}