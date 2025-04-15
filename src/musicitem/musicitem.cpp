
#include "musicitem.hpp"

void MusicItem::setPath(const string& path) {
    trackPath = path;
}

auto MusicItem::getPath() const -> const string& {
    return trackPath;
}