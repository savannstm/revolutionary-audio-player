#include "aliases.hpp"
#include "constants.hpp"

#include <QDebug>
#include <windows.h>

auto setRegistryValue(
    const HKEY root,
    const wstring& subKey,
    const wstring& valueName,
    const wstring& valueData,
    const u32 type
) -> bool {
    HKEY hKey;
    i32 result = RegCreateKeyExW(
        root,
        subKey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hKey,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        qDebug() << L"Failed to open/create registry key: " << subKey
                 << L" (Error " << result << L")\n";
        return false;
    }

    result = RegSetValueExW(
        hKey,
        valueName.c_str(),
        0,
        type,
        ras<const u8*>(valueData.c_str()),
        as<u32>((valueData.size() + 1) * sizeof(wchar))
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        qDebug() << L"Failed to set value for key: " << subKey << L" (Error "
                 << result << L")\n";
        return false;
    }

    return true;
}

auto removeRegistryKey(HKEY root, const wstring& subKey) -> bool {
    i32 result = RegDeleteTreeW(root, subKey.c_str());

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        qDebug() << L"Failed to delete registry key: " << subKey << L" (Error "
                 << result << L")\n";
        return false;
    }

    return true;
}

void createFileAssociationsWindows(
    const wstring& appPath,
    const wstring& iconPath
) {
    for (const QStringView ext : ALLOWED_MUSIC_FILE_EXTENSIONS) {
        const wstring extension =
            wstring(ras<const wchar*>(ext.utf16()), ext.length());
        const wstring extKey = LR"(Software\Classes\.)" + extension;
        const wstring progId = L"rap." + extension;

        // Set HKEY_CURRENT_USER\Software\Classes\.ext
        setRegistryValue(HKEY_CURRENT_USER, extKey, L"", progId, REG_SZ);

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext
        const wstring progIdKey = LR"(Software\Classes\)" + progId;
        setRegistryValue(HKEY_CURRENT_USER, progIdKey, L"", extension, REG_SZ);

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\DefaultIcon
        const wstring iconKey = progIdKey + LR"(\DefaultIcon)";
        setRegistryValue(
            HKEY_CURRENT_USER,
            iconKey,
            L"",
            L'"' + iconPath + L'"',
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell
        const wstring shellKey = progIdKey + LR"(\shell)";
        setRegistryValue(HKEY_CURRENT_USER, shellKey, L"", L"open", REG_SZ);

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open
        const wstring openKey = shellKey + LR"(\open)";
        setRegistryValue(
            HKEY_CURRENT_USER,
            openKey,
            L"",
            L"Open in RAP",
            REG_SZ
        );

        // Set HKEY_CURRENT_USER\Software\Classes\rap.ext\shell\open\command
        const wstring commandKey = openKey + LR"(\command)";
        const wstring commandValue = L'"' + appPath + LR"(" "%1")";
        setRegistryValue(
            HKEY_CURRENT_USER,
            commandKey,
            L"",
            commandValue,
            REG_SZ
        );

        // Set
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest\ProgId
        const wstring progIdDir =
            LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.)" +
            extension + LR"(\UserChoiceLatest\ProgId)";
        setRegistryValue(
            HKEY_CURRENT_USER,
            progIdDir,
            L"ProgId",
            progId,
            REG_SZ
        );

        setRegistryValue(
            HKEY_CURRENT_USER,
            LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.)" +
                extension + LR"(\OpenWithList)",
            L"a",
            L"rap.exe",
            REG_SZ
        );

        setRegistryValue(
            HKEY_CURRENT_USER,
            LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.)" +
                extension + LR"(\OpenWithList)",
            L"MRUList",
            L"a",
            REG_SZ
        );

        setRegistryValue(
            HKEY_CURRENT_USER,
            LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.)" +
                extension + LR"(\OpenWithProgids)",
            L"rap.mkv",
            L"",
            REG_BINARY
        );
    }
}

void removeFileAssociationsWindows() {
    for (const QStringView ext : ALLOWED_MUSIC_FILE_EXTENSIONS) {
        const wstring extension =
            wstring(ras<const wchar*>(ext.utf16()), ext.length());
        const wstring extKey = LR"(Software\Classes\.)" + extension;
        const wstring progId = L"rap." + extension;
        const wstring progIdKey = LR"(Software\Classes\)" + progId;

        // Delete HKEY_CURRENT_USER\Software\Classes\.ext
        removeRegistryKey(HKEY_CURRENT_USER, extKey);

        // Delete HKEY_CURRENT_USER\Software\Classes\rap.ext (including subkeys)
        removeRegistryKey(HKEY_CURRENT_USER, progIdKey);

        // Delete
        // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\UserChoiceLatest
        const wstring userChoiceKey =
            LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.)" +
            extension + LR"(\UserChoiceLatest)";
        removeRegistryKey(HKEY_CURRENT_USER, userChoiceKey);
    }
}

void createContextMenuDirectoryEntryWindows(const wstring& appPath) {
    setRegistryValue(
        HKEY_CURRENT_USER,
        LR"(Software\Classes\Directory\shell\Open directory in RAP)",
        L"Open directory in RAP",
        L"",
        REG_SZ
    );

    setRegistryValue(
        HKEY_CURRENT_USER,
        LR"(Software\Classes\Directory\shell\Open directory in RAP)",
        L"Icon",
        L'"' + appPath + LR"(",0)",
        REG_SZ
    );

    setRegistryValue(
        HKEY_CURRENT_USER,
        LR"(Software\Classes\Directory\shell\Open directory in RAP\command)",
        L"Open directory in RAP",
        L'"' + appPath + LR"(" "%1")",
        REG_SZ
    );
}

void removeContextMenuDirectoryEntryWindows() {
    removeRegistryKey(
        HKEY_CURRENT_USER,
        LR"(Software\Classes\Directory\shell\Open directory in RAP)"
    );
}