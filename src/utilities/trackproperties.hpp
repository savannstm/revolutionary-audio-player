#pragma once

#include <QObject>

inline auto trackPropertiesLabels() -> QStringList {
    return { QString(),
             QObject::tr("Title"),
             QObject::tr("Artist"),
             QObject::tr("Track Number"),
             QObject::tr("Album"),
             QObject::tr("Album Artist"),
             QObject::tr("Genre"),
             QObject::tr("Year"),
             QObject::tr("Duration"),
             QObject::tr("Composer"),
             QObject::tr("BPM"),
             QObject::tr("Language"),
             QObject::tr("Disc Number"),
             QObject::tr("Comment"),
             QObject::tr("Publisher"),
             QObject::tr("Bitrate"),
             QObject::tr("Sample Rate"),
             QObject::tr("Channels"),
             QObject::tr("Format"),
             QObject::tr("Path"),
             QObject::tr("Order") };
}