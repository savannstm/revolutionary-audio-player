#include "PlaylistView.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "PlaylistTabBar.hpp"
#include "Settings.hpp"
#include "TrackTable.hpp"
#include "TrackTableHeader.hpp"
#include "TrackTableModel.hpp"
#include "Utils.hpp"

#include <QDir>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QScreen>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

PlaylistView::PlaylistView(QWidget* const parent) :
    QWidget(parent),

    tabBar_(new PlaylistTabBar(this)),
    stackedWidget(new QStackedWidget(this)),
    layout(new QVBoxLayout(this)) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabBar_, 0, Qt::AlignLeft);
    layout->addWidget(stackedWidget);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(
        tabBar_,
        &PlaylistTabBar::indexChanged,
        this,
        &PlaylistView::changePage
    );

    connect(
        tabBar_,
        &PlaylistTabBar::tabAdded,
        this,
        &PlaylistView::createTabPage
    );

    connect(
        tabBar_,
        &PlaylistTabBar::tabsRemoved,
        this,
        &PlaylistView::removePages
    );
}

void PlaylistView::removePages(const TabRemoveMode mode, const u8 startIndex) {
    u8 count = 0;

    switch (mode) {
        case TabRemoveMode::Single:
            removePage(startIndex);
            count += 1;
            break;
        case TabRemoveMode::Other:
        case TabRemoveMode::ToRight:
            for (i8 i = i8(stackedWidget->count() - 1); i > startIndex; i--) {
                removePage(i);
                count += 1;
            }

            if (mode == TabRemoveMode::ToRight) {
                break;
            }
        case TabRemoveMode::ToLeft:
            for (i8 i = i8(startIndex - 1); i >= 0; i--) {
                removePage(i);
                count += 1;
            }
            break;
    }

    changePage(tabBar_->currentIndex());
    emit tabsRemoved(mode, startIndex, count);
}

auto PlaylistView::createPage(const optional<ColumnSettingsArray> cols)
    -> QWidget* {
    auto* const page = new QWidget(this);
    auto* const pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto* const table = new TrackTable(page);
    TrackTableModel* const model = table->model();
    TrackTableHeader* const header = table->header();
    table->setObjectName("table"_L1);

    header->setDefaultAlignment(Qt::AlignLeft);
    header->setSectionsClickable(true);
    header->setSectionsMovable(true);
    header->setFirstSectionMovable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicatorClearable(true);
    header->setMinimumSectionSize(MINIMUM_TRACK_TABLE_COLUMN_SECTION_SIZE);

    auto* const centerBackgroundLabel = new QLabel(page);
    centerBackgroundLabel->setObjectName("centerLabel"_L1);

    auto* const leftBackgroundLabel = new QLabel(page);
    leftBackgroundLabel->setObjectName("leftLabel"_L1);

    auto* const rightBackgroundLabel = new QLabel(page);
    rightBackgroundLabel->setObjectName("rightLabel"_L1);

    pageLayout->addWidget(table);

    table->setColumnSettings(cols.value_or(settings->playlist.columns));

    for (const auto prop : TRACK_PROPERTIES) {
        const ColumnSettings col = table->columnSettings()[u8(prop)];

        model
            ->setHeaderData(u8(prop), Qt::Horizontal, trackPropertyLabel(prop));

        if (col.hidden) {
            table->hideColumn(u8(prop));
        }
    };

    array<std::pair<u8, u8>, TRACK_PROPERTY_COUNT> moves;
    u8 moveCount = 0;

    for (const auto prop : TRACK_PROPERTIES) {
        const ColumnSettings col = table->columnSettings()[u8(prop)];
        moves[moveCount++] = { u8(prop), col.index };
    }

    ranges::sort(
        moves.begin(),
        moves.begin() + moveCount,
        [](const auto elA, const auto elB) -> auto {
        return elA.second < elB.second;
    }
    );

    for (const u8 idx : range<u8>(0, moveCount)) {
        header->moveSection(header->visualIndex(moves[idx].first), idx);
    }

    connect(
        header,
        &TrackTableHeader::sectionMoved,
        table,
        [table, header](const u8 logical, const u8 oldIndex, const u8 newIndex)
            -> void {
        array<std::pair<u8, u8>, TRACK_PROPERTY_COUNT> visibleColumns;
        u8 visibleCount = 0;

        for (const auto logicalIdx : TRACK_PROPERTIES) {
            if (!header->isSectionHidden(u8(logicalIdx))) {
                const u8 visualIdx = header->visualIndex(u8(logicalIdx));
                visibleColumns[visibleCount++] = { visualIdx, u8(logicalIdx) };
            }
        }

        ranges::sort(
            visibleColumns.begin(),
            visibleColumns.begin() + visibleCount,
            [](const auto elA, const auto elB) -> auto {
            return elA.first < elB.first;
        }
        );

        for (const u8 relativeIdx : range<u8>(0, visibleCount)) {
            const u8 logicalIdx = visibleColumns[relativeIdx].second;
            table->setColumnIndex(logicalIdx, u8(relativeIdx));
        }
    }
    );

    connect(
        table,
        &TrackTable::playingChanged,
        this,
        [this, table](const QModelIndex& index) -> void {
        setPlayingIndex(table, index);
        // TODO: Add play indicator to playing tab
    }
    );

    connect(table, &TrackTable::pressed, this, &PlaylistView::trackPressed);

    connect(
        model,
        &TrackTableModel::rowsRemoved,
        this,
        &PlaylistView::rowsRemoved
    );

    connect(
        model,
        &TrackTableModel::rowsInserted,
        this,
        &PlaylistView::rowsInserted
    );

    connect(model, &TrackTableModel::layoutChanged, this, [this] -> void {
        emit layoutChanged();
    });

    table->sortByPath();

    return page;
}

