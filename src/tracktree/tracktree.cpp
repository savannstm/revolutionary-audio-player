#include "tracktree.hpp"

#include "extractmetadata.hpp"
#include "rapidhasher.hpp"

#include <QDirIterator>
#include <QMouseEvent>
#include <QThreadPool>

TrackTree::TrackTree(QWidget* parent) : QTreeView(parent) {
    setHeader(musicHeader);
    setModel(musicModel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIndentation(1);
    setSortingEnabled(true);

    connect(
        selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &TrackTree::reselectCurrentElement
    );
    connect(this, &TrackTree::finishedFilling, this, &TrackTree::postFill);
    connect(this, &TrackTree::metadataReceived, this, &TrackTree::addFile);
}

void TrackTree::reselectCurrentElement(
    const QItemSelection& /*unused*/,
    const QItemSelection& deselected
) {
    const QModelIndex requiredIndex = currentIndex();

    if (deselected.contains(requiredIndex)) {
        selectionModel()->select(
            requiredIndex,
            QItemSelectionModel::Select | QItemSelectionModel::Rows
        );
    }
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

auto TrackTree::rowMetadata(const u16 row) const
    -> HashMap<TrackProperty, QString> {
    HashMap<TrackProperty, QString> result;

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        const TrackProperty headerProperty = musicModel->trackProperty(column);
        const QModelIndex index = musicModel->index(row, column);

        if (index.isValid()) {
            result.emplace(headerProperty, musicModel->data(index).toString());
        }
    }

    return result;
}

void TrackTree::addFile(
    const QString& filePath,
    const HashMap<TrackProperty, QString>& metadata
) {
    const u16 row = musicModel->rowCount();

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        const auto headerProperty = musicModel->trackProperty(column);
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

        musicModel->setItem(row, column, item, headerProperty == Path);
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
    QThreadPool::globalInstance()->start([=, this] {
        for (const QString& path : paths) {
            QFileInfo info(path);

            if (info.isDir()) {
                QDirIterator iterator(
                    path,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories
                );

                QStringList filePaths;
                filePaths.reserve(MINIMUM_TRACK_COUNT);

                while (iterator.hasNext()) {
                    iterator.next();
                    filePaths.append(iterator.fileInfo().filePath());
                }

                fillTable(filePaths);
                continue;
            }

            if (musicModel->contains(path)) {
                continue;
            }

            const auto metadata = extractMetadata(path);
            if (metadata.empty()) {
                continue;
            }

            emit metadataReceived(path, metadata);
        }

        emit finishedFilling();
    });
}

void TrackTree::postFill() {
    resizeColumnsToContents();
    sortByPath();
}

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