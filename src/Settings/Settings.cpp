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

// TabObject implementation
TabObject::TabObject(const QJsonObject& obj) {
    label = obj["label"].toString();
    backgroundImagePath = obj["backgroundImagePath"].toString();
    tracks = toStringList(obj["tracks"].toArray());
    order = toStringList(obj["order"].toArray());
    columnProperties = fromJsonArray<TrackProperty, TRACK_PROPERTY_COUNT>(
        obj["columnProperties"].toArray()
    );
    columnStates = fromJsonArray<bool, TRACK_PROPERTY_COUNT>(
        obj["columnStates"].toArray()
    );
}

auto TabObject::stringify() const -> QJsonObject {
    QJsonObject json;
    json["label"] = label;
    json["backgroundImagePath"] = backgroundImagePath;
    json["tracks"] = QJsonArray::fromStringList(tracks);
    json["order"] = QJsonArray::fromStringList(order);
    json["columnProperties"] = toJsonArray(columnProperties);
    json["columnStates"] = toJsonArray(columnStates);
    return json;
}

// EqualizerSettings implementation
EqualizerSettings::EqualizerSettings(const QJsonObject& obj) {
    enabled = obj["enabled"].toBool();
    bandIndex = obj["bandIndex"].toInt();
    presetIndex = obj["presetIndex"].toInt();

    if (obj.contains("presets")) {
        const QJsonObject presetsObj = obj["presets"].toObject();

        for (const QString& bandKey : { "10", "18", "30" }) {
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
    json["enabled"] = enabled;
    json["bandIndex"] = bandIndex;
    json["presetIndex"] = presetIndex;

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

    json["presets"] = presetsObj;
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
    applicationStyle = as<Style>(obj["applicationStyle"].toInt());
    playlistNaming = as<PlaylistNaming>(obj["playlistNaming"].toInt());
    contextMenuEntryEnabled = obj["contextMenuEntryEnabled"].toBool();
    fileAssociationsEnabled = obj["fileAssociationsEnabled"].toBool();
    prioritizeExternalCover = obj["prioritizeExternalCover"].toBool();
    autoSetBackground = obj["autoSetBackground"].toBool();
}

auto SettingsFlags::stringify() const -> QJsonObject {
    QJsonObject json;
    json["applicationStyle"] = applicationStyle;
    json["playlistNaming"] = playlistNaming;
    json["contextMenuEntryEnabled"] = contextMenuEntryEnabled;
    json["fileAssociationsEnabled"] = fileAssociationsEnabled;
    json["prioritizeExternalCover"] = prioritizeExternalCover;
    json["autoSetBackground"] = autoSetBackground;
    return json;
}

// DockWidgetSettings implementation
DockWidgetSettings::DockWidgetSettings(const QJsonObject& obj) {
    position = as<DockWidgetPosition>(obj["position"].toInt());
    size = obj["size"].toInt();
    imageSize = obj["imageSize"].toInt();
}

auto DockWidgetSettings::stringify() const -> QJsonObject {
    QJsonObject json;
    json["size"] = size;
    json["position"] = position;
    json["imageSize"] = imageSize;
    return json;
}

PeakVisualizerSettings::PeakVisualizerSettings(const QJsonObject& obj) {
    hidden = obj["hidden"].toBool();
    mode = as<PeakVisualizerMode>(obj["mode"].toInt());
    preset = as<QGradient::Preset>(obj["preset"].toInt());
}

auto PeakVisualizerSettings::stringify() const -> QJsonObject {
    QJsonObject json;
    json["hidden"] = hidden;
    json["mode"] = mode;
    json["preset"] = preset;
    return json;
}

// Settings implementation
Settings::Settings(const QJsonObject& settingsObject) {
    lastOpenedDirectory = settingsObject["lastOpenedDirectory"].toString();

    if (settingsObject.contains("volume")) {
        volume = settingsObject["volume"].toInt();
    }

    if (settingsObject.contains("currentTab")) {
        currentTab = i8(settingsObject["currentTab"].toInt());
    }

    if (settingsObject.contains("language")) {
        language = as<QLocale::Language>(settingsObject["language"].toInt());
    }

    if (settingsObject.contains("equalizerSettings")) {
        equalizerSettings =
            EqualizerSettings(settingsObject["equalizerSettings"].toObject());
    }

    if (settingsObject.contains("tabs")) {
        const QJsonArray array = settingsObject["tabs"].toArray();
        tabs.reserve(array.size());

        for (const auto& tab : array) {
            tabs.emplace_back(tab.toObject());
        }
    }

    if (settingsObject.contains("flags")) {
        flags = SettingsFlags(settingsObject["flags"].toObject());
    }

    if (settingsObject.contains("dockWidget")) {
        dockWidgetSettings =
            DockWidgetSettings(settingsObject["dockWidget"].toObject());
    }

    if (settingsObject.contains("outputDevice")) {
        const auto outputDevices = QMediaDevices::audioOutputs();
        const QString requiredName = settingsObject["outputDevice"].toString();

        for (const auto& device : outputDevices) {
            if (device.description() == requiredName) {
                outputDevice = device;
            }
        }
    }

    if (settingsObject.contains("peakVisualizerSettings")) {
        peakVisualizerSettings = PeakVisualizerSettings(
            settingsObject["peakVisualizerSettings"].toObject()
        );
    }
}

auto Settings::stringify() const -> QJsonObject {
    QJsonObject json;
    json["equalizerSettings"] = equalizerSettings.stringify();
    json["lastOpenedDirectory"] = lastOpenedDirectory;

    QJsonArray array;
    for (const auto& tab : tabs) {
        array.append(tab.stringify());
    }

    json["tabs"] = array;
    json["volume"] = volume;
    json["currentTab"] = currentTab;
    json["language"] = language;
    json["flags"] = flags.stringify();
    json["dockWidget"] = dockWidgetSettings.stringify();
    json["outputDevice"] = outputDevice.description();
    json["peakVisualizerSettings"] = peakVisualizerSettings.stringify();

    return json;
}

void Settings::save(QFile& file) const {
    file.write(QJsonDocument(stringify()).toJson(QJsonDocument::Compact));
    file.close();
}
