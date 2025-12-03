#include "PlaylistView.hpp"

#include "PlaylistTabBar.hpp"
#include "Settings.hpp"
#include "TrackTree.hpp"
#include "TrackTreeHeader.hpp"
#include "TrackTreeModel.hpp"
#include "Utils.hpp"

#include <QDir>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QScreen>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>

class CustomDelegate : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(TRACK_TREE_ROW_HEIGHT);
        return size;
    }
};

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

auto compactAndFill(array<TrackProperty, TRACK_PROPERTY_COUNT>& arr) -> u8 {
    array<TrackProperty, TRACK_PROPERTY_COUNT> result;
    u8 writeIndex = 0;
    HashSet<u8> seen;
    seen.reserve(TRACK_PROPERTY_COUNT);

    for (const TrackProperty property : arr) {
        const u8 value = u8(property);

        if (value != 0) {
            result[writeIndex++] = property;
            seen.insert(value);
        }
    }

    u8 candidate = 1;
    for (u8 i = writeIndex; i < TRACK_PROPERTY_COUNT; i++) {
        while (seen.contains(candidate)) {
            candidate++;
        }

        result[i] = TrackProperty(candidate++);
    }

    arr = result;
    return seen.size();
}

auto PlaylistView::createPage(
    optional<array<TrackProperty, TRACK_PROPERTY_COUNT>> defaultColumns
) -> QWidget* {
    auto* const page = new QWidget(this);
    auto* const pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto* const pageTree = new TrackTree(page);
    pageTree->setObjectName(u"tree"_qsv);

    auto* const centerBackgroundLabel = new QLabel(page);
    centerBackgroundLabel->setObjectName(u"centerLabel"_qsv);

    auto* const leftBackgroundLabel = new QLabel(page);
    leftBackgroundLabel->setObjectName(u"leftLabel"_qsv);

    auto* const rightBackgroundLabel = new QLabel(page);
    rightBackgroundLabel->setObjectName(u"rightLabel"_qsv);

    pageLayout->addWidget(pageTree);

    TrackTreeModel* const pageTreeModel = pageTree->model();
    const QStringList propertyLabels = trackPropertiesLabels();

    pageTreeModel->setColumnCount(TRACK_PROPERTY_COUNT);
    pageTreeModel->setHeaderData(
        0,
        Qt::Horizontal,
        QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart),
        Qt::DecorationRole
    );
    pageTreeModel->setHeaderData(0, Qt::Horizontal, propertyLabels[0]);
    pageTreeModel->setHeaderData(
        0,
        Qt::Horizontal,
        u8(TrackProperty::Play),
        PROPERTY_ROLE
    );

    if (!defaultColumns) {
        defaultColumns = settings->playlist.defaultColumns;
    }

    const u8 shownColumns = compactAndFill(defaultColumns.value()) + 1;

    for (const u8 idx : range(1, TRACK_PROPERTY_COUNT)) {
        pageTreeModel->setHeaderData(
            idx,
            Qt::Horizontal,
            propertyLabels[u8(defaultColumns.value()[idx - 1])]
        );

        pageTreeModel->setHeaderData(
            idx,
            Qt::Horizontal,
            u8(defaultColumns.value()[idx - 1]),
            PROPERTY_ROLE
        );

        if (idx >= shownColumns) {
            pageTree->hideColumn(idx);
        }
    }

    TrackTreeHeader* const header = pageTree->header();
    header->setDefaultAlignment(Qt::AlignLeft);
    header->setSectionsClickable(true);
    header->setSectionsMovable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicatorClearable(true);
    header->setMinimumSectionSize(MINIMUM_TRACK_TREE_COLUMN_SECTION_SIZE);
    header->resizeSection(0, MINIMUM_TRACK_TREE_COLUMN_SECTION_SIZE);
    header->setSectionResizeMode(0, QHeaderView::Fixed);

    connect(
        header,
        &TrackTreeHeader::sectionMoved,
        this,
        [=](const u8, const u8 oldIndex, const u8 newIndex) -> void {
        const QVariant oldProperty =
            pageTreeModel->headerData(oldIndex, Qt::Horizontal, PROPERTY_ROLE);

        const QVariant newProperty =
            pageTreeModel->headerData(newIndex, Qt::Horizontal, PROPERTY_ROLE);

        pageTreeModel->setHeaderData(
            newIndex,
            Qt::Horizontal,
            oldProperty,
            PROPERTY_ROLE
        );

        pageTreeModel->setHeaderData(
            oldIndex,
            Qt::Horizontal,
            newProperty,
            PROPERTY_ROLE
        );
    }
    );

    pageTree->setItemDelegate(new CustomDelegate(pageTree));
    return page;
}

void PlaylistView::setTreeOpacity(const u8 index, const f32 opacity) const {
    this->tree(index)->setOpacity(opacity);
}

void PlaylistView::removeBackgroundImage(const u8 index) const {
    QLabel* centerLabel = backgroundImage(index);

    if (!hasBackgroundImage(index)) {
        return;
    }

    setTreeOpacity(index, 1.0F);

    QWidget* const pageWidget = page(index);
    auto* leftLabel = pageWidget->findChild<QLabel*>(u"leftLabel"_qsv);
    auto* rightLabel = pageWidget->findChild<QLabel*>(u"rightLabel"_qsv);

    delete centerLabel;
    delete leftLabel;
    delete rightLabel;

    centerLabel = new QLabel(pageWidget);
    leftLabel = new QLabel(pageWidget);
    rightLabel = new QLabel(pageWidget);

    centerLabel->setObjectName(u"centerLabel"_qsv);
    leftLabel->setObjectName(u"leftLabel"_qsv);
    rightLabel->setObjectName(u"rightLabel"_qsv);
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

    const f32 currentOpacity = treeOpacity(index);

    setTreeOpacity(
        index,
        treeOpacity(index) == 1.0F ? HALF_TRANSPARENT : currentOpacity
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

    auto* const leftLabel = pageWidget->findChild<QLabel*>(u"leftLabel"_qsv);
    auto* const rightLabel = pageWidget->findChild<QLabel*>(u"rightLabel"_qsv);

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

auto PlaylistView::treeOpacity(const u8 index) const -> f32 {
    return tree(index)->opacity();
}

void PlaylistView::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::PaletteChange) {
        for (const u8 idx : range(0, tabCount())) {
            TrackTree* tree = this->tree(idx);
            tree->setOpacity(tree->opacity());

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

[[nodiscard]] auto PlaylistView::currentBackgroundImage() const -> QLabel* {
    return stackedWidget->currentWidget()->findChild<QLabel*>(
        u"centerLabel"_qsv
    );
};

[[nodiscard]] auto PlaylistView::backgroundImage(const u8 index) const
    -> QLabel* {
    return stackedWidget->widget(index)->findChild<QLabel*>(u"centerLabel"_qsv);
};

[[nodiscard]] auto PlaylistView::tree(const u8 index) const -> TrackTree* {
    const QWidget* const widget = stackedWidget->widget(index);

    if (widget == nullptr) {
        return nullptr;
    }

    return widget->findChild<TrackTree*>(u"tree"_qsv);
};

[[nodiscard]] auto PlaylistView::currentTree() const -> TrackTree* {
    return stackedWidget->currentWidget()->findChild<TrackTree*>(u"tree"_qsv);
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
    const array<TrackProperty, TRACK_PROPERTY_COUNT>& defaultColumns
) -> u8 {
    const u8 index = u8(stackedWidget->addWidget(createPage(defaultColumns)));
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