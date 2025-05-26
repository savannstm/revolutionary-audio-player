#include "tracktree.hpp"

#include "enums.hpp"
#include "extractmetadata.hpp"
#include "musicitem.hpp"
#include "optionmenu.hpp"
#include "rapidhasher.hpp"
#include "trackproperties.hpp"

#include <QApplication>
#include <QDirIterator>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QThreadPool>

TrackTree::TrackTree(QWidget* parent) : QTreeView(parent) {
    setHeader(musicHeader);
    setModel(musicModel);
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

    connect(this, &TrackTree::finishedFilling, this, &TrackTree::postFill);

    connect(this, &TrackTree::metadataReceived, this, &TrackTree::addFile);

    connect(
        musicHeader,
        &MusicHeader::headerPressed,
        this,
        &TrackTree::handleHeaderPress
    );

    connect(
        musicHeader,
        &MusicHeader::sortIndicatorChanged,
        this,
        &TrackTree::resetSorting
    );
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

    if (newIndex.column() != Order) {
        emit trackSelected(index.row(), newIndex.row());
        index = newIndex;
        setCurrentIndex(newIndex);
    }

    QTreeView::mouseDoubleClickEvent(event);
}

void TrackTree::mousePressEvent(QMouseEvent* event) {
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

    auto* mimeData = new QMimeData();
    // CRAZY hack to not override other methods
    mimeData->setData(u"application/x-qabstractitemmodeldatalist"_s, byteArray);

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(supportedActions, defaultDropAction());
}

void TrackTree::dropEvent(QDropEvent* event) {
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

    const QMimeData* mimeData = event->mimeData();
    const QByteArray data =
        mimeData->data(u"application/x-qabstractitemmodeldatalist"_s);

    u16 sourceRow = *ras<const u16*>(data.constData());

    vector<QStandardItem*> clonedItems;
    clonedItems.reserve(TRACK_PROPERTY_COUNT);

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT - 1)) {
        clonedItems.emplace_back(musicModel->item(sourceRow, column)->clone());
    }

    musicModel->insertRow(targetRow);

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT - 1)) {
        musicModel->setItem(targetRow, column, clonedItems[column]);
    }

    if (sourceRow >= targetRow) {
        sourceRow += 1;
    }

    musicModel->removeRow(sourceRow);

    for (const u16 row : range(0, musicModel->rowCount())) {
        auto* item = new QStandardItem();
        item->setData(row, Qt::EditRole);
        musicModel->setItem(row, Order, item);
    }

    event->acceptProposedAction();
}

void TrackTree::focusOutEvent(QFocusEvent* event) {
    clearSelection();
    QTreeView::focusOutEvent(event);
}

void TrackTree::setCurrentIndex(const QModelIndex& newIndex) {
    index = newIndex;
    QTreeView::setCurrentIndex(index);
}

auto TrackTree::rowMetadata(const u16 row) const
    -> HashMap<TrackProperty, QString> {
    HashMap<TrackProperty, QString> result;
    result.reserve(TRACK_PROPERTY_COUNT);

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

    for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
        const auto headerProperty = musicModel->trackProperty(column);
        auto* item = new MusicItem();

        if (headerProperty == TrackNumber) {
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
        } else if (headerProperty == Order) {
            item->setData(row, Qt::EditRole);
            item->setEditable(true);
        } else if (headerProperty != Play) {
            item->setText(metadata[headerProperty]);
        }

        musicModel->setItem(row, column, item, headerProperty == Path);
    }
}

void TrackTree::sortByPath() {
    for (u8 column = TRACK_PROPERTY_COUNT - 1; column >= 0; column--) {
        if (musicModel->trackProperty(column) == Path) {
            sortByColumn(column, Qt::AscendingOrder);
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

void TrackTree::handleHeaderPress(
    const u8 index,
    const Qt::MouseButton button
) {
    if (button == Qt::RightButton) {
        auto* menu = new OptionMenu(this);

        for (const auto& [idx, label] :
             views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
            auto* action = new QAction(label, menu);
            action->setCheckable(true);

            i8 columnIndex = -1;

            for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
                if (musicModel->headerData(column, Qt::Horizontal).toString() ==
                    label) {
                    columnIndex = as<i8>(column);
                    break;
                }
            }

            if (columnIndex != -1) {
                action->setChecked(!isColumnHidden(columnIndex));
            } else {
                action->setEnabled(false);
            }

            connect(action, &QAction::triggered, menu, [=, this] {
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