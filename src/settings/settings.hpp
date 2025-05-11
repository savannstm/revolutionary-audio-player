#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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

    for (u16 i = 0; i < count; i++) {
        result[i] = jsonArray[i].toVariant().value<T>();
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
    explicit EqualizerSettings() {
        gains.fill(0);
        memcpy(
            frequencies.data(),
            TEN_BAND_FREQUENCIES.data(),
            TEN_BANDS * SAMPLE_SIZE
        );
    };

    explicit EqualizerSettings(const QJsonObject& obj) {
        enabled = obj["enabled"].toBool();
        bandIndex = obj["bandIndex"].toInt();

        if (obj.contains("gains")) {
            gains = fromJsonArray<i8, THIRTY_BANDS>(obj["gains"].toArray());
        }

        if (obj.contains("frequencies")) {
            frequencies =
                fromJsonArray<f32, THIRTY_BANDS>(obj["frequencies"].toArray());
        }
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
        QJsonObject json;
        json["enabled"] = enabled;
        json["bandIndex"] = bandIndex;
        json["gains"] = toJsonArray(gains);
        json["frequencies"] = toJsonArray(frequencies);
        return json;
    }

    bool enabled = false;
    u8 bandIndex = 2;
    array<i8, THIRTY_BANDS> gains;
    array<f32, THIRTY_BANDS> frequencies;
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
    }

    [[nodiscard]] auto stringify() const -> QJsonObject {
        QJsonObject json;
        json["applicationStyle"] = applicationStyle;
        json["dragNDropMode"] = dragNDropMode;
        json["playlistNaming"] = playlistNaming;
        json["contextMenuEntryEnabled"] = contextMenuEntryEnabled;
        return json;
    }

    // TODO: Automatically set the background from track cover
    // TODO: Prioritize embedded/external
    Style applicationStyle;
    DragDropMode dragNDropMode = DragDropMode::CreateNewPlaylist;
    PlaylistNaming playlistNaming = PlaylistNaming::DirectoryName;
    bool contextMenuEntryEnabled = false;
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
};
