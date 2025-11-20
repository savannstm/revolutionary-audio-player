#include "TrackTree.hpp"

#include "Constants.hpp"
#include "DurationConversions.hpp"
#include "Enums.hpp"
#include "ExtractMetadata.hpp"
#include "Logger.hpp"
#include "OptionMenu.hpp"
#include "TrackProperties.hpp"
#include "TrackTreeItem.hpp"

#include <QApplication>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QThreadPool>

TrackTree::TrackTree(QWidget* const parent) : QTreeView(parent) {
    setHeader(musicHeader);
    setModel(trackTreeModel);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIndentation(1);
    setSortingEnabled(true);
    setDragDropMode(QAbstractItemView::DragDropMode::DragDrop);

    connect(
        selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &TrackTree::reselectCurrentElement
    );

    connect(
        musicHeader,
        &TrackTreeHeader::headerPressed,
        this,
        &TrackTree::handleHeaderPress
    );

    connect(
        musicHeader,
        &TrackTreeHeader::sortIndicatorChanged,
        this,
        &TrackTree::resetSorting
    );
}

void TrackTree::reselectCurrentElement(
    const QItemSelection& /* unused */,
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

void TrackTree::mouseDoubleClickEvent(QMouseEvent* const event) {
    const QModelIndex newIndex = indexAt(event->pos());
    if (!newIndex.isValid()) {
        return;
    }

    if (newIndex.column() != u8(TrackProperty::Order)) {
        emit trackSelected(index.row(), newIndex.row());
        index = newIndex;
        setCurrentIndex(newIndex);
    }

    QTreeView::mouseDoubleClickEvent(event);
}

void TrackTree::mousePressEvent(QMouseEvent* const event) {
    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        draggedRow = UINT16_MAX;
        clearSelection();
    }

    const Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    const bool shiftHeld = (mods & Qt::ShiftModifier) != 0;
    const bool controlHeld = (mods & Qt::ControlModifier) != 0;

    if (shiftHeld || controlHeld) {
        setDragEnabled(false);
        setAcceptDrops(false);
        setDropIndicatorShown(false);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
    } else {
        draggedRow = index.row();
        setDragEnabled(true);
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setSelectionMode(QAbstractItemView::SingleSelection);
    }

    QTreeView::mousePressEvent(event);
}

void TrackTree::startDrag(const Qt::DropActions supportedActions) {
    auto byteArray = QByteArray();
    byteArray.resize(sizeof(u16));
    memcpy(byteArray.data(), &draggedRow, sizeof(u16));

    auto* const mimeData = new QMimeData();
    // CRAZY hack to not override other methods
    mimeData->setData(u"application/x-qabstractitemmodeldatalist"_s, byteArray);

    auto* const drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(supportedActions, defaultDropAction());
}

void TrackTree::dropEvent(QDropEvent* const event) {
    const QModelIndex targetIndex = indexAt(event->position().toPoint());
    if (!targetIndex.isValid()) {
        return;
    }

    const QRect rect = visualRect(targetIndex);
    const u16 centerY = rect.top() + (rect.height() / 2);
    const u16 dropY = event->position().toPoint().y();

    u16 targetRow = targetIndex.row();

    const bool insertBelow = dropY > centerY;
    if (insertBelow) {
        targetRow += 1;
    }

    const QMimeData* const mimeData = event->mimeData();
    const QByteArray data =
        mimeData->data(u"application/x-qabstractitemmodeldatalist"_s);

    u16 sourceRow = *ras<const u16*>(data.constData());

    vector<QStandardItem*> clonedItems;
    clonedItems.reserve(TRACK_PROPERTY_COUNT);

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT - 1)) {
        clonedItems.emplace_back(
            trackTreeModel->item(sourceRow, column)->clone()
        );
    }

    trackTreeModel->insertRow(targetRow);

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT - 1)) {
        trackTreeModel->setItem(targetRow, column, clonedItems[column]);
    }

    if (sourceRow >= targetRow) {
        sourceRow += 1;
    }

    trackTreeModel->removeRow(sourceRow);

    for (const u16 row : range(0, trackTreeModel->rowCount())) {
        auto* const item = new QStandardItem();
        item->setData(row, Qt::EditRole);
        trackTreeModel->setItem(row, u8(TrackProperty::Order), item);
    }

    event->acceptProposedAction();
}

void TrackTree::focusOutEvent(QFocusEvent* const event) {
    clearSelection();
    QTreeView::focusOutEvent(event);
}

void TrackTree::setCurrentIndex(const QModelIndex& newIndex) {
    index = newIndex;
    QTreeView::setCurrentIndex(index);
}

auto TrackTree::rowMetadata(const u16 row) const -> TrackMetadata {
    TrackMetadata result;
    result.reserve(TRACK_PROPERTY_COUNT);

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
        const TrackProperty headerProperty =
            trackTreeModel->trackProperty(column);
        const QModelIndex index = trackTreeModel->index(row, column);

        if (index.isValid()) {
            result.insert_or_assign(
                headerProperty,
                trackTreeModel->data(index).toString()
            );
        }
    }

    return result;
}

