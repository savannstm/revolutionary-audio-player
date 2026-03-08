#include "TrackTable.hpp"

#include "Constants.hpp"
#include "Duration.hpp"
#include "Enums.hpp"
#include "OptionMenu.hpp"
#include "TrackTableHeader.hpp"
#include "TrackTableModel.hpp"
#include "Utils.hpp"

#include <QDir>
#include <QDrag>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QThreadPool>

// TODO: Allow custom row height/width for rows
// TODO: Allow custom highlight options for selected row

class TrackTableDelegate : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(TrackTable::TRACK_TABLE_ROW_HEIGHT);
        size.setWidth(size.width() + PLAY_ICON_SIZE);
        return size;
    }

    void paint(
        QPainter* const painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override {
        const auto* const tree = as<const TrackTable*>(option.widget);
        const bool isPlayingRow = tree != nullptr &&
                                  tree->currentIndex().isValid() &&
                                  index.row() == tree->currentIndex().row();
        QStyleOptionViewItem modifiedOption = option;

        if (isPlayingRow) {
            modifiedOption.state |= QStyle::State_Selected;
        }

        i8 leftmostShown = -1;
        i8 leftmostVisualIndex = INT8_MAX;

        for (const auto [idx, settings] :
             views::enumerate(tree->columnSettings())) {
            if (!settings.hidden && settings.index < leftmostVisualIndex) {
                leftmostVisualIndex = i8(settings.index);
                leftmostShown = i8(idx);
            }
        }

        if (index.column() != leftmostShown) {
            QStyledItemDelegate::paint(painter, modifiedOption, index);
            return;
        }

        if ((modifiedOption.state & QStyle::State_Selected) != 0) {
            const QRect iconArea(
                option.rect.left(),
                option.rect.top(),
                TrackTable::TRACK_TABLE_ROW_HEIGHT,
                option.rect.height()
            );
            painter->fillRect(
                iconArea,
                modifiedOption.palette.brush(QPalette::Highlight)
            );
        }

        modifiedOption.rect.setLeft(
            option.rect.left() + TrackTable::TRACK_TABLE_ROW_HEIGHT
        );

        QStyledItemDelegate::paint(painter, modifiedOption, index);

        if (tree != nullptr && index.row() == tree->currentIndex().row() &&
            tree->status() != TreeStatus::Idle) {
            const QIcon icon = QIcon::fromTheme(
                tree->status() == TreeStatus::Playing
                    ? QIcon::ThemeIcon::MediaPlaybackStart
                    : QIcon::ThemeIcon::MediaPlaybackPause
            );

            QRect iconRect = option.rect.adjusted(1, 1, 0, -1);
            iconRect.setWidth(PLAY_ICON_SIZE);

            icon.paint(painter, iconRect);
        }
    }

   private:
    static constexpr u8 PLAY_ICON_SIZE = TrackTable::TRACK_TABLE_ROW_HEIGHT - 2;
};

TrackTable::TrackTable(QWidget* const parent) :
    QTableView(parent),
    header_(new TrackTableHeader(Qt::Horizontal, this)),
    model_(new TrackTableModel(this)) {
    setHorizontalHeader(header_);
    setModel(model_);
    setItemDelegate(new TrackTableDelegate(this));

    setShowGrid(false);
    setGridStyle(Qt::NoPen);
    setAlternatingRowColors(false);
    setWordWrap(false);

    setEditTriggers(QTableView::NoEditTriggers);

    setSelectionMode(QTableView::ContiguousSelection);
    setSelectionBehavior(QTableView::SelectRows);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QTableView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(false);

    setTabKeyNavigation(false);

    setTextElideMode(Qt::ElideNone);

    setHorizontalScrollMode(QTableView::ScrollPerPixel);
    setVerticalScrollMode(QTableView::ScrollPerPixel);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setSortingEnabled(true);

    setUpdateThreshold(200);

    connect(
        header_,
        &TrackTableHeader::headerPressed,
        this,
        &TrackTable::handleHeaderPress
    );

    connect(
        model_,
        &TrackTableModel::rowsAboutToBeRemoved,
        this,
        [this](const QModelIndex& /* parent */, const i32 first, const i32 last)
            -> void {
        for (const i32 row : range(first, last + 1)) {
            const u32 secsDuration =
                model_->item(row, u8(TrackProperty::Duration)).toUInt();
            duration_ -= secsDuration;
        }
    }
    );

    connect(model_, &TrackTableModel::rowsRemoved, this, [this] -> void {
        emit durationChanged(duration_);
    });
}

