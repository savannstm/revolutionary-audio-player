#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"

#include <QApplication>
#include <QFile>
#include <QGradient>
#include <QJsonArray>
#include <QJsonObject>
#include <miniaudio.h>

template <typename T>
auto toJsonArray(const T& container) -> QJsonArray;

template <typename T, u16 N>
auto fromJsonArray(const QJsonArray& jsonArray) -> array<T, N>;

auto toStringList(const QJsonArray& jsonArray) -> QStringList;

class EqualizerSettings {
   public:
    EqualizerSettings() = default;
    explicit EqualizerSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    QMap<Bands, QMap<QString, QVector<i8>>> presets = { { Bands::Ten, {} },
                                                        { Bands::Eighteen, {} },
                                                        { Bands::Thirty, {} } };

    array<i8, usize(Bands::Thirty)> gains;

    const f32* frequencies = nullptr;

    u16 presetIndex = 0;

    bool enabled = false;
    u8 bandIndex = 2;
    Bands bandCount = Bands::Ten;
};

class DockWidgetSettings {
   public:
    explicit DockWidgetSettings() = default;
    explicit DockWidgetSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    u16 size = 0;
    u16 imageSize = 0;
    DockWidgetPosition position = DockWidgetPosition::Right;
};

class PeakVisualizerSettings {
   public:
    explicit PeakVisualizerSettings() = default;
    explicit PeakVisualizerSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    QGradient::Preset preset = QGradient::MorpheusDen;

    bool hidden = false;
    Bands bands = Bands::Eighteen;
    PeakVisualizerMode mode = PeakVisualizerMode::DBFS;
};

class CoreSettings {
   public:
    explicit CoreSettings() = default;
    explicit CoreSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    // Runtime only
    optional<ma_device_id> outputDeviceID;

    QString settingsDir = QApplication::applicationDirPath();
    QString playlistsDir = QApplication::applicationDirPath();

    QString lastDir;
    QString outputDevice;

    QString defaultStyle;
    QString applicationStyle;

    QLocale::Language language = QLocale().language();

    u8 volume = MAX_VOLUME;
    i8 currentTab = -1;

    Qt::ColorScheme applicationTheme;
};

class ShellIntegrationSettings {
   public:
    explicit ShellIntegrationSettings() = default;
    explicit ShellIntegrationSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    // Enabled file extensions, as bitflags integer.
    Associations enabledAssociations;

    // Open in RAP context menu entry.
    bool contextMenuEntryEnabled = false;
};

class PlaylistSettings {
   public:
    explicit PlaylistSettings() = default;
    explicit PlaylistSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    array<TrackProperty, TRACK_PROPERTY_COUNT> defaultColumns =
        DEFAULT_COLUMN_PROPERTIES;
    PlaylistNaming playlistNaming = PlaylistNaming::DirectoryName;

    // Automatically set the playlist background from the track cover.
    bool autoSetBackground = false;

    // Prioritize external (cover.jpg/cover.png) cover over embedded.
    bool prioritizeExternalCover = false;
};

class VisualizerSettings {
   public:
    explicit VisualizerSettings() = default;
    explicit VisualizerSettings(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    u16 meshWidth = DEFAULT_MESH_WIDTH;
    u16 meshHeight = DEFAULT_MESH_HEIGHT;
    u16 fps = 0;
    QString presetPath;
};

class Settings {
   public:
    explicit Settings() = default;
    explicit Settings(const QJsonObject& settingsObject);

    [[nodiscard]] auto stringify() const -> QJsonObject;
    void save(QFile& file) const;

    CoreSettings core;
    ShellIntegrationSettings shell;
    PlaylistSettings playlist;
    EqualizerSettings equalizer;
    PeakVisualizerSettings peakVisualizer;
    VisualizerSettings visualizer;
    DockWidgetSettings dockWidget;
};

class PlaylistObject {
   public:
    explicit PlaylistObject(const u16 rowCount) {
        tracks.reserve(rowCount);
        order.reserve(rowCount);
        cueOffsets.reserve(rowCount);
        cueFilePaths.reserve(rowCount);
    };

    explicit PlaylistObject(const QJsonObject& obj);

    [[nodiscard]] auto stringify() const -> QJsonObject;

    array<TrackProperty, TRACK_PROPERTY_COUNT> defaultColumns =
        DEFAULT_COLUMN_PROPERTIES;

    QStringList tracks;
    QStringList order;
    QStringList cueFilePaths;
    QVariantList cueOffsets;

    QString label;
    QString backgroundImagePath;

    QString color;
};
