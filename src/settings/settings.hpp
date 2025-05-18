#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"

#include <QApplication>
#include <QAudioDevice>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMediaDevices>
#include <QStyle>

template <typename T>
inline auto toJsonArray(const T& container) -> QJsonArray {
    QJsonArray array;

    for (const auto& item : container) {
        array.append(QJsonValue::fromVariant(QVariant::fromValue(item)));
    }

    return array;
}

template <typename T, u16 N>
inline auto fromJsonArray(const QJsonArray& jsonArray) -> array<T, N> {
    array<T, N> result;
    const u16 count = min<u16>(jsonArray.size(), N);

    for (const u16 idx : range(0, count)) {
        result[idx] = jsonArray[idx].toVariant().value<T>();
    }

    return result;
}

inline auto toStringList(const QJsonArray& jsonArray) -> QStringList {
    QStringList list;
    list.reserve(jsonArray.size());

    for (const auto& value : jsonArray) {
        list.append(value.toString());
    }

    return list;
}

class TabObject {
   public:
    explicit TabObject() = default;

    explicit TabObject(const QJsonObject& obj) {
        label = obj["label"].toString();
        backgroundImagePath = obj["backgroundImagePath"].toString();
        tracks = toStringList(obj["tracks"].toArray());
        columnProperties = fromJsonArray<TrackProperty, TRACK_PROPERTY_COUNT>(
            obj["columnProperties"].toArray()
        );
        columnStates = fromJsonArray<bool, TRACK_PROPERTY_COUNT>(
            obj["columnStates"].toArray()
        );
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
        QJsonObject json;
        json["label"] = label;
        json["backgroundImagePath"] = backgroundImagePath;
        json["tracks"] = QJsonArray::fromStringList(tracks);
        json["columnProperties"] = toJsonArray(columnProperties);
        json["columnStates"] = toJsonArray(columnStates);
        return json;
    }

    QString label;
    QString backgroundImagePath;
    QStringList tracks;
    array<TrackProperty, TRACK_PROPERTY_COUNT> columnProperties;
    array<bool, TRACK_PROPERTY_COUNT> columnStates;
};

class EqualizerSettings {
   public:
    EqualizerSettings() = default;

    explicit EqualizerSettings(const QJsonObject& obj) {
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
                            gainsVec.emplace_back(as<i8>(val.toInt()));
                        }

                        bandPresets[presetName] = std::move(gainsVec);
                    }

                    presets[bandKey.toInt()] = std::move(bandPresets);
                }
            }
        }
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
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

    bool enabled = false;
    u8 bandIndex = 2;
    u16 presetIndex = 0;
    QMap<u16, QMap<QString, QVector<i8>>> presets = { { TEN_BANDS, {} },
                                                      { EIGHTEEN_BANDS, {} },
                                                      { THIRTY_BANDS, {} } };
};

class SettingsFlags {
   public:
    explicit SettingsFlags() {
        const QString style = QApplication::style()->name();
        const auto item = ranges::find(APPLICATION_STYLES, style);

        if (item != APPLICATION_STYLES.end()) {
            applicationStyle =
                as<Style>(ranges::distance(APPLICATION_STYLES.begin(), item));
        }
    };

    explicit SettingsFlags(const QJsonObject& obj) {
        applicationStyle = as<Style>(obj["applicationStyle"].toInt());
        dragNDropMode = as<DragDropMode>(obj["dragNDropMode"].toInt());
        playlistNaming = as<PlaylistNaming>(obj["playlistNaming"].toInt());
        contextMenuEntryEnabled = obj["contextMenuEntryEnabled"].toBool();
        fileAssociationsEnabled = obj["fileAssociationsEnabled"].toBool();
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
        QJsonObject json;
        json["applicationStyle"] = applicationStyle;
        json["dragNDropMode"] = dragNDropMode;
        json["playlistNaming"] = playlistNaming;
        json["contextMenuEntryEnabled"] = contextMenuEntryEnabled;
        json["fileAssociationsEnabled"] = fileAssociationsEnabled;
        return json;
    }

    Style applicationStyle;
    DragDropMode dragNDropMode = DragDropMode::CreateNewPlaylist;
    PlaylistNaming playlistNaming = PlaylistNaming::DirectoryName;

    // Self-explanatory.
    bool fileAssociationsEnabled = false;

    // Open in RAP context menu entry.
    bool contextMenuEntryEnabled = false;

    // Automatically set the playlist background from the track cover.
    bool autoSetBackground = false;

    // Prioritize external (cover.jpg/cover.png) cover over embedded.
    bool prioritizeExternal = false;
};

class DockWidgetSettings {
   public:
    explicit DockWidgetSettings() = default;

    explicit DockWidgetSettings(const QJsonObject& obj) {
        position = as<DockWidgetPosition>(obj["position"].toInt());
        size = obj["size"].toInt();
        imageSize = obj["imageSize"].toInt();
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
        QJsonObject json;
        json["size"] = size;
        json["position"] = position;
        json["imageSize"] = imageSize;
        return json;
    }

    DockWidgetPosition position = DockWidgetPosition::Right;
    u16 size = 0;
    u16 imageSize = 0;
};

class Settings {
   public:
    explicit Settings() = default;

    explicit Settings(const QJsonObject& settingsObject) {
        lastOpenedDirectory = settingsObject["lastOpenedDirectory"].toString();

        if (settingsObject.contains("volume")) {
            volume = settingsObject["volume"].toInt();
        }

        if (settingsObject.contains("currentTab")) {
            currentTab = as<i8>(settingsObject["currentTab"].toInt());
        }

        if (settingsObject.contains("language")) {
            language =
                as<QLocale::Language>(settingsObject["language"].toInt());
        }

        if (settingsObject.contains("equalizerSettings")) {
            equalizerSettings = EqualizerSettings(
                settingsObject["equalizerSettings"].toObject()
            );
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
            const QString requiredName =
                settingsObject["outputDevice"].toString();

            for (const auto& device : outputDevices) {
                if (device.description() == requiredName) {
                    outputDevice = device;
                }
            }
        }
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
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

        return json;
    }

    void save(QFile& file) const {
        file.write(QJsonDocument(stringify()).toJson(QJsonDocument::Compact));
        file.close();
    }

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
