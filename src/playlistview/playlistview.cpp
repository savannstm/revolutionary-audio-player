#include "playlistview.hpp"

#include "aliases.hpp"
#include "playlisttabbar.hpp"
#include "trackproperties.hpp"
#include "tracktree.hpp"

#include <CImg.h>
#include <QLabel>

constexpr f32 HALF_TRANSPARENCY = 0.5;
constexpr f32 BLUR_SIGMA = 10;
constexpr u8 MINIMUM_SECTION_SIZE = 26;

using namespace cimg_library;

PlaylistView::PlaylistView(QWidget* parent) : QWidget(parent) {
    layout.setContentsMargins(0, 0, 0, 0);
    layout.addWidget(&tabBar, 0, Qt::AlignLeft);
    layout.addWidget(&stackedWidget);

    connect(
        &tabBar,
        &PlaylistTabBar::indexChanged,
        this,
        &PlaylistView::changePage
    );

    connect(
        &tabBar,
        &PlaylistTabBar::removeTabRequested,
        this,
        &PlaylistView::removeTab
    );

    connect(
        &tabBar,
        &PlaylistTabBar::tabAdded,
        this,
        &PlaylistView::createTabPage
    );

    connect(
        &tabBar,
        &PlaylistTabBar::removeTabs,
        this,
        &PlaylistView::removeTabs
    );
}

void PlaylistView::removeTabs(const RemoveMode mode, const i8 index) {
    switch (mode) {
        case ToLeft:
            for (i8 i = static_cast<i8>(index - 1); i >= 0; i--) {
                removeTab(i);
            }
            break;
        case ToRight:
            for (i8 i = static_cast<i8>(tabCount() - 2); i > index; i--) {
                removeTab(i);
            }
            break;
        case Other:
            for (i8 i = static_cast<i8>(tabCount() - 2); i > index; i--) {
                removeTab(i);
            }

            for (i8 i = static_cast<i8>(index - 1); i >= 0; i--) {
                removeTab(i);
            }
            break;
    }
}

auto PlaylistView::createPage() -> QWidget* {
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);

    pageLayout->setContentsMargins({ 0, 0, 0, 0 });

    auto* pageTree = new TrackTree(page);
    pageTree->setObjectName("tree");

    auto* centerBackgroundLabel = new QLabel(page);
    centerBackgroundLabel->setObjectName("centerLabel");

    auto* leftBackgroundLabel = new QLabel(page);
    leftBackgroundLabel->setObjectName("leftLabel");

    auto* rightBackgroundLabel = new QLabel(page);
    rightBackgroundLabel->setObjectName("rightLabel");

    QColor bgColor = this->palette().color(QPalette::Window);
    bgColor.setAlphaF(HALF_TRANSPARENCY);

    const QString css = QString("background-color: rgba(%1, %2, %3, %4);")
                            .arg(bgColor.red())
                            .arg(bgColor.green())
                            .arg(bgColor.blue())
                            .arg(QString::number(bgColor.alphaF(), 'f', 2));

    pageTree->setStyleSheet(css);
    pageLayout->addWidget(pageTree);

    auto* pageTreeModel = pageTree->model();

    pageTreeModel->setHorizontalHeaderLabels(trackPropertiesLabels());

    for (u8 column = 4; column < pageTreeModel->columnCount(); column++) {
        pageTree->hideColumn(column);
    }

    for (u8 column = 0; column < pageTreeModel->columnCount(); column++) {
        pageTreeModel
            ->setHeaderData(column, Qt::Horizontal, column, PropertyRole);
    }

    auto* pageTreeHeader = pageTree->header();

    pageTreeHeader->setDefaultAlignment(Qt::AlignLeft);
    pageTreeHeader->setSectionsClickable(true);
    pageTreeHeader->setSectionsMovable(true);
    pageTreeHeader->setSortIndicatorShown(true);
    pageTreeHeader->setSortIndicatorClearable(true);
    pageTreeHeader->setMinimumSectionSize(MINIMUM_SECTION_SIZE);
    pageTreeHeader->resizeSection(0, MINIMUM_SECTION_SIZE);
    pageTreeHeader->setSectionResizeMode(0, QHeaderView::Fixed);

    return page;
}

auto PlaylistView::backgroundImagePath(const i8 index) const -> QString {
    auto* centerLabel = backgroundImage(index);
    return centerLabel->property("path").toString();
}

auto PlaylistView::hasBackgroundImage(const i8 index) const -> bool {
    auto* centerLabel = backgroundImage(index);
    return centerLabel->property("path").isValid();
}

