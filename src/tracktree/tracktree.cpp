#include "tracktree.hpp"

#include "extractmetadata.hpp"
#include "rapidhasher.hpp"

#include <QMouseEvent>

TrackTree::TrackTree(QWidget* parent) : QTreeView(parent) {
    setHeader(musicHeader);
    setModel(musicModel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIndentation(1);
    setSortingEnabled(true);

    auto* sModel = selectionModel();

    connect(
        sModel,
        &QItemSelectionModel::selectionChanged,
        [=, this](const QItemSelection&, const QItemSelection& deselected) {
        const QModelIndex requiredIndex = currentIndex();

        if (deselected.contains(requiredIndex)) {
            sModel->select(
                requiredIndex,
                QItemSelectionModel::Select | QItemSelectionModel::Rows
            );
        }
    }
    );
}

void TrackTree::mouseDoubleClickEvent(QMouseEvent* event) {
    const QModelIndex newIndex = indexAt(event->pos());
    if (!newIndex.isValid()) {
        return;
    }

    emit trackSelected(index.row(), newIndex.row());
    index = newIndex;
    setCurrentIndex(newIndex);
    QTreeView::mouseDoubleClickEvent(event);
}

void TrackTree::setCurrentIndex(const QModelIndex& newIndex) {
    index = newIndex;
    QTreeView::setCurrentIndex(index);
}

auto TrackTree::rowMetadata(const u16 row) const -> MetadataMap {
    MetadataMap result;

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        const TrackProperty headerProperty = musicModel->trackProperty(column);
        const QModelIndex index = musicModel->index(row, column);

        if (index.isValid()) {
            result.emplace(headerProperty, musicModel->data(index).toString());
        }
    }

    return result;
}

void TrackTree::addFile(const QString& filePath) {
    const MetadataMap metadata = extractMetadata(filePath);

    if (metadata.empty()) {
        return;
    }

    const u16 row = musicModel->rowCount();

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        const auto headerProperty =
            as<TrackProperty>(musicModel->trackProperty(column));
        auto* item = new MusicItem();

        if (headerProperty == TrackNumber) {
            QString number;

            for (const auto& [idx, chr] :
                 views::enumerate(metadata[headerProperty])) {
                if (idx == 0 && chr == '0') {
                    continue;
                }

                if (!chr.isDigit()) {
                    break;
                }

                number.append(chr);
            }

            item->setData(number.toInt(), Qt::EditRole);
        } else {
            item->setText(metadata[headerProperty]);
        }

        musicModel->setItem(row, column, item);
    }
}

void TrackTree::sortByPath() {
    for (u8 column = TRACK_PROPERTY_COUNT - 1; column >= 0; column--) {
        if (musicModel->trackProperty(column) == Path) {
            sortByColumn(column, Qt::SortOrder::AscendingOrder);
            break;
        }
    }
}

void TrackTree::fillTable(const QStringList& paths) {
    for (const QString& path : paths) {
        const QFileInfo info(path);

        if (info.isDir()) {
            QDirIterator iterator(path);
            fillTable(iterator);
            continue;
        }

        if (musicModel->contains(path)) {
            continue;
        }

        addFile(path);
    }

    resizeColumnsToContents();
    sortByPath();
}

void TrackTree::fillTable(QDirIterator& iterator) {
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo entry = iterator.fileInfo();
        const QString path = entry.filePath();

        if (musicModel->contains(path)) {
            continue;
        }

        for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
            if (path.endsWith(extension)) {
                addFile(path);
            }
        }
    }

    resizeColumnsToContents();
    sortByPath();
};

void TrackTree::resizeColumnsToContents() {
    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        resizeColumnToContents(column);
    }
}

auto TrackTree::deselect(const i32 index) -> QModelIndex {
    clearSelection();

    const QModelIndex modelIndex = currentIndex();

    if (index != -1) {
        musicModel->item(index, 0)->setData(QString(), Qt::DecorationRole);
    } else {
        if (modelIndex.isValid()) {
            musicModel->item(modelIndex.row(), 0)
                ->setData(QString(), Qt::DecorationRole);
        }
    }

    setCurrentIndex(musicModel->index(-1, -1));
    return modelIndex;
}