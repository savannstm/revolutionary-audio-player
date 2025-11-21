#pragma once

#include "Enums.hpp"

#include <QDataStream>
#include <QString>

struct EqualizerPreset {
    QString name;
    Bands bandCount;

    array<i8, THIRTY_BANDS> bands;
};

inline auto checkStream(const QDataStream& stream) -> optional<QString> {
    if (stream.status() != QDataStream::Ok) {
        return u"Unable to parse preset. Error %1 occurred."_s.arg(
            stream.status()
        );
    }

    return nullopt;
}

// TODO: improve
inline auto parsePreset(const QByteArray& presetData)
    -> result<EqualizerPreset, QString> {
    EqualizerPreset preset{};

    auto byteStream = QDataStream(presetData);
    byteStream >> preset.name >> preset.bandCount;

    for (const u8 band : range(0, u8(preset.bandCount))) {
        byteStream >> preset.bands[band];
    }

    if (byteStream.status() != QDataStream::Ok) {
        return err(u"Unable to parse preset. Error %1 occurred."_s.arg(
            byteStream.status()
        ));
    }

    return preset;
}

inline auto serializePreset(const EqualizerPreset& preset) -> QByteArray {
    QByteArray presetData;
    presetData.reserve(sizeof(EqualizerPreset));

    auto byteStream =
        QDataStream(&presetData, QDataStream::WriteOnly | QDataStream::Text);

    byteStream << preset.name << preset.bandCount;

    for (const i8 gain : span(preset.bands.data(), u8(preset.bandCount))) {
        byteStream << gain;
    }

    return presetData;
}