auto TrackTable::visualRect(const QModelIndex& index) const -> QRect {
    const QRect rect = QTableView::visualRect(index);
    if (rect.isValid() || !index.isValid() || state() != DraggingState) {
        return rect;
    }

    if (!isColumnHidden(index.column())) {
        return rect;
    }

    const auto* const currentModel = model();
    if (currentModel == nullptr || index.model() != currentModel) {
        return rect;
    }

    i32 firstVisibleColumn = -1;
    for (i32 column = 0; column < currentModel->columnCount(); ++column) {
        if (!isColumnHidden(column)) {
            firstVisibleColumn = column;
            break;
        }
    }

    if (firstVisibleColumn == -1) {
        return rect;
    }

    const QModelIndex fallbackIndex =
        currentModel->index(index.row(), firstVisibleColumn, index.parent());
    return QTableView::visualRect(fallbackIndex);
}

void TrackTable::mouseDoubleClickEvent(QMouseEvent* const event) {
    const QModelIndex newIndex = indexAt(event->pos());
    if (!newIndex.isValid()) {
        return;
    }

    if (newIndex.column() != u8(TrackProperty::Order)) {
        emit playingChanged(newIndex);
        playingIndex = newIndex;
        viewport()->update();
    }

    QTableView::mouseDoubleClickEvent(event);
}

void TrackTable::startDrag(const Qt::DropActions supportedActions) {
    Q_UNUSED(supportedActions);

    if (selectionModel() == nullptr) {
        return;
    }

    const QModelIndexList selectedRows = selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    auto* const mimeData = model_->mimeData(selectedRows);
    if (mimeData == nullptr) {
        return;
    }

    auto* const drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Keep source rows unchanged unless the model performs an internal move.
    drag->exec(Qt::MoveAction, Qt::MoveAction);
}

auto TrackTable::rowMetadata(const i32 row) const -> TrackMetadata {
    TrackMetadata result;
    result.reserve(TRACK_PROPERTY_COUNT);

    for (const auto prop : TRACK_PROPERTIES) {
        const QModelIndex index = model_->index(row, u8(prop));
        result.insert_or_assign(prop, model_->data(index).toString());
    }

    return result;
}

auto TrackTable::makeItemText(
    const u32 row,
    const TrackProperty prop,
    const TrackMetadata& metadata
) -> QString {
    switch (prop) {
        case TrackProperty::TrackNumber: {
            QString number;
            number.reserve(3);

            for (const auto [idx, chr] : views::enumerate(metadata[prop])) {
                if (idx == 0 && chr == '0') {
                    continue;
                }

                if (!chr.isDigit()) {
                    break;
                }

                number.append(chr);
            }

            return number;
        }
        case TrackProperty::Order:
            return QString::number(row);
        case TrackProperty::Duration: {
            const u32 secsDuration = metadata[prop].toUInt();
            duration_ += secsDuration;
            return Duration::secondsToString(secsDuration);
        }
        default:
            return metadata[prop];
    }
}

void TrackTable::addFile(const TrackMetadata& metadata) {
    const i32 row = model_->rowCount();
    model_->appendRow({});

    for (const auto prop : TRACK_PROPERTIES) {
        model_->setText(row, u8(prop), makeItemText(row, prop, metadata));
    }
}

void TrackTable::addFileCUE(
    const CUETrack& track,
    const TrackMetadata& metadata,
    const QString& cueFilePath
) {
    const i32 row = model_->rowCount();
    model_->appendRow({});
    model_->setCUEInfo(row, cueFilePath, track.offset);

    for (const auto prop : TRACK_PROPERTIES) {
        model_->setText(row, u8(prop), makeItemText(row, prop, metadata));
    }
}

void TrackTable::sortByPath() {
    sortByColumn(u8(TrackProperty::Path), Qt::AscendingOrder);
}

