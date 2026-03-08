#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "DockWidget.hpp"
#include "Enums.hpp"
#include "SpectrumVisualizer.hpp"
#include "Visualizer.hpp"

#include <QApplication>
#include <QFile>
#include <QGradient>
#include <QJsonObject>
#include <QVariantList>
#include <miniaudio.h>

struct EqualizerSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> EqualizerSettings;

    QMap<Bands, QMap<QString, QList<i8>>> presets = { { Bands::Ten, {} },
                                                      { Bands::Eighteen, {} },
                                                      { Bands::Thirty, {} } };

    array<i8, THIRTY_BANDS> gains;

    const f32* frequencies = nullptr;

    u16 presetIndex = 0;

    bool enabled = false;
    u8 bandIndex = 2;
    Bands bandCount = Bands::Ten;
};

struct DockWidgetSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> DockWidgetSettings;

    u16 size = 0;
    u16 imageSize = 0;
    DockWidget::Position position = DockWidget::Position::Right;
};

struct SpectrumVisualizerSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> SpectrumVisualizerSettings;

    QGradient::Preset preset = QGradient::MorpheusDen;

    f32 gainFactor = 0.0F;
    u8 peakPadding = 0;

    bool hidden = false;
    Bands bands = Bands::Eighteen;
    SpectrumVisualizer::Mode mode = SpectrumVisualizer::Mode::DBFS;
};

struct CoreSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> CoreSettings;

    // Runtime only
    optional<ma_device_id> outputDeviceID;

    QString settingsDir = qApp->applicationDirPath();
    QString playlistsDir = qApp->applicationDirPath();

    QString lastDir;
    QString outputDevice;

    QString defaultStyle;
    QString applicationStyle;

    QLocale::Language language = QLocale().language();

    u8 volume = MAX_VOLUME;
    i8 currentTab = -1;

    Qt::ColorScheme applicationTheme;

    bool checkForUpdates = true;
    ProgressDisplayMode progressDisplayMode = ProgressDisplayMode::Elapsed;
};

struct ShellIntegrationSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> ShellIntegrationSettings;

    // Enabled file extensions, as bitflags integer.
    Associations enabledAssociations;

    // Open in RAP context menu entry.
    bool contextMenuEntryEnabled = false;
};

struct PlaylistSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> PlaylistSettings;

    ColumnSettingsArray columns = DEFAULT_COLUMN_SETTINGS;
    PlaylistNaming playlistNaming = PlaylistNaming::DirectoryName;

    // Automatically set the playlist background from the track cover.
    bool autoSetBackground = false;

    // Prioritize external (cover.jpg/cover.png) cover over embedded.
    bool prioritizeExternalCover = false;
};

struct VisualizerSettings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> VisualizerSettings;

    bool useRandomPresets = false;
    QString randomPresetDir =
        qApp->applicationDirPath() + u"/visualizer/presets"_qssv;
    QString presetPath;

    u16 meshWidth = Visualizer::DEFAULT_MESH_WIDTH;
    u16 meshHeight = Visualizer::DEFAULT_MESH_HEIGHT;
    u16 fps = 0;
};

struct Settings {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> Settings;
    void save(QFile& file) const;

    CoreSettings core;
    ShellIntegrationSettings shell;
    PlaylistSettings playlist;
    EqualizerSettings equalizer;
    SpectrumVisualizerSettings spectrumVisualizer;
    VisualizerSettings visualizer;
    DockWidgetSettings dockWidget;
};

struct PlaylistObject {
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> PlaylistObject;
    static auto withRowCount(u32 rowCount) -> PlaylistObject;

    ColumnSettingsArray columns;

    QStringList tracks;
    QStringList order;
    QStringList cueFilePaths;
    QVariantList cueOffsets;

    QString tabLabel;
    QString backgroundImagePath;

    QString tabColor;

    f32 backgroundOpacity;
};
