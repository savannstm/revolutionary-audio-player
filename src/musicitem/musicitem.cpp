#include "musicitem.hpp"

MusicItem::MusicItem() {
    setEditable(false);
}

MusicItem::MusicItem(const QString& text) : QStandardItem(text) {
    setEditable(false);
}

void MusicItem::setPath(const QString& path) {
    trackPath = path;
}

auto MusicItem::getPath() const -> const QString& {
    return trackPath;
}