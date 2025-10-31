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
    array<T, N> result;
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

    if (obj.contains("presets")) {
        const QJsonObject presetsObj = obj[u"presets"_qsv].toObject();

        for (const QStringView bandKey : { u"10"_qsv, u"18"_qsv, u"30"_qsv }) {
            if (presetsObj.contains(bandKey)) {
                const QJsonObject bandPresetsObj =
                    presetsObj[bandKey].toObject();
                QMap<QString, QVector<i8>> bandPresets;

                for (const QString& presetName : bandPresetsObj.keys()) {
                    const auto gainsArray =
                        bandPresetsObj[presetName].toArray();

                    QVector<i8> gainsVec;
                    gainsVec.reserve(gainsArray.size());

                    for (const auto& val : gainsArray) {
                        gainsVec.emplace_back(i8(val.toInt()));
                    }

                    bandPresets[presetName] = std::move(gainsVec);
                }

                presets[bandKey.toInt()] = std::move(bandPresets);
            }
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

        presetsObj[QString::number(bandKey)] = bandPresetsObj;
    }

    json[u"presets"_qsv] = presetsObj;
    return json;
}

// SettingsFlags implementation
SettingsFlags::SettingsFlags() {
    const QString style = QApplication::style()->name();
    const auto item = ranges::find(APPLICATION_STYLES, style);

    if (item != APPLICATION_STYLES.end()) {
        applicationStyle =
            as<Style>(ranges::distance(APPLICATION_STYLES.begin(), item));
    }
}

SettingsFlags::SettingsFlags(const QJsonObject& obj) {
    applicationStyle = as<Style>(obj[u"applicationStyle"_qsv].toInt());
    playlistNaming = as<PlaylistNaming>(obj[u"playlistNaming"_qsv].toInt());
    contextMenuEntryEnabled = obj[u"contextMenuEntryEnabled"_qsv].toBool();
    fileAssociationsEnabled = obj[u"fileAssociationsEnabled"_qsv].toBool();
    prioritizeExternalCover = obj[u"prioritizeExternalCover"_qsv].toBool();
    autoSetBackground = obj[u"autoSetBackground"_qsv].toBool();
}

auto SettingsFlags::stringify() const -> QJsonObject {
    QJsonObject json;
    json[u"applicationStyle"_qsv] = applicationStyle;
    json[u"playlistNaming"_qsv] = playlistNaming;
    json[u"contextMenuEntryEnabled"_qsv] = contextMenuEntryEnabled;
    json[u"fileAssociationsEnabled"_qsv] = fileAssociationsEnabled;
    json[u"prioritizeExternalCover"_qsv] = prioritizeExternalCover;
    json[u"autoSetBackground"_qsv] = autoSetBackground;
    return json;
}

// DockWidgetSettings implementation
DockWidgetSettings::DockWidgetSettings(const QJsonObject& obj) {
    position = as<DockWidgetPosition>(obj[u"position"_qsv].toInt());
    size = obj[u"size"_qsv].toInt();
    imageSize = obj[u"imageSize"_qsv].toInt();
}

auto DockWidgetSettings::stringify() const -> QJsonObject {
    QJsonObject json;
    json[u"size"_qsv] = size;
    json[u"position"_qsv] = position;
    json[u"imageSize"_qsv] = imageSize;
    return json;
}

PeakVisualizerSettings::PeakVisualizerSettings(const QJsonObject& obj) {
    hidden = obj[u"hidden"_qsv].toBool();
    mode = as<PeakVisualizerMode>(obj[u"mode"_qsv].toInt());
    preset = as<QGradient::Preset>(obj[u"preset"_qsv].toInt());
}

auto PeakVisualizerSettings::stringify() const -> QJsonObject {
    QJsonObject json;
    json[u"hidden"_qsv] = hidden;
    json[u"mode"_qsv] = mode;
    json[u"preset"_qsv] = preset;
    return json;
}