void TrackTable::fill(
    const QStringList& paths,
    const QVariantList& cueOffsets,
    const bool fromArgs
) {
    // TODO: This still runs when tab is closed, which leads to segfault
    QThreadPool::globalInstance()->start([=, this] -> void {
        HashMap<QString, CUEInfo> CUEInfos;
        u32 durationSecs = UINT32_MAX;

        bool firstIsDir = false;

        for (const u32 idx : range<u32>(0, paths.size())) {
            const QString& path = paths[idx];
            const auto info = QFileInfo(path);
            const QString extension = info.suffix().toLower();

            if (extension == EXT_CUE) {
                if (!CUEInfos.contains(path)) {
                    auto cueFile = QFile(path);

                    if (!cueFile.open(QFile::ReadOnly | QFile::Text)) {
                        // This is not likely to fail, because we just
                        // parsed the contents from this cue file. But if it
                        // does fail, just continue
                        continue;
                    };

                    CUEInfos.emplace(path, parseCUE(cueFile, info));
                }

                if (CUEInfos.contains(path)) {
                    const auto& cueOffset = cueOffsets[idx];

                    if (durationSecs == UINT32_MAX) {
                        durationSecs = CUEInfos[path]
                                           .metadata[TrackProperty::Duration]
                                           .toUInt();
                    }

                    const auto& tracks = CUEInfos[path].tracks;
                    const auto& offsets = views::transform(
                        tracks,
                        [this](const auto& track) -> u32 {
                        return track.offset;
                    }
                    );

                    const isize idx = find_index(offsets, cueOffset);

                    if (idx != -1) {
                        const auto& track = tracks[idx];
                        auto& metadata =
                            const_cast<TrackMetadata&>(CUEInfos[path].metadata);

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

                        const u32 offset = track.offset;
                        const u32 endTime = idx < tracks.size() - 1
                                                ? tracks[idx + 1].offset
                                                : durationSecs;

                        const auto duration = QString::number(endTime - offset);

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
                !ranges::contains(SUPPORTED_PLAYABLE_EXTENSIONS, extension)) {
                continue;
            }

            if (info.isDir()) {
                if (idx == 0) {
                    firstIsDir = true;
                }

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

                fill(paths);
                continue;
            }

            if (model_->contains(path)) {
                continue;
            }

            const auto extracted = extractMetadata(path);

            if (!extracted) {
                qWarning() << extracted.error();
                continue;
            }

            const auto& metadata = extracted.value();

            if (metadata.empty()) {
                continue;
            }

            QMetaObject::invokeMethod(
                this,
                &TrackTable::addFile,
                Qt::QueuedConnection,
                metadata
            );
        }

        QMetaObject::invokeMethod(
            this,
            &TrackTable::postFill,
            Qt::QueuedConnection
        );

        emit fillingFinished(fromArgs && paths.size() == 1 && !firstIsDir);
    });
}

void TrackTable::fillCUE(
    TrackMetadata& metadata,
    const vector<CUETrack>& tracks,
    const QString& cueFilePath
) {
    const u32 durationSecs = metadata[TrackProperty::Duration].toUInt();

    for (const auto& [idx, track] : views::enumerate(tracks)) {
        metadata.insert_or_assign(
            TrackProperty::TrackNumber,
            track.trackNumber
        );
        metadata.insert_or_assign(TrackProperty::Title, track.title);
        metadata.insert_or_assign(TrackProperty::Artist, track.artist);

        const u32 offset = track.offset;
        const u32 endTime =
            idx < tracks.size() - 1 ? tracks[idx + 1].offset : durationSecs;

        const QString duration = QString::number(endTime - offset);

        metadata.insert_or_assign(TrackProperty::Duration, duration);

        addFileCUE(track, metadata, cueFilePath);
    }

    QMetaObject::invokeMethod(
        this,
        &TrackTable::postFill,
        Qt::QueuedConnection
    );
}

void TrackTable::postFill() {
    resizeColumnsToContents();
    sortByPath();
}

void TrackTable::resizeColumnsToContents() {
    for (const auto prop : TRACK_PROPERTIES) {
        resizeColumnToContents(u8(prop));
    }
}

void TrackTable::handleHeaderPress(
    const u8 index,
    const Qt::MouseButton button
) {
    if (button != Qt::RightButton) {
        return;
    }

    auto* const menu = new OptionMenu(this);

    for (const auto prop : TRACK_PROPERTIES) {
        const QString label = trackPropertyLabel(prop);
        auto* const action = new QAction(label, menu);
        action->setCheckable(true);

        if (isColumnHidden(u8(prop))) {
            action->setChecked(false);
        } else {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, menu, [this, prop] -> void {
            if (isColumnHidden(u8(prop))) {
                showColumn(u8(prop));
                resizeColumnToContents(u8(prop));
                columnSettings_[u8(prop)].hidden = false;
            } else {
                hideColumn(u8(prop));
                columnSettings_[u8(prop)].hidden = true;
            }
        });

        menu->addAction(action);
    }

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void TrackTable::setOpacity(const f32 opacity) {
    opacity_ = opacity;

    if (opacity == 1.0F) {
        setStyleSheet(QString());
    } else {
        const QPalette palette = qApp->palette();

        QColor treeBackgroundColor = palette.color(QPalette::Base);
        QColor headerBackgroundColor = palette.color(QPalette::Button);

        treeBackgroundColor.setAlphaF(opacity);
        headerBackgroundColor.setAlphaF(opacity);

        setStyleSheet(
            u"TrackTable { background-color: rgba(%1, %2, %3, %4) }\nTrackTableHeader { background-color: rgba(%5, %6, %7, %8) }"_s
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