void PlaylistView::removeBackgroundImage(const i8 index) const {
    auto* centerLabel = backgroundImage(index);

    if (!hasBackgroundImage(index)) {
        return;
    }

    auto* pageWidget = page(index);
    auto* leftLabel = pageWidget->findChild<QLabel*>("leftLabel");
    auto* rightLabel = pageWidget->findChild<QLabel*>("rightLabel");

    delete centerLabel;
    delete leftLabel;
    delete rightLabel;

    centerLabel = new QLabel(pageWidget);
    leftLabel = new QLabel(pageWidget);
    rightLabel = new QLabel(pageWidget);

    centerLabel->setObjectName("centerLabel");
    leftLabel->setObjectName("leftLabel");
    rightLabel->setObjectName("rightLabel");
}

void PlaylistView::setBackgroundImage(
    const i8 index,
    const QString& path
) const {
    auto* centerLabel = backgroundImage(index);
    auto* pageWidget = page(index);

    const i32 layoutWidth = pageWidget->width();
    const i32 layoutHeight = pageWidget->height();

    CImg<u8> img = CImg(path.toStdString().c_str());
    CImg<u8> interleaved = img.get_permute_axes("cxyz").unroll('x');

    const QImage normalImage = QImage(
        interleaved.data(),
        img.width(),
        img.height(),
        static_cast<i32>(img.width() * img.spectrum()),
        QImage::Format_RGB888
    );

    QPixmap centerPixmap = QPixmap::fromImage(normalImage);
    centerPixmap =
        centerPixmap.scaledToHeight(layoutHeight, Qt::SmoothTransformation);

    centerLabel->setPixmap(centerPixmap);
    centerLabel->setFixedSize(centerPixmap.size());
    centerLabel->move(
        pageWidget->geometry().center().x() - (centerLabel->width() / 2),
        pageWidget->geometry().top()
    );

    centerLabel->lower();
    centerLabel->show();

    auto* leftLabel = pageWidget->findChild<QLabel*>("leftLabel");
    auto* rightLabel = pageWidget->findChild<QLabel*>("rightLabel");

    if (centerPixmap.width() < layoutWidth) {
        const i32 sideWidth = (layoutWidth - centerPixmap.width()) / 2;

        img.blur(BLUR_SIGMA);
        interleaved = img.get_permute_axes("cxyz").unroll('x');

        const QImage blurredImage = QImage(
            interleaved.data(),
            img.width(),
            img.height(),
            static_cast<i32>(img.width() * img.spectrum()),
            QImage::Format_RGB888
        );

        QPixmap blurredPixmap = QPixmap::fromImage(blurredImage);
        blurredPixmap = blurredPixmap.scaled(
            centerPixmap.size(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );

        const QPixmap leftSide =
            blurredPixmap.copy(0, 0, sideWidth, layoutHeight);
        const QPixmap rightSide = blurredPixmap.copy(
            blurredPixmap.width() - sideWidth,
            0,
            sideWidth,
            layoutHeight
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
    } else {
        leftLabel->hide();
        rightLabel->hide();
    }

    centerLabel->setProperty("path", path);
}

auto PlaylistView::currentTree() const -> TrackTree* {
    return stackedWidget.currentWidget()->findChild<TrackTree*>("tree");
}

auto PlaylistView::tree(const i8 index) const -> TrackTree* {
    return stackedWidget.widget(index)->findChild<TrackTree*>("tree");
}

auto PlaylistView::currentBackgroundImage() const -> QLabel* {
    return stackedWidget.currentWidget()->findChild<QLabel*>("centerLabel");
}

auto PlaylistView::backgroundImage(const i8 index) const -> QLabel* {
    return stackedWidget.widget(index)->findChild<QLabel*>("centerLabel");
}

auto PlaylistView::currentPage() const -> QWidget* {
    return stackedWidget.currentWidget();
}

auto PlaylistView::page(const i8 index) const -> QWidget* {
    return stackedWidget.widget(index);
}

void PlaylistView::createTabPage(const i8 index) {
    stackedWidget.insertWidget(index, createPage());
}

auto PlaylistView::addTab(const QString& label) -> i8 {
    const i8 index = static_cast<i8>(stackedWidget.addWidget(createPage()));
    tabBar.addTab(label);
    return index;
}

void PlaylistView::removeTab(const i8 index) {
    auto* widget = stackedWidget.widget(index);
    stackedWidget.removeWidget(widget);
    delete widget;

    tabBar.removeTab(index);
    emit tabRemoved(index);
}

auto PlaylistView::tabCount() const -> i8 {
    return tabBar.count();
}

void PlaylistView::setCurrentIndex(const i8 index) {
    tabBar.setCurrentIndex(index);
}

auto PlaylistView::currentIndex() const -> i8 {
    return tabBar.currentIndex();
}

void PlaylistView::changePage(const i8 index) {
    stackedWidget.setCurrentIndex(index);
    emit indexChanged(index);
}

auto PlaylistView::tabText(const i8 index) const -> QString {
    return tabBar.tabText(index);
}