// Settings implementation
Settings::Settings(const QJsonObject& settingsObject) {
    lastOpenedDirectory = settingsObject[u"lastOpenedDirectory"_qsv].toString();

    if (settingsObject.contains(u"volume"_qsv)) {
        volume = settingsObject[u"volume"_qsv].toInt();
    }

    if (settingsObject.contains(u"currentTab"_qsv)) {
        currentTab = i8(settingsObject[u"currentTab"_qsv].toInt());
    }

    if (settingsObject.contains(u"language"_qsv)) {
        language =
            as<QLocale::Language>(settingsObject[u"language"_qsv].toInt());
    }

    if (settingsObject.contains(u"equalizerSettings"_qsv)) {
        equalizerSettings = EqualizerSettings(
            settingsObject[u"equalizerSettings"_qsv].toObject()
        );
    }

    if (settingsObject.contains(u"flags"_qsv)) {
        flags = SettingsFlags(settingsObject[u"flags"_qsv].toObject());
    }

    if (settingsObject.contains(u"dockWidget"_qsv)) {
        dockWidgetSettings =
            DockWidgetSettings(settingsObject[u"dockWidget"_qsv].toObject());
    }

    if (settingsObject.contains(u"outputDevice"_qsv)) {
        outputDevice = settingsObject[u"outputDevice"_qsv].toString();

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
            ) != MA_SUCCESS) {}

        playbackDevices = span<ma_device_info>(playbackInfos, playbackCount);

        for (const auto& device : playbackDevices) {
            if (outputDevice == device.name) {
                outputDeviceID = device.id;
            }
        }

        ma_context_uninit(&context);
    }

    if (settingsObject.contains(u"peakVisualizerSettings"_qsv)) {
        peakVisualizerSettings = PeakVisualizerSettings(
            settingsObject[u"peakVisualizerSettings"_qsv].toObject()
        );
    }
}

auto Settings::stringify() const -> QJsonObject {
    QJsonObject json;
    json[u"equalizerSettings"_qsv] = equalizerSettings.stringify();
    json[u"lastOpenedDirectory"_qsv] = lastOpenedDirectory;

    json[u"volume"_qsv] = volume;
    json[u"currentTab"_qsv] = currentTab;
    json[u"language"_qsv] = language;
    json[u"flags"_qsv] = flags.stringify();
    json[u"dockWidget"_qsv] = dockWidgetSettings.stringify();
    json[u"outputDevice"_qsv] = outputDevice;
    json[u"peakVisualizerSettings"_qsv] = peakVisualizerSettings.stringify();

    return json;
}

void Settings::save(QFile& file) const {
    file.write(QJsonDocument(stringify()).toJson(QJsonDocument::Compact));
    file.close();
}

PlaylistObject::PlaylistObject(const QJsonObject& obj) {
    label = obj[u"label"_qsv].toString();
    backgroundImagePath = obj[u"backgroundImagePath"_qsv].toString();
    tracks = toStringList(obj[u"tracks"_qsv].toArray());
    order = toStringList(obj[u"order"_qsv].toArray());
    columnProperties = fromJsonArray<TrackProperty, TRACK_PROPERTY_COUNT>(
        obj[u"columnProperties"_qsv].toArray()
    );
    columnStates = fromJsonArray<bool, TRACK_PROPERTY_COUNT>(
        obj[u"columnStates"_qsv].toArray()
    );
    cueFilePaths = toStringList(obj[u"cueFilePaths"_qsv].toArray());

    cueOffsets.reserve(tracks.size());

    const auto cueOffsetsArray = obj[u"cueOffsets"_qsv].toArray();

    for (const auto& element : cueOffsetsArray) {
        cueOffsets.append(element.toVariant());
    }
}

auto PlaylistObject::stringify() const -> QJsonObject {
    QJsonObject json;
    json[u"label"_qsv] = label;
    json[u"backgroundImagePath"_qsv] = backgroundImagePath;
    json[u"tracks"_qsv] = QJsonArray::fromStringList(tracks);
    json[u"order"_qsv] = QJsonArray::fromStringList(order);
    json[u"columnProperties"_qsv] = toJsonArray(columnProperties);
    json[u"columnStates"_qsv] = toJsonArray(columnStates);
    json[u"cueOffsets"_qsv] = toJsonArray(cueOffsets);
    json[u"cueFilePaths"_qsv] = toJsonArray(cueFilePaths);
    return json;
}