void TrackTree::addFile(const TrackMetadata& metadata) {
    const u16 row = trackTreeModel->rowCount();
    const QModelIndex index = currentIndex();

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
        const TrackProperty headerProperty =
            trackTreeModel->trackProperty(column);
        auto* const item = new TrackTreeItem();

        if (headerProperty == TrackProperty::TrackNumber) {
            QString number;
            number.reserve(3);

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
        } else if (headerProperty == TrackProperty::Order) {
            item->setData(row, Qt::EditRole);
            item->setEditable(true);
        } else if (headerProperty != TrackProperty::Play) {
            item->setText(metadata[headerProperty]);
        }

        trackTreeModel
            ->setItem(row, column, item, headerProperty == TrackProperty::Path);
    }
}

void TrackTree::addFileCUE(
    const CUETrack& track,
    const TrackMetadata& metadata,
    const QString& cueFilePath
) {
    const u16 row = trackTreeModel->rowCount();

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
        const auto headerProperty = trackTreeModel->trackProperty(column);
        auto* item = new TrackTreeItem();

        if (column == 0) {
            item->setData(track.offset, CUE_OFFSET_ROLE);
            item->setData(cueFilePath, CUE_FILE_PATH_ROLE);
        }

        if (headerProperty == TrackProperty::TrackNumber) {
            QString number;
            number.reserve(3);

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
        } else if (headerProperty == TrackProperty::Order) {
            item->setData(row, Qt::EditRole);
            item->setEditable(true);
        } else if (headerProperty != TrackProperty::Play) {
            item->setText(metadata[headerProperty]);
        }

        trackTreeModel
            ->setItem(row, column, item, headerProperty == TrackProperty::Path);
    }
}

void TrackTree::sortByPath() {
    //! Don't forget about this cool persistent model index thing: it really
    //! simplifies EVERYTHING
    const QPersistentModelIndex persistent = currentIndex();

    for (u8 column = TRACK_PROPERTY_COUNT - 1; column >= 0; column--) {
        if (trackTreeModel->trackProperty(column) == TrackProperty::Path) {
            sortByColumn(column, Qt::AscendingOrder);
            break;
        }
    }

    if (persistent.isValid()) {
        setCurrentIndex(persistent);
    }
}

void TrackTree::fillTable(
    const QStringList& paths,
    const QList<QVariant>& cueOffsets,
    const bool fromArgs
) {
    QThreadPool::globalInstance()->start([=, this] -> void {
        map<QString, CueInfo> cueInfos;
        QString totalDuration;

        for (const u16 idx : range(0, paths.size())) {
            const QString& path = paths[idx];
            const auto info = QFileInfo(path);
            const QString extension = info.suffix().toLower();

            if (extension == EXT_CUE) {
                if (!cueInfos.contains(path)) {
                    auto cueFile = QFile(path);

                    if (!cueFile.open(QFile::ReadOnly | QFile::Text)) {
                        // This is not likely to fail, because we just parsed
                        // the contents from this cue file. But if it does fail,
                        // just continue
                        continue;
                    };

                    cueInfos.emplace(path, parseCUE(cueFile, info));
                }

                if (cueInfos.contains(path)) {
                    const auto& cueOffset = cueOffsets[idx];

                    if (totalDuration.isEmpty()) {
                        totalDuration =
                            cueInfos[path].metadata[TrackProperty::Duration];
                    }

                    const auto& tracks = cueInfos[path].tracks;
                    const auto& offsets = views::transform(
                        tracks,
                        [this](const auto& track) -> u16 {
                        return track.offset;
                    }
                    );

                    const isize idx = find_index(offsets, cueOffset);

                    if (idx != -1) {
                        const auto& track = tracks[idx];
                        auto& metadata = cueInfos[path].metadata;

                        metadata.insert_or_assign(
                            TrackProperty::TrackNumber,
                            track.trackNumber
                        );
                        metadata.insert_or_assign(
                            TrackProperty::Title,
                            track.title
                        );
                        metadata.insert_or_assign(
                            TrackProperty::Artist,
                            track.artist
                        );

                        const u16 offset = track.offset;
                        const u16 endTime = idx < tracks.size() - 1
                                                ? tracks[idx + 1].offset
                                                : timeToSecs(totalDuration);

                        const QString duration = secsToMins(endTime - offset);

                        metadata.insert_or_assign(
                            TrackProperty::Duration,
                            duration
                        );

                        addFileCUE(track, metadata, path);
                    }
                }

                continue;
            }

            if (info.isFile() &&
                !ranges::contains(ALLOWED_PLAYABLE_EXTENSIONS, extension)) {
                continue;
            }

            if (info.isDir()) {
                const QDir dir = QDir(path);
                const QStringList entries = dir.entryList(
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
                );

                const auto pathsView = views::transform(
                    entries,
                    [this, &dir](const QString& entry) -> QString {
                    return dir.absoluteFilePath(entry);
                }
                );

                const auto paths =
                    QStringList(pathsView.begin(), pathsView.end());

                fillTable(paths);
                continue;
            }

            if (trackTreeModel->contains(path)) {
                continue;
            }

            const auto extracted = extractMetadata(path);

            if (!extracted) {
                LOG_WARN(extracted.error());
                continue;
            }

            const auto& metadata = extracted.value();

            if (metadata.empty()) {
                continue;
            }

            QMetaObject::invokeMethod(
                this,
                &TrackTree::addFile,
                Qt::QueuedConnection,
                metadata
            );
        }

        QMetaObject::invokeMethod(
            this,
            &TrackTree::postFill,
            Qt::QueuedConnection
        );

        emit fillingFinished(fromArgs && paths.size() == 1);
    });
}

