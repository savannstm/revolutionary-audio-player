#include "Settings.hpp"

#include "Enums.hpp"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
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

    for (const u16 idx : range(0, count)) {
        result[idx] = jsonArray[idx].toVariant().value<T>();
    }

    return result;
}

auto toStringList(const QJsonArray& jsonArray) -> QStringList {
    QStringList list;
    list.reserve(jsonArray.size());

    for (const auto& value : jsonArray) {
        list.append(value.toString());
    }

    return list;
}

// EqualizerSettings implementation
EqualizerSettings::EqualizerSettings(const QJsonObject& obj) {
    enabled = obj[u"enabled"_qsv].toBool();
    bandIndex = obj[u"bandIndex"_qsv].toInt();
    presetIndex = obj[u"presetIndex"_qsv].toInt();

    const QJsonObject presetsObj = obj[u"presets"_qsv].toObject();

    for (const QStringView bandKey : { u"10"_qsv, u"18"_qsv, u"30"_qsv }) {
        if (presetsObj.contains(bandKey)) {
            const QJsonObject bandPresetsObj = presetsObj[bandKey].toObject();
            QMap<QString, QVector<i8>> bandPresets;

            for (const QString& presetName : bandPresetsObj.keys()) {
                const auto gainsArray = bandPresetsObj[presetName].toArray();

                QVector<i8> gainsVec;
                gainsVec.reserve(gainsArray.size());

                for (const auto& val : gainsArray) {
                    gainsVec.emplace_back(i8(val.toInt()));
                }

                bandPresets[presetName] = std::move(gainsVec);
            }

            presets[Bands(bandKey.toInt())] = std::move(bandPresets);
        }
    }
}

auto EqualizerSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"enabled"_qsv] = enabled;
    json[u"bandIndex"_qsv] = bandIndex;
    json[u"presetIndex"_qsv] = presetIndex;

    QJsonObject presetsObj;

    for (const auto& [bandKey, bandPresets] : presets.toStdMap()) {
        QJsonObject bandPresetsObj;

        for (const auto& [presetName, gainsVec] : bandPresets.toStdMap()) {
            QJsonArray gainsArray;

            for (const auto& val : gainsVec) {
                gainsArray.append(val);
            }

            bandPresetsObj[presetName] = gainsArray;
        }

        presetsObj[QString::number(u8(bandKey))] = bandPresetsObj;
    }

    json[u"presets"_qsv] = presetsObj;

    return json;
}

// DockWidgetSettings implementation
DockWidgetSettings::DockWidgetSettings(const QJsonObject& obj) {
    position = DockWidgetPosition(obj[u"position"_qsv].toInt());
    size = obj[u"size"_qsv].toInt();
    imageSize = obj[u"imageSize"_qsv].toInt();
}

auto DockWidgetSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"size"_qsv] = size;
    json[u"position"_qsv] = u8(position);
    json[u"imageSize"_qsv] = imageSize;

    return json;
}

PeakVisualizerSettings::PeakVisualizerSettings(const QJsonObject& obj) {
    hidden = obj[u"hidden"_qsv].toBool();
    mode = PeakVisualizerMode(obj[u"mode"_qsv].toInt());
    bands = Bands(obj[u"bands"_qsv].toInt());
    preset = QGradient::Preset(obj[u"preset"_qsv].toInt());
}

auto PeakVisualizerSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"hidden"_qsv] = hidden;
    json[u"mode"_qsv] = u8(mode);
    json[u"bands"_qsv] = u8(bands);
    json[u"preset"_qsv] = preset;

    return json;
}

VisualizerSettings::VisualizerSettings(const QJsonObject& obj) {
    meshWidth = obj[u"meshWidth"_qsv].toInt();
    meshHeight = obj[u"meshHeight"_qsv].toInt();
    fps = obj[u"fps"_qsv].toInt();
    presetPath = obj[u"presetPath"_qsv].toString();
}

auto VisualizerSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"meshWidth"_qsv] = meshWidth;
    json[u"meshHeight"_qsv] = meshHeight;
    json[u"fps"_qsv] = fps;
    json[u"presetPath"_qsv] = presetPath;

    return json;
}

CoreSettings::CoreSettings(const QJsonObject& obj) {
    lastDir = obj[u"lastDir"_qsv].toString();
    outputDevice = obj[u"outputDevice"_qsv].toString();

    ma_context context;
    ma_context_config config = ma_context_config_init();
    span<ma_device_info> playbackDevices;
    ma_context_init(nullptr, -1, &config, &context);

    ma_device_info* playbackInfos = nullptr;
    u32 playbackCount = 0;

    if (ma_context_get_devices(
            &context,
            &playbackInfos,
            &playbackCount,
            nullptr,
            nullptr
        ) != MA_SUCCESS) {
        // TODO
    }

    playbackDevices = span<ma_device_info>(playbackInfos, playbackCount);

    for (const auto& device : playbackDevices) {
        if (outputDevice == device.name) {
            outputDeviceID = device.id;
        }
    }

    ma_context_uninit(&context);

    language = QLocale::Language(obj[u"language"_qsv].toBool());

    volume = obj[u"volume"_qsv].toInt();
    currentTab = i8(obj[u"currentTab"_qsv].toInt());

    applicationStyle = obj[u"applicationStyle"_qsv].toString();
    applicationTheme = Qt::ColorScheme(obj[u"applicationTheme"_qsv].toInt());
}