void PlaylistView::setTableOpacity(const u8 index, const f32 opacity) const {
    this->table(index)->setOpacity(opacity);
}

void PlaylistView::removeBackgroundImage(const u8 index) const {
    QLabel* centerLabel = backgroundImage(index);

    if (!hasBackgroundImage(index)) {
        return;
    }

    setTableOpacity(index, 1.0F);

    QWidget* const pageWidget = page(index);
    auto* leftLabel = pageWidget->findChild<QLabel*>("leftLabel"_L1);
    auto* rightLabel = pageWidget->findChild<QLabel*>("rightLabel"_L1);

    delete centerLabel;
    delete leftLabel;
    delete rightLabel;

    centerLabel = new QLabel(pageWidget);
    leftLabel = new QLabel(pageWidget);
    rightLabel = new QLabel(pageWidget);

    centerLabel->setObjectName("centerLabel"_L1);
    leftLabel->setObjectName("leftLabel"_L1);
    rightLabel->setObjectName("rightLabel"_L1);
}

void PlaylistView::setBackgroundImage(
    const u8 index,  // NOLINT
    const QImage& image,
    const QString& path
) const {
    if (path == backgroundImagePath(index)) {
        return;
    }

    QLabel* const centerLabel = backgroundImage(index);
    const QWidget* const pageWidget = page(index);

    const f32 currentOpacity = tableOpacity(index);

    setTableOpacity(
        index,
        tableOpacity(index) == 1.0F ? HALF_TRANSPARENT : currentOpacity
    );

    const u16 layoutWidth = screen()->size().width();

    const QPixmap centerPixmap = QPixmap::fromImage(image).scaled(
        layoutWidth,
        stackedWidget->height(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    centerLabel->setPixmap(centerPixmap);
    centerLabel->setFixedSize(centerPixmap.size());
    centerLabel->move(
        pageWidget->geometry().center().x() - (centerLabel->width() / 2),
        pageWidget->geometry().top()
    );

    centerLabel->lower();
    centerLabel->show();

    auto* const leftLabel = pageWidget->findChild<QLabel*>("leftLabel"_L1);
    auto* const rightLabel = pageWidget->findChild<QLabel*>("rightLabel"_L1);

    const u16 sideWidth = (layoutWidth - centerPixmap.width()) / 2;

    auto* const blurEffect = new QGraphicsBlurEffect();
    blurEffect->setBlurHints(QGraphicsBlurEffect::QualityHint);
    blurEffect->setBlurRadius(BLUR_SIGMA);

    const QImage blurredImage = applyEffectToImage(image, blurEffect);

    const QPixmap blurredPixmap = QPixmap::fromImage(blurredImage)
                                      .scaled(
                                          centerPixmap.size(),
                                          Qt::IgnoreAspectRatio,
                                          Qt::SmoothTransformation
                                      );

    const QPixmap leftSide =
        blurredPixmap.copy(0, 0, sideWidth, centerPixmap.height());
    const QPixmap rightSide = blurredPixmap.copy(
        blurredPixmap.width() - sideWidth,
        0,
        sideWidth,
        centerPixmap.height()
    );

    leftLabel->setPixmap(leftSide);
    leftLabel->setFixedSize(leftSide.size());
    leftLabel->lower();
    leftLabel->show();

    rightLabel->setPixmap(rightSide);
    rightLabel->setFixedSize(rightSide.size());
    rightLabel->move(
        centerLabel->geometry().right(),
        centerLabel->geometry().top()
    );
    rightLabel->lower();
    rightLabel->show();

    centerLabel->setProperty("path", path);
}

auto PlaylistView::tableOpacity(const u8 index) const -> f32 {
    return table(index)->opacity();
}

void PlaylistView::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::PaletteChange) {
        for (const u8 idx : range<u8>(0, tabCount())) {
            TrackTable* const table = this->table(idx);
            table->setOpacity(table->opacity());

            // TODO: Tabs text repaint
        }
    }

    QWidget::changeEvent(event);
}