void TrackTree::fillTableCUE(
    TrackMetadata& metadata,
    const QList<CUETrack>& tracks,
    const QString& cueFilePath
) {
    const QString totalDuration = metadata[TrackProperty::Duration];

    for (const auto& [idx, track] : views::enumerate(tracks)) {
        metadata.insert_or_assign(
            TrackProperty::TrackNumber,
            track.trackNumber
        );
        metadata.insert_or_assign(TrackProperty::Title, track.title);
        metadata.insert_or_assign(TrackProperty::Artist, track.artist);

        const u16 offset = track.offset;
        const u16 endTime = idx < tracks.size() - 1 ? tracks[idx + 1].offset
                                                    : timeToSecs(totalDuration);

        const QString duration = secsToMins(endTime - offset);

        metadata.insert_or_assign(TrackProperty::Duration, duration);

        addFileCUE(track, metadata, cueFilePath);
    }

    QMetaObject::invokeMethod(this, &TrackTree::postFill, Qt::QueuedConnection);
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
        trackTreeModel->item(index, 0)->setData(QString(), Qt::DecorationRole);
    } else {
        if (modelIndex.isValid()) {
            trackTreeModel->item(modelIndex.row(), 0)
                ->setData(QString(), Qt::DecorationRole);
        }
    }

    setCurrentIndex(trackTreeModel->index(-1, -1));
    return modelIndex;
}

void TrackTree::handleHeaderPress(
    const u8 index,
    const Qt::MouseButton button
) {
    if (button == Qt::RightButton) {
        auto* const menu = new OptionMenu(this);

        for (const auto& [idx, label] :
             views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
            auto* const action = new QAction(label, menu);
            action->setCheckable(true);

            i8 columnIndex = -1;

            for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
                if (trackTreeModel->headerData(column, Qt::Horizontal)
                        .toString() == label) {
                    columnIndex = i8(column);
                    break;
                }
            }

            if (columnIndex != -1) {
                action->setChecked(!isColumnHidden(columnIndex));
            } else {
                action->setDisabled(true);
            }

            connect(action, &QAction::triggered, menu, [=, this] -> void {
                if (columnIndex != -1) {
                    const bool currentlyHidden = isColumnHidden(columnIndex);
                    setColumnHidden(columnIndex, !currentlyHidden);

                    if (currentlyHidden) {
                        resizeColumnToContents(columnIndex);
                    }
                }
            });

            menu->addAction(action);
        }

        menu->exec(QCursor::pos());
        menu->deleteLater();
    }
}

void TrackTree::resetSorting(
    const i32 /* unused */,
    const Qt::SortOrder sortOrder
) {
    if (musicHeader->sortIndicatorSection() == -1) {
        sortByPath();
    }
};

void TrackTree::setOpacity(const f32 opacity) {
    opacity_ = opacity;

    if (opacity == 1.0F) {
        setStyleSheet(QString());
    } else {
        const QPalette palette = QApplication::palette();

        QColor treeBackgroundColor = palette.color(QPalette::Base);
        QColor headerBackgroundColor = palette.color(QPalette::Button);

        treeBackgroundColor.setAlphaF(opacity);
        headerBackgroundColor.setAlphaF(opacity);

        setStyleSheet(
            u"TrackTree { background-color: rgba(%1, %2, %3, %4) }\nTrackTreeHeader { background-color: rgba(%5, %6, %7, %8) }"_s
                .arg(treeBackgroundColor.red())
                .arg(treeBackgroundColor.green())
                .arg(treeBackgroundColor.blue())
                .arg(QString::number(treeBackgroundColor.alphaF(), 'f', 2))
                .arg(headerBackgroundColor.red())
                .arg(headerBackgroundColor.green())
                .arg(headerBackgroundColor.blue())
                .arg(QString::number(headerBackgroundColor.alphaF(), 'f', 2))
        );
    }
};