auto CoreSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"lastDir"_qsv] = lastDir;
    json[u"outputDevice"_qsv] = outputDevice;

    json[u"language"_qsv] = language;

    json[u"volume"_qsv] = volume;
    json[u"currentTab"_qsv] = currentTab;

    json[u"applicationStyle"_qsv] = applicationStyle;
    json[u"applicationTheme"_qsv] = u8(applicationTheme);

    return json;
}

ShellIntegrationSettings::ShellIntegrationSettings(const QJsonObject& obj) {
    enabledAssociations = Associations(obj[u"enabledAssociations"_qsv].toInt());
    contextMenuEntryEnabled = obj[u"contextMenuEntryEnabled"_qsv].toBool();
}

auto ShellIntegrationSettings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"enabledAssociations"_qsv] = i32(enabledAssociations);
    json[u"contextMenuEntryEnabled"_qsv] = contextMenuEntryEnabled;

    return json;
}

PlaylistSettings::PlaylistSettings(const QJsonObject& obj) {
    QJsonArray columns = obj[u"defaultColumns"_qsv].toArray();

    for (const u8 idx : range(0, TRACK_PROPERTY_COUNT - 1)) {
        defaultColumns[idx] = TrackProperty(columns[idx].toInt());
    }

    playlistNaming = PlaylistNaming(obj[u"playlistNaming"_qsv].toInt());
    autoSetBackground = obj[u"autoSetBackground"_qsv].toBool();
    prioritizeExternalCover = obj[u"autoSetBackground"_qsv].toBool();
}

auto PlaylistSettings::stringify() const -> QJsonObject {
    QJsonObject json;
    QJsonArray columns;

    for (const u8 idx : range(0, TRACK_PROPERTY_COUNT - 1)) {
        columns.append(u8(defaultColumns[idx]));
    }

    json[u"defaultColumns"_qsv] = columns;
    json[u"playlistNaming"_qsv] = u8(playlistNaming);
    json[u"autoSetBackground"_qsv] = autoSetBackground;
    json[u"prioritizeExternalCover"_qsv] = prioritizeExternalCover;

    return json;
}

// Settings implementation
Settings::Settings(const QJsonObject& settingsObject) {
    core = CoreSettings(settingsObject[u"core"_qsv].toObject());

    shell = ShellIntegrationSettings(settingsObject[u"shell"_qsv].toObject());

    playlist = PlaylistSettings(settingsObject[u"playlist"_qsv].toObject());

    equalizer = EqualizerSettings(settingsObject[u"equalizer"_qsv].toObject());

    dockWidget =
        DockWidgetSettings(settingsObject[u"dockWidget"_qsv].toObject());

    peakVisualizer = PeakVisualizerSettings(
        settingsObject[u"peakVisualizer"_qsv].toObject()
    );

    visualizer =
        VisualizerSettings(settingsObject[u"visualizer"_qsv].toObject());
}

auto Settings::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"core"_qsv] = core.stringify();
    json[u"shell"_qsv] = shell.stringify();
    json[u"playlist"_qsv] = playlist.stringify();
    json[u"equalizer"_qsv] = equalizer.stringify();
    json[u"dockWidget"_qsv] = dockWidget.stringify();
    json[u"peakVisualizer"_qsv] = peakVisualizer.stringify();
    json[u"visualizer"_qsv] = visualizer.stringify();

    return json;
}

void Settings::save(QFile& file) const {
    file.write(QJsonDocument(stringify()).toJson(QJsonDocument::Compact));
}

PlaylistObject::PlaylistObject(const QJsonObject& obj) {
    defaultColumns = fromJsonArray<TrackProperty, TRACK_PROPERTY_COUNT>(
        obj[u"defaultColumns"_qsv].toArray()
    );

    tracks = toStringList(obj[u"tracks"_qsv].toArray());
    order = toStringList(obj[u"order"_qsv].toArray());
    cueFilePaths = toStringList(obj[u"cueFilePaths"_qsv].toArray());

    cueOffsets.reserve(tracks.size());

    const QJsonArray cueOffsetsArray = obj[u"cueOffsets"_qsv].toArray();

    for (const auto& element : cueOffsetsArray) {
        cueOffsets.append(element.toVariant());
    }

    label = obj[u"label"_qsv].toString();
    backgroundImagePath = obj[u"backgroundImagePath"_qsv].toString();

    color = obj[u"color"_qsv].toString();
}

auto PlaylistObject::stringify() const -> QJsonObject {
    QJsonObject json;

    json[u"defaultColumns"_qsv] = toJsonArray(defaultColumns);

    json[u"tracks"_qsv] = QJsonArray::fromStringList(tracks);
    json[u"order"_qsv] = QJsonArray::fromStringList(order);
    json[u"cueOffsets"_qsv] = toJsonArray(cueOffsets);
    json[u"cueFilePaths"_qsv] = toJsonArray(cueFilePaths);

    json[u"label"_qsv] = label;
    json[u"backgroundImagePath"_qsv] = backgroundImagePath;

    json[u"color"_qsv] = color;

    return json;
}