void PlaylistView::setTabColor(const u8 index, const QString& color) {
    tabBar_->setTabColor(index, color);
};

void PlaylistView::setTabLabel(const u8 index, const QString& label) {
    tabBar_->setTabLabel(index, label);
};

[[nodiscard]] auto PlaylistView::tabCount() const -> u8 {
    return tabBar_->tabCount();
};

[[nodiscard]] auto PlaylistView::currentIndex() const -> i8 {
    return tabBar_->currentIndex();
};

[[nodiscard]] auto PlaylistView::playingIndex() const -> QModelIndex {
    return playingData.index;
};

[[nodiscard]] auto PlaylistView::playingTable() const -> TrackTable* {
    return playingData.table;
};

[[nodiscard]] auto PlaylistView::currentBackgroundImage() const -> QLabel* {
    return stackedWidget->currentWidget()->findChild<QLabel*>("centerLabel"_L1);
};

[[nodiscard]] auto PlaylistView::backgroundImage(const u8 index) const
    -> QLabel* {
    return stackedWidget->widget(index)->findChild<QLabel*>("centerLabel"_L1);
};

[[nodiscard]] auto PlaylistView::table(const u8 index) const -> TrackTable* {
    const QWidget* const widget = stackedWidget->widget(index);

    if (widget == nullptr) {
        return nullptr;
    }

    return widget->findChild<TrackTable*>("table"_L1);
};

[[nodiscard]] auto PlaylistView::currentTable() const -> TrackTable* {
    return table(currentIndex());
};

[[nodiscard]] auto PlaylistView::tabLabel(const u8 index) const -> QString {
    return tabBar_->tabLabel(index);
};

[[nodiscard]] auto PlaylistView::tabColor(const u8 index) const -> QString {
    return tabBar_->tabColor(index);
};

[[nodiscard]] auto PlaylistView::currentPage() const -> QWidget* {
    return stackedWidget->currentWidget();
};

[[nodiscard]] auto PlaylistView::page(const u8 index) const -> QWidget* {
    return stackedWidget->widget(index);
};

[[nodiscard]] auto PlaylistView::backgroundImagePath(const u8 index) const
    -> QString {
    const QLabel* const centerLabel = backgroundImage(index);
    return centerLabel->property("path").toString();
};

[[nodiscard]] auto PlaylistView::addTab(
    const QString& label,
    const ColumnSettingsArray& cols
) -> u8 {
    const u8 index = u8(stackedWidget->addWidget(createPage(cols)));
    tabBar_->addTab(label);
    return index;
};

void PlaylistView::createTabPage(const u8 index) {
    if (stackedWidget->widget(index) != nullptr) {
        return;
    }

    stackedWidget->insertWidget(index, createPage());
};

[[nodiscard]] auto PlaylistView::hasBackgroundImage(const u8 index) const
    -> bool {
    const QWidget* const widget = stackedWidget->widget(index);

    if (widget == nullptr) {
        return false;
    }

    const QLabel* const centerLabel = backgroundImage(index);
    return centerLabel->property("path").isValid();
};

void PlaylistView::removePage(const u8 index) {
    QWidget* const widget = stackedWidget->widget(index);
    stackedWidget->removeWidget(widget);
    delete widget;
};

void PlaylistView::changePage(const i8 index) {
    stackedWidget->setCurrentIndex(index);
    emit indexChanged(index);
};

void PlaylistView::setCurrentIndex(const i8 index) {
    tabBar_->setCurrentIndex(index);
};

void PlaylistView::setPlayingIndex(
    TrackTable* const table,
    const QModelIndex& index
) {
    if (playingData.table != nullptr) {
        playingData.table->setStatus(TableStatus::Suspended);
    }

    if (!random) {
        shuffledPlaylist = vector<QPersistentModelIndex>();
    }

    const bool needShuffle = random && table != playingTable();

    playingData = { .table = table, .index = index };
    table->setCurrentIndex(index);
    table->setStatus(TableStatus::Playing);

    if (needShuffle) {
        shufflePlaylist();
    }

    emit playingChanged(table, index.row());
}

void PlaylistView::resetPlayingIndex() {
    TrackTable* const playingTable = this->playingTable();

    if (playingTable == nullptr) {
        return;
    }

    playingTable->setStatus(TableStatus::Idle);
    playingTable->setCurrentIndex(QModelIndex());

    playingData = { .table = nullptr, .index = QModelIndex() };
    shuffledPlaylist = vector<QPersistentModelIndex>();
};

