
#include "musicitem.hpp"

MusicItem::MusicItem() {
    setEditable(false);
}

MusicItem::MusicItem(const QString& text) : QStandardItem(text) {
    setEditable(false);
}

void MusicItem::setPath(const string& path) {
    trackPath = path;
}

auto MusicItem::getPath() const -> const string& {
    return trackPath;
}