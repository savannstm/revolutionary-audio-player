// NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
// The reason for disabling this lint check is that all string views constructed
// from string literals always include null terminator at end upon calling
// `.data()`.
#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"

#include <windows.h>

constexpr wstring_view DEFAULT_KEY;

constexpr wstring_view EXTENSION_ENTRY_TEMPLATE = LR"(Software\Classes\.{})";
constexpr wstring_view PROG_ID_TEMPLATE = L"rap.{}";
constexpr wstring_view PROG_ID_ENTRY_TEMPLATE = LR"(Software\Classes\rap.{})";
constexpr wstring_view ICON_ENTRY_TEMPLATE =
    LR"(Software\Classes\rap.{}\DefaultIcon)";
constexpr wstring_view SHELL_ENTRY_TEMPLATE =
    LR"(Software\Classes\rap.{}\shell)";
constexpr wstring_view SHELL_OPEN_ENTRY_TEMPLATE =
    LR"(Software\Classes\rap.{}\shell\open)";
constexpr wstring_view SHELL_COMMAND_ENTRY_TEMPLATE =
    LR"(Software\Classes\rap.{}\shell\open\command)";
constexpr wstring_view EXPLORER_ENTRY_TEMPLATE =
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.{}\UserChoiceLatest)";
constexpr wstring_view EXPLORER_PROG_ID_ENTRY_TEMPLATE =
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.{}\UserChoiceLatest\ProgId)";
constexpr wstring_view EXPLORER_OPEN_WITH_LIST_ENTRY_TEMPLATE =
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.{}\UserChoiceLatest\OpenWithList)";
constexpr wstring_view EXPLORER_OPEN_WITH_PROG_ID_ENTRY_TEMPLATE =
    LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.{}\UserChoiceLatest\OpenWithProgids)";

constexpr wstring_view SHELL_ENTRY =
    LR"(Software\Classes\Directory\shell\Open directory in RAP)";
constexpr wstring_view SHELL_ENTRY_COMMAND =
    LR"(Software\Classes\Directory\shell\Open directory in RAP\command)";

inline auto setRegistryValue(
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

inline auto removeRegistryEntry(const HKEY root, const wstring_view entry)
    -> bool {
    const i32 result = RegDeleteTreeW(root, entry.data());

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        LOG_ERROR(u"Failed to delete registry key: %1. Error: %2"_s.arg(entry)
                      .arg(result));
        return false;
    }

    return true;
}

inline void updateFileAssociationsOS(
    const QString& appPath_,
    const QString& iconPath_,
    const Associations associations
) {
    const wstring_view appPath = { ras<wcstr>(appPath_.utf16()),
                                   usize(appPath_.size()) };

    const wstring_view iconPath = { ras<wcstr>(iconPath_.utf16()),
                                    usize(iconPath_.size()) };

    for (const auto [idx, ext] :
         views::enumerate(ALLOWED_PLAYABLE_EXTENSIONS)) {
        const wstring_view wextension = { ras<wcstr>(ext.utf16()),
                                          usize(ext.size()) };

        if (u32(associations & Associations(1 << idx)) != 0) {
            const wstring progID = std::format(PROG_ID_TEMPLATE, wextension);
            const wstring extensionEntry =
                std::format(EXTENSION_ENTRY_TEMPLATE, wextension);
            const wstring progIDEntry =
                std::format(PROG_ID_ENTRY_TEMPLATE, wextension);
            const wstring iconEntry =
                std::format(ICON_ENTRY_TEMPLATE, wextension);
            const wstring shellEntry =
                std::format(SHELL_ENTRY_TEMPLATE, wextension);
            const wstring shellOpenEntry =
                std::format(SHELL_OPEN_ENTRY_TEMPLATE, wextension);
            const wstring shellCommandEntry =
                std::format(SHELL_COMMAND_ENTRY_TEMPLATE, wextension);
            const wstring explorerProgIDEntry =
                std::format(EXPLORER_PROG_ID_ENTRY_TEMPLATE, wextension);
            const wstring explorerOpenWithListEntry =
                std::format(EXPLORER_OPEN_WITH_LIST_ENTRY_TEMPLATE, wextension);
            const wstring explorerOpenWithProgIDEntry = std::format(
                EXPLORER_OPEN_WITH_PROG_ID_ENTRY_TEMPLATE,
                wextension
            );

            // Set HKEY_CURRENT_USER\Software\Classes\.ext
            setRegistryValue(
                HKEY_CURRENT_USER,
                extensionEntry,
                DEFAULT_KEY,
                progID,
                REG_SZ
            );

            // Set HKEY_CURRENT_USER\Software\Classes\rap.ext
            setRegistryValue(
                HKEY_CURRENT_USER,
                progIDEntry,
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
            constexpr wstring_view explorerProgIdKey = L"ProgId";
            setRegistryValue(
                HKEY_CURRENT_USER,
                explorerProgIDEntry,
                explorerProgIdKey,
                progID,
                REG_SZ
            );

            // Set
            // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\OpenWithList
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

            setRegistryValue(
                HKEY_CURRENT_USER,
                explorerOpenWithProgIDEntry,
                progID,
                DEFAULT_KEY,
                REG_BINARY
            );
        } else {
            const wstring extensionEntry =
                std::format(EXTENSION_ENTRY_TEMPLATE, wextension);
            const wstring progIDEntry =
                std::format(PROG_ID_ENTRY_TEMPLATE, wextension);
            const wstring explorerEntry =
                std::format(EXPLORER_ENTRY_TEMPLATE, wextension);

            // Delete HKEY_CURRENT_USER\Software\Classes\.ext
            removeRegistryEntry(HKEY_CURRENT_USER, extensionEntry);

            // Delete HKEY_CURRENT_USER\Software\Classes\rap.ext (including
            // subkeys)
            removeRegistryEntry(HKEY_CURRENT_USER, progIDEntry);

            // Delete
            // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest
            removeRegistryEntry(HKEY_CURRENT_USER, explorerEntry);
        }
    }
}

inline void createContextMenuEntryOS(const QString& appPath_) {
    const wstring_view appPath = { ras<wcstr>(appPath_.utf16()),
                                   usize(appPath_.size()) };

    const wstring shellEntryNameKey =
        QObject::tr("Open directory in RAP").toStdWString();
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

[[nodiscard]] inline auto getExistingAssociationsOS() -> Associations {
    HKEY hkey;
    Associations associations = Associations::None;

    for (const auto [idx, ext] :
         views::enumerate(ALLOWED_PLAYABLE_EXTENSIONS)) {
        const u32 result = RegOpenKeyExW(
            HKEY_CURRENT_USER,
            std::format(
                PROG_ID_ENTRY_TEMPLATE,
                wstring_view(ras<wcstr>(ext.utf16()), ext.size())
            )
                .c_str(),
            0,
            KEY_READ,
            &hkey
        );

        if (result == ERROR_SUCCESS) {
            RegCloseKey(hkey);
            associations |= Associations(1 << idx);
        }
    }

    return associations;
}

[[nodiscard]] inline auto shellEntryExistsOS() -> bool {
    HKEY hkey;

    const u32 result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        SHELL_ENTRY.data(),
        0,
        KEY_READ,
        &hkey
    );

    if (result == ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return true;
    }

    return false;
}

// NOLINTEND(bugprone-suspicious-stringview-data-usage)