auto PlaylistView::advance(const Direction direction) -> AdvanceResult {
    AdvanceResult result;

    TrackTable* table = playingTable();
    if (table == nullptr) {
        return result;
    }

    const TrackTableModel* model = table->model();
    const u16 rowCount = model->rowCount();
    if (rowCount == 0) {
        return result;
    }

    const QModelIndex currentIndex = playingIndex();
    if (!currentIndex.isValid()) {
        return result;
    }

    const u16 currentRow = currentIndex.row();
    u16 nextRow = 0;

    switch (direction) {
        case Direction::Forward:
            if (random) {
                if (shuffledPlaylist.size() == 0) {
                    shufflePlaylist();
                }

                nextRow = shuffledPlaylist[shuffledPos++].row();

                if (shuffledPos == shuffledPlaylist.size()) {
                    shufflePlaylist();
                }

                result.status = AdvanceStatus::TrackFinished;
                break;
            }

            if (currentRow + 1 < rowCount) {
                result.status = AdvanceStatus::TrackFinished;
                nextRow = currentRow + 1;
                break;
            }

            result.status = AdvanceStatus::PlaylistFinished;

            if (repeatMode_ == RepeatMode::Playlist) {
                nextRow = 0;
                break;
            }

            table->setCurrentIndex(QModelIndex());
            table->setStatus(TableStatus::Idle);

            if (repeatMode_ == RepeatMode::Off) {
                while (advanceToNextTable()) {
                    table = playingTable();
                    model = table->model();

                    if (model->rowCount() > 0) {
                        nextRow = 0;
                        break;
                    }
                }

                if (playingTable() == nullptr) {
                    result.status = AdvanceStatus::PlaylistFinished;
                    return result;
                }

                break;
            }

            result.status = AdvanceStatus::PlaylistFinished;
            return result;

        case Direction::Backward:
            if (shuffledPlaylist.empty()) {
                nextRow = (currentRow == 0 ? rowCount : currentRow) - 1;
            } else {
                nextRow = shuffledPlaylist[--shuffledPos].row();
            }

            result.status = AdvanceStatus::TrackFinished;
            break;
    }

    const QModelIndex nextIndex = model->index(nextRow, 0);
    playingData = { .table = table, .index = nextIndex };

    result.table = table;
    result.index = nextIndex;
    table->setCurrentIndex(nextIndex);

    return result;
}

auto PlaylistView::advanceToNextTable() -> bool {
    TrackTable* current = playingTable();

    if (current == nullptr) {
        playingData = { .table = nullptr, .index = QModelIndex() };
        return false;
    }

    bool nextIsSibling = false;

    for (const u8 idx : range<u8>(0, tabCount())) {
        TrackTable* const table = this->table(idx);

        if (nextIsSibling) {
            playingData = { .table = table,
                            .index = table->model()->index(0, 0) };
            return true;
        }

        if (table == current) {
            nextIsSibling = true;
        }
    }

    current->setStatus(TableStatus::Idle);
    current->setCurrentIndex(QModelIndex());
    playingData = { .table = nullptr, .index = QModelIndex() };

    return false;
}

struct RandIntGenerator {
    using result_type = u32;

    static constexpr auto min() -> result_type { return 0; }

    static constexpr auto max() -> result_type { return UINT32_MAX; }

    auto operator()() -> result_type { return randint(); }
};

void PlaylistView::shufflePlaylist() {
    const TrackTableModel* model = playingTable()->model();
    const i32 rowCount = model->rowCount();

    if (shuffledPlaylist.capacity() < rowCount) {
        shuffledPlaylist.resize(rowCount);
    } else {
        shuffledPlaylist.shrink_to_fit();
    }

    for (const i32 idx : range(0, rowCount)) {
        shuffledPlaylist[idx] = model->index(idx, 0);
    }

    shuffledPos = 0;

    RandIntGenerator gen;
    ranges::shuffle(shuffledPlaylist, gen);
}

void PlaylistView::toggleRandom() {
    random = !random;
}

auto PlaylistView::repeatMode() const -> RepeatMode {
    return repeatMode_;
}

void PlaylistView::toggleRepeatMode() {
    switch (repeatMode_) {
        case RepeatMode::Off:
            repeatMode_ = RepeatMode::Playlist;
            break;
        case RepeatMode::Playlist:
            repeatMode_ = RepeatMode::Track;
            break;
        case RepeatMode::Track:
            repeatMode_ = RepeatMode::Off;
            break;
        default:
            break;
    }
}