#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"

#include <QAudioDevice>
#include <QFile>
#include <QJsonArray>
#include <QMediaDevices>

template <typename T>
auto toJsonArray(const T& container) -> QJsonArray;

template <typename T, u16 N>
auto fromJsonArray(const QJsonArray& jsonArray) -> array<T, N>;

auto toStringList(const QJsonArray& jsonArray) -> QStringList;

class TabObject {
   public:
    explicit TabObject() = default;
    explicit TabObject(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    QString label;
    QString backgroundImagePath;
    QStringList tracks;
    QStringList customNumbers;
    array<TrackProperty, TRACK_PROPERTY_COUNT> columnProperties;
    array<bool, TRACK_PROPERTY_COUNT> columnStates;
};

class EqualizerSettings {
   public:
    EqualizerSettings() = default;
    explicit EqualizerSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    bool enabled = false;
    u8 bandIndex = 2;
    u16 presetIndex = 0;
    QMap<u16, QMap<QString, QVector<i8>>> presets = { { TEN_BANDS, {} },
                                                      { EIGHTEEN_BANDS, {} },
                                                      { THIRTY_BANDS, {} } };
};

class SettingsFlags {
   public:
    explicit SettingsFlags();
    explicit SettingsFlags(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    Style applicationStyle;
    PlaylistNaming playlistNaming = PlaylistNaming::DirectoryName;

    // Self-explanatory.
    bool fileAssociationsEnabled = false;

    // Open in RAP context menu entry.
    bool contextMenuEntryEnabled = false;

    // Automatically set the playlist background from the track cover.
    bool autoSetBackground = false;

    // Prioritize external (cover.jpg/cover.png) cover over embedded.
    bool prioritizeExternalCover = false;
};

class DockWidgetSettings {
   public:
    explicit DockWidgetSettings() = default;
    explicit DockWidgetSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    DockWidgetPosition position = DockWidgetPosition::Right;
    u16 size = 0;
    u16 imageSize = 0;
};

class Settings {
   public:
    explicit Settings() = default;
    explicit Settings(const QJsonObject& settingsObject);

    [[nodiscard]] auto stringify() const -> QJsonObject;
    void save(QFile& file) const;

    EqualizerSettings equalizerSettings;
    QString lastOpenedDirectory;
    vector<TabObject> tabs;
    u8 volume = MAX_VOLUME;
    i8 currentTab = -1;
    QLocale::Language language = QLocale().language();
    SettingsFlags flags;
    DockWidgetSettings dockWidgetSettings;
    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
};