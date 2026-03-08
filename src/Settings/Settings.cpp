#include "Settings.hpp"

#include "Constants.hpp"
#include "Enums.hpp"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStyle>

template <typename T>
auto toJsonArray(const T& container) -> QJsonArray {
    QJsonArray array;

    for (const auto& item : container) {
        array.append(QJsonValue::fromVariant(QVariant::fromValue(item)));
    }

    return array;
}

template <typename T, u16 N>
auto fromJsonArray(const QJsonArray& jsonArray) -> array<T, N> {
    array<T, N> result{};
    const u16 count = min<u16>(jsonArray.size(), N);

    for (const u16 idx : range<u16>(0, count)) {
        result[idx] = jsonArray[idx].toVariant().value<T>();
    }

    return result;
}

auto toStringList(const QJsonArray& jsonArray) -> QStringList {
    QStringList list;
    list.reserve(jsonArray.size());

    for (const auto value : jsonArray) {
        list.append(value.toString());
    }

    return list;
}

auto EqualizerSettings::fromJSON(const QJsonObject& obj) -> EqualizerSettings {
    QMap<Bands, QMap<QString, QList<i8>>> presets = { { Bands::Ten, {} },
                                                      { Bands::Eighteen, {} },
                                                      { Bands::Thirty, {} } };

    const QJsonObject presetsObj = obj["presets"_L1].toObject();

    for (const QLatin1StringView bandKey : { "10"_L1, "18"_L1, "30"_L1 }) {
        if (presetsObj.contains(bandKey)) {
            const QJsonObject bandPresetsObj = presetsObj[bandKey].toObject();
            QMap<QString, QList<i8>> bandPresets;

            for (const QString& presetName : bandPresetsObj.keys()) {
                const auto gainsArray = bandPresetsObj[presetName].toArray();

                QList<i8> gainsVec;
                gainsVec.reserve(gainsArray.size());

                for (const auto val : gainsArray) {
                    gainsVec.emplace_back(i8(val.toInt()));
                }

                bandPresets[presetName] = std::move(gainsVec);
            }

            presets[Bands(bandKey.toInt())] = std::move(bandPresets);
        }
    }

    return {
        .presets = presets,
        .presetIndex = u8(obj["presetIndex"_L1].toInt()),
        .enabled = obj["enabled"_L1].toBool(),
        .bandIndex = u8(obj["bandIndex"_L1].toInt()),
    };
}

auto EqualizerSettings::toJSON() const -> QJsonObject {
    QJsonObject presetsObj;

    for (const auto& [bandKey, bandPresets] : presets.toStdMap()) {
        QJsonObject bandPresetsObj;

        for (const auto& [presetName, gainsVec] : bandPresets.toStdMap()) {
            QJsonArray gainsArray;

            for (const auto val : gainsVec) {
                gainsArray.append(val);
            }

            bandPresetsObj[presetName] = gainsArray;
        }

        presetsObj[QString::number(u8(bandKey))] = bandPresetsObj;
    }

    return {
        { u"enabled"_s, enabled },
        { u"bandIndex"_s, bandIndex },
        { u"presetIndex"_s, presetIndex },
        { u"presets"_s, presetsObj },
    };
}

auto DockWidgetSettings::fromJSON(const QJsonObject& obj)
    -> DockWidgetSettings {
    return {
        .size = u16(obj["size"_L1].toInt()),
        .imageSize = u16(obj["imageSize"_L1].toInt()),
        .position = DockWidget::Position(obj["position"_L1].toInt()),
    };
}

auto DockWidgetSettings::toJSON() const -> QJsonObject {
    return {
        { u"size"_s, size },
        { u"position"_s, u8(position) },
        { u"imageSize"_s, imageSize },
    };
}

auto SpectrumVisualizerSettings::fromJSON(const QJsonObject& obj)
    -> SpectrumVisualizerSettings {
    return {
        .preset = QGradient::Preset(obj["preset"_L1].toInt()),
        .gainFactor = f32(obj["gainFactor"_L1].toDouble()),
        .peakPadding = u8(obj["peakPadding"_L1].toInt()),
        .hidden = obj["hidden"_L1].toBool(),
        .bands = Bands(obj["bands"_L1].toInt()),
        .mode = SpectrumVisualizer::Mode(obj["mode"_L1].toInt()),
    };
}

auto SpectrumVisualizerSettings::toJSON() const -> QJsonObject {
    return {
        { u"gainFactor"_s, gainFactor }, { u"peakPadding"_s, peakPadding },
        { u"hidden"_s, hidden },         { u"mode"_s, u8(mode) },
        { u"bands"_s, u8(bands) },       { u"preset"_s, preset },
    };
}

auto VisualizerSettings::fromJSON(const QJsonObject& obj)
    -> VisualizerSettings {
    return {
        .useRandomPresets = obj["useRandomPresets"_L1].toBool(),
        .randomPresetDir = obj["randomPresetDir"_L1].toString(),
        .presetPath = obj["presetPath"_L1].toString(),
        .meshWidth = u16(obj["meshWidth"_L1].toInt()),
        .meshHeight = u16(obj["meshHeight"_L1].toInt()),
        .fps = u16(obj["fps"_L1].toInt()),
    };
}

auto VisualizerSettings::toJSON() const -> QJsonObject {
    return {
        { u"useRandomPresets"_s, useRandomPresets },
        { u"randomPresetDir"_s, randomPresetDir },
        { u"meshWidth"_s, meshWidth },
        { u"meshHeight"_s, meshHeight },
        { u"fps"_s, fps },
        { u"presetPath"_s, presetPath },
    };
}

auto CoreSettings::fromJSON(const QJsonObject& obj) -> CoreSettings {
    ma_context context;
    ma_context_config config = ma_context_config_init();
    span<ma_device_info> playbackDevices;
    ma_context_init(nullptr, 0, &config, &context);

    ma_device_info* playbackInfos = nullptr;
    u32 playbackCount = 0;

    if (ma_context_get_devices(
            &context,
            &playbackInfos,
            &playbackCount,
            nullptr,
            nullptr
        ) != MA_SUCCESS) {
        qCritical() << "Failed to get audio playback devices"_L1;
    }

    playbackDevices = span<ma_device_info>(playbackInfos, playbackCount);
    optional<ma_device_id> outputDeviceID;
    QString outputDevice = obj["outputDevice"_L1].toString();

    for (const auto& device : playbackDevices) {
        if (outputDevice == device.name) {
            outputDeviceID = device.id;
        }
    }

    ma_context_uninit(&context);

    return {
        .outputDeviceID = outputDeviceID,

        .lastDir = obj["lastDir"_L1].toString(),
        .outputDevice = std::move(outputDevice),

        .applicationStyle = obj["applicationStyle"_L1].toString(),

        .language = QLocale::Language(obj["language"_L1].toBool()),

        .volume = u8(obj["volume"_L1].toInt()),
        .currentTab = i8(obj["currentTab"_L1].toInt()),

        .applicationTheme = Qt::ColorScheme(obj["applicationTheme"_L1].toInt()),

        .checkForUpdates = obj["checkForUpdates"_L1].toBool(true),
        .progressDisplayMode =
            ProgressDisplayMode(obj["progressDisplayMode"_L1].toInt()),
    };
}

auto CoreSettings::toJSON() const -> QJsonObject {
    return {
        { u"lastDir"_s, lastDir },
        { u"outputDevice"_s, outputDevice },
        { u"language"_s, language },
        { u"volume"_s, volume },
        { u"currentTab"_s, currentTab },
        { u"applicationStyle"_s, applicationStyle },
        { u"applicationTheme"_s, u8(applicationTheme) },
        { u"checkForUpdates"_s, checkForUpdates },
        { u"progressDisplayMode"_s, u8(progressDisplayMode) },
    };
}

auto ShellIntegrationSettings::fromJSON(const QJsonObject& obj)
    -> ShellIntegrationSettings {
    return {
        .enabledAssociations =
            Associations(obj["enabledAssociations"_L1].toInt()),
        .contextMenuEntryEnabled = obj["contextMenuEntryEnabled"_L1].toBool(),
    };
}

auto ShellIntegrationSettings::toJSON() const -> QJsonObject {
    return {
        { u"enabledAssociations"_s, i32(enabledAssociations) },
        { u"contextMenuEntryEnabled"_s, contextMenuEntryEnabled },
    };
}

auto PlaylistSettings::fromJSON(const QJsonObject& obj) -> PlaylistSettings {
    const QJsonArray columnsArray = obj["columns"_L1].toArray();
    ColumnSettingsArray columns;

    for (const auto [idx, val] : views::enumerate(columnsArray)) {
        const QJsonObject settings = val.toObject();

        columns[idx] = { .index = u8(settings["index"_L1].toInt()),
                         .hidden = settings["hidden"_L1].toBool() };
    }

    return {
        .columns = columns,
        .playlistNaming = PlaylistNaming(obj["playlistNaming"_L1].toInt()),
        .autoSetBackground = obj["autoSetBackground"_L1].toBool(),
        .prioritizeExternalCover = obj["autoSetBackground"_L1].toBool(),
    };
}

auto PlaylistSettings::toJSON() const -> QJsonObject {
    QJsonObject json;
    QJsonArray columns;

    for (const auto [index, hidden] : this->columns) {
        QJsonObject columnSettings;
        columnSettings["index"_L1] = index;
        columnSettings["hidden"_L1] = hidden;
        columns.append(columnSettings);
    }

    return {
        { u"columns"_s, columns },
        { u"playlistNaming"_s, u8(playlistNaming) },
        { u"autoSetBackground"_s, autoSetBackground },
        { u"prioritizeExternalCover"_s, prioritizeExternalCover },
    };
}

// Settings implementation
auto Settings::fromJSON(const QJsonObject& obj) -> Settings {
    return {
        .core = CoreSettings::fromJSON(obj["core"_L1].toObject()),
        .shell = ShellIntegrationSettings::fromJSON(obj["shell"_L1].toObject()),
        .playlist = PlaylistSettings::fromJSON(obj["playlist"_L1].toObject()),
        .equalizer =
            EqualizerSettings::fromJSON(obj["equalizer"_L1].toObject()),
        .spectrumVisualizer = SpectrumVisualizerSettings::fromJSON(
            obj["spectrumVisualizer"_L1].toObject()
        ),
        .visualizer =
            VisualizerSettings::fromJSON(obj["visualizer"_L1].toObject()),
        .dockWidget =
            DockWidgetSettings::fromJSON(obj["dockWidget"_L1].toObject()),
    };
}

auto Settings::toJSON() const -> QJsonObject {
    return {
        { u"core"_s, core.toJSON() },
        { u"shell"_s, shell.toJSON() },
        { u"playlist"_s, playlist.toJSON() },
        { u"equalizer"_s, equalizer.toJSON() },
        { u"dockWidget"_s, dockWidget.toJSON() },
        { u"spectrumVisualizer"_s, spectrumVisualizer.toJSON() },
        { u"visualizer"_s, visualizer.toJSON() },
    };
}

void Settings::save(QFile& file) const {
    file.write(QJsonDocument(toJSON()).toJson(QJsonDocument::Compact));
}

auto PlaylistObject::withRowCount(const u32 rowCount) -> PlaylistObject {
    QStringList tracks;
    QStringList order;
    QStringList cueFilePaths;
    QVariantList cueOffsets;

    tracks.reserve(rowCount);
    order.reserve(rowCount);
    cueOffsets.reserve(rowCount);
    cueFilePaths.reserve(rowCount);

    return {
        .tracks = std::move(tracks),
        .order = std::move(order),
        .cueFilePaths = std::move(cueFilePaths),
        .cueOffsets = std::move(cueOffsets),
    };
};

auto PlaylistObject::fromJSON(const QJsonObject& obj) -> PlaylistObject {
    const QJsonArray columnsArray = obj["columns"_L1].toArray();
    ColumnSettingsArray columns;

    for (const auto [idx, val] : views::enumerate(columnsArray)) {
        const QJsonObject settings = val.toObject();

        columns[idx] = { .index = u8(settings["index"_L1].toInt()),
                         .hidden = settings["hidden"_L1].toBool() };
    }

    QStringList tracks = toStringList(obj["tracks"_L1].toArray());

    QVariantList cueOffsets;
    cueOffsets.reserve(tracks.size());

    const QJsonArray cueOffsetsArray = obj["cueOffsets"_L1].toArray();

    for (const auto element : cueOffsetsArray) {
        cueOffsets.append(element.toVariant());
    }

    return {
        .columns = columns,
        .tracks = std::move(tracks),
        .order = toStringList(obj["order"_L1].toArray()),
        .cueFilePaths = toStringList(obj["cueFilePaths"_L1].toArray()),

        .cueOffsets = std::move(cueOffsets),

        .tabLabel = obj["tabLabel"_L1].toString(),
        .backgroundImagePath = obj["backgroundImagePath"_L1].toString(),

        .tabColor = obj["tabColor"_L1].toString(),
        .backgroundOpacity = f32(obj["backgroundOpacity"_L1].toDouble()),
    };
}

auto PlaylistObject::toJSON() const -> QJsonObject {
    QJsonArray columns;

    for (const auto [index, hidden] : this->columns) {
        QJsonObject columnSettings;
        columnSettings["index"_L1] = index;
        columnSettings["hidden"_L1] = hidden;
        columns.append(columnSettings);
    }

    return {
        { u"columns"_s, columns },

        { u"tracks"_s, QJsonArray::fromStringList(tracks) },
        { u"order"_s, QJsonArray::fromStringList(order) },
        { u"cueOffsets"_s, toJsonArray(cueOffsets) },
        { u"cueFilePaths"_s, toJsonArray(cueFilePaths) },

        { u"tabLabel"_s, tabLabel },
        { u"backgroundImagePath"_s, backgroundImagePath },

        { u"tabColor"_s, tabColor },
        { u"backgroundOpacity"_s, backgroundOpacity },
    };
}
