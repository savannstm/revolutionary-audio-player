#include "playlistview.hpp"

#include "aliases.hpp"
#include "enums.hpp"
#include "playlisttabbar.hpp"
#include "trackproperties.hpp"
#include "tracktree.hpp"

#include <CImg.h>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QXmlStreamReader>

using namespace cimg_library;

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

PlaylistView::PlaylistView(QWidget* parent) : QWidget(parent) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabBar, 0, Qt::AlignLeft);
    layout->addWidget(stackedWidget);

    connect(
        tabBar,
        &PlaylistTabBar::indexChanged,
        this,
        &PlaylistView::changePage
    );

    connect(
        tabBar,
        &PlaylistTabBar::tabAdded,
        this,
        &PlaylistView::createTabPage
    );

    connect(
        tabBar,
        &PlaylistTabBar::tabsRemoved,
        this,
        &PlaylistView::removeTabPages
    );

    connect(
        tabBar,
        &PlaylistTabBar::exportPlaylistRequested,
        this,
        &PlaylistView::exportPlaylist
    );

    connect(
        tabBar,
        &PlaylistTabBar::importPlaylistRequested,
        this,
        &PlaylistView::importPlaylist
    );
}

void PlaylistView::removeTabPages(const TabRemoveMode mode, const u8 index) {
    u8 count = 0;

    switch (mode) {
        case Single:
            count += 1;
            removeTabPage(index);
            break;
        case Other:
        case ToRight:
            for (i8 i = as<i8>(stackedWidget->count() - 1); i > index; i--) {
                count += 1;
                removeTabPage(i);
            }

            if (mode == ToRight) {
                break;
            }
        case ToLeft:
            for (i8 i = as<i8>(index - 1); i >= 0; i--) {
                count += 1;
                removeTabPage(i);
            }
            break;
    }

    emit tabsRemoved(mode, index, count);
}

auto PlaylistView::createPage(
    const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties,
    const array<bool, TRACK_PROPERTY_COUNT>& columnStates
) -> QWidget* {
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto* pageTree = new TrackTree(page);
    pageTree->setObjectName("tree");

    auto* centerBackgroundLabel = new QLabel(page);
    centerBackgroundLabel->setObjectName("centerLabel");

    auto* leftBackgroundLabel = new QLabel(page);
    leftBackgroundLabel->setObjectName("leftLabel");

    auto* rightBackgroundLabel = new QLabel(page);
    rightBackgroundLabel->setObjectName("rightLabel");

    QColor bgColor = palette().color(QPalette::Window);
    bgColor.setAlphaF(HALF_TRANSPARENT);
    pageTree->setStyleSheet(
        u"background-color: rgba(%1, %2, %3, %4);"_s.arg(bgColor.red())
            .arg(bgColor.green())
            .arg(bgColor.blue())
            .arg(QString::number(bgColor.alphaF(), 'f', 2))
    );

    pageLayout->addWidget(pageTree);
    auto* pageTreeModel = pageTree->model();
    pageTreeModel->setColumnCount(TRACK_PROPERTY_COUNT);

    if (columnProperties.empty()) {
        pageTreeModel->setHorizontalHeaderLabels(trackPropertiesLabels());
    } else {
        const QStringList propertyLabelMap = trackPropertiesLabels();

        for (const u8 idx : range(0, TRACK_PROPERTY_COUNT)) {
            pageTreeModel->setHeaderData(
                idx,
                Qt::Horizontal,
                propertyLabelMap[columnProperties[idx]]
            );
        }
    }

    pageTreeModel->setHeaderData(
        0,
        Qt::Horizontal,
        QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart),
        Qt::DecorationRole
    );

    array<bool, TRACK_PROPERTY_COUNT> states =
        columnStates.empty() ? array<bool, TRACK_PROPERTY_COUNT>()
                             : columnStates;

    if (columnStates.empty()) {
        ranges::fill(states, true);
        ranges::fill_n(states.begin(), 4, true);
    }

    for (const auto [idx, state] : views::enumerate(states)) {
        if (state) {
            pageTree->hideColumn(as<i32>(idx));
        }
    }

    const array<TrackProperty, TRACK_PROPERTY_COUNT> properties =
        columnProperties.empty() ? array<TrackProperty, TRACK_PROPERTY_COUNT>()
                                 : columnProperties;

    for (const auto [idx, property] : views::enumerate(properties)) {
        pageTreeModel->setHeaderData(
            as<i32>(idx),
            Qt::Horizontal,
            columnProperties.empty() ? idx : property,
            PROPERTY_ROLE
        );
    }

    auto* header = pageTree->header();
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
        &MusicHeader::sectionMoved,
        this,
        [=](const u8, const u8 oldIndex, const u8 newIndex) {
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

auto PlaylistView::backgroundImagePath(const u8 index) const -> QString {
    const QLabel* centerLabel = backgroundImage(index);
    return centerLabel->property("path").toString();
}

auto PlaylistView::hasBackgroundImage(const u8 index) const -> bool {
    const QLabel* centerLabel = backgroundImage(index);
    return centerLabel->property("path").isValid();
}

void PlaylistView::removeBackgroundImage(const u8 index) const {
    QLabel* centerLabel = backgroundImage(index);

    if (!hasBackgroundImage(index)) {
        return;
    }

    QWidget* pageWidget = page(index);
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
    const u8 index,  // NOLINT
    const u16 height,
    const QString& path
) const {
    if (!QFile::exists(path)) {
        return;
    }

    QLabel* centerLabel = backgroundImage(index);
    QWidget* pageWidget = page(index);

    const u16 layoutWidth = screen()->size().width();

    CImg<u8> img = CImg(path.toStdString().c_str());
    CImg<u8> interleaved = img.get_permute_axes("cxyz").unroll('x');

    const QImage normalImage = QImage(
        interleaved.data(),
        img.width(),
        img.height(),
        as<u32>(img.width() * img.spectrum()),
        QImage::Format_RGB888
    );

    const QPixmap centerPixmap = QPixmap::fromImage(normalImage)
                                     .scaled(
                                         layoutWidth,
                                         height,
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

    auto* leftLabel = pageWidget->findChild<QLabel*>("leftLabel");
    auto* rightLabel = pageWidget->findChild<QLabel*>("rightLabel");

    const u16 sideWidth = (layoutWidth - centerPixmap.width()) / 2;

    img.blur(BLUR_SIGMA);
    interleaved = img.get_permute_axes("cxyz").unroll('x');

    const QImage blurredImage = QImage(
        interleaved.data(),
        img.width(),
        img.height(),
        as<u32>(img.width() * img.spectrum()),
        QImage::Format_RGB888
    );

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

auto PlaylistView::currentTree() const -> TrackTree* {
    return stackedWidget->currentWidget()->findChild<TrackTree*>("tree");
}

auto PlaylistView::tree(const u8 index) const -> TrackTree* {
    const auto* widget = stackedWidget->widget(index);

    if (widget == nullptr) {
        return nullptr;
    }

    return widget->findChild<TrackTree*>("tree");
}

auto PlaylistView::currentBackgroundImage() const -> QLabel* {
    return stackedWidget->currentWidget()->findChild<QLabel*>("centerLabel");
}

auto PlaylistView::backgroundImage(const u8 index) const -> QLabel* {
    return stackedWidget->widget(index)->findChild<QLabel*>("centerLabel");
}

auto PlaylistView::currentPage() const -> QWidget* {
    return stackedWidget->currentWidget();
}

auto PlaylistView::page(const u8 index) const -> QWidget* {
    return stackedWidget->widget(index);
}

void PlaylistView::createTabPage(const u8 index) {
    if (stackedWidget->widget(index) != nullptr) {
        return;
    }

    stackedWidget->insertWidget(index, createPage());
}

auto PlaylistView::addTab(
    const QString& label,
    const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties,
    const array<bool, TRACK_PROPERTY_COUNT>& columnStates
) -> u8 {
    const u8 index = as<u8>(
        stackedWidget->addWidget(createPage(columnProperties, columnStates))
    );
    tabBar->addTab(label);
    return index;
}

void PlaylistView::removeTabPage(const u8 index) {
    QWidget* widget = stackedWidget->widget(index);
    stackedWidget->removeWidget(widget);
    delete widget;
}

auto PlaylistView::tabCount() const -> u8 {
    return tabBar->tabCount();
}

void PlaylistView::setCurrentIndex(const i8 index) {
    tabBar->setCurrentIndex(index);
}

auto PlaylistView::currentIndex() const -> i8 {
    return tabBar->currentIndex();
}

void PlaylistView::changePage(const i8 index) {
    stackedWidget->setCurrentIndex(index);
    emit indexChanged(index);
}

auto PlaylistView::tabLabel(const u8 index) const -> QString {
    return tabBar->tabLabel(index);
}

void PlaylistView::setTabLabel(const u8 index, const QString& label) {
    tabBar->setTabLabel(index, label);
}

auto PlaylistView::exportXSPF(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    QFile file(outputPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return err(file.errorString());
    }

    QTextStream output(&file);
    output.setEncoding(QStringConverter::Utf8);

    output << R"(<?xml version="1.0" encoding="UTF-8"?>)";
    output << '\n';
    output << R"(<playlist version="1" xmlns="http://xspf.org/ns/0/">)";
    output << '\n';
    output << "<trackList>";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        output << "<track>";
        output << '\n';

        for (const auto& [property, value] : views::drop(metadata, 1)) {
            const QString tag = getXSPFTag(property);

            QString content = value;

            if (property == TrackProperty::Path) {
                content =
                    QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                        .relativeFilePath(value);
            }

            output << '<' << tag << '>' << content.toHtmlEscaped() << "</"
                   << tag << '>';
            output << '\n';
        }

        output << "</track>";
        output << '\n';
    }

    output << "</trackList>";
    output << '\n';
    output << "</playlist>";
    output << '\n';

    file.close();
    return true;
}

auto PlaylistView::exportM3U8(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    QFile outputFile = QFile(outputPath);

    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return err(outputFile.errorString());
    }

    QTextStream output(&outputFile);
    output.setEncoding(QStringConverter::Utf8);

    output << "#EXTM3U";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        const QString title = metadata.find(Title)->second;
        const QString artist = metadata.find(Artist)->second;
        const QString path = metadata.find(Path)->second;
        const QString durationStr = metadata.find(Duration)->second;

        u16 duration = 0;

        const QStringList strings = durationStr.split(':');
        const QString& minutes = strings[0];
        const QString& seconds = strings[1];

        duration += minutes.toUInt() * MINUTE_SECONDS;
        duration += seconds.toUInt();

        output << "#EXTINF:";
        output << duration << ',' << artist << " - " << title;
        output << '\n';

        output << QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                      .relativeFilePath(path);
        output << '\n';
    }

    outputFile.close();
    return true;
}

void PlaylistView::exportPlaylist(
    const u8 index,
    const PlaylistFileType playlistType
) {
    const auto* trackTree = tree(index);

    QString outputPath =
        QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));

    if (outputPath.isEmpty()) {
        return;
    }

    outputPath += u"/%1.%2"_s.arg(
        tabLabel(index),
        playlistType == XSPF ? "xspf" : "m3u8"
    );

    if (QFile(outputPath).exists()) {
        const auto pressedButton = QMessageBox::warning(
            this,
            tr("Playlist already exists"),
            tr("Playlist already exists. Rewrite it?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (pressedButton != QMessageBox::Yes) {
            return;
        }
    }

    const u16 rowCount = trackTree->model()->rowCount();
    vector<HashMap<TrackProperty, QString>> properties;
    properties.reserve(rowCount);

    for (const u16 row : range(0, rowCount)) {
        properties.emplace_back(trackTree->rowMetadata(row));
    }

    const auto result = playlistType == XSPF
                            ? exportXSPF(outputPath, properties)
                            : exportM3U8(outputPath, properties);

    if (!result) {
        const auto pressedButton = QMessageBox::warning(
            this,
            tr("Unable to export playlist"),
            tr("Error: %1").arg(result.error()),
            QMessageBox::Retry
        );

        if (pressedButton == QMessageBox::StandardButton::Retry) {
            exportPlaylist(index, playlistType);
        }
    }
};

void PlaylistView::importPlaylist(const PlaylistFileType playlistType) {
    const QString filter = playlistType == XSPF ? tr("XSPF Playlist (*.xspf)")
                                                : tr("M3U8 Playlist (*.m3u8)");

    const QUrl filePath = QFileDialog::getOpenFileUrl(
        this,
        tr("Open Playlist File"),
        QString(),
        filter
    );

    if (filePath.isEmpty()) {
        return;
    }

    QStringList filePaths;

    QFile file(filePath.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), file.errorString());
        return;
    }

    switch (playlistType) {
        case XSPF: {
            QXmlStreamReader xml(&file);

            while (!xml.atEnd()) {
                xml.readNext();

                if (xml.isStartElement() && xml.name() == "location") {
                    const QUrl url(xml.readElementText());

                    if (url.isLocalFile()) {
                        filePaths << url.toLocalFile();
                    }
                }
            }

            if (xml.hasError()) {
                QMessageBox::warning(this, tr("Error"), xml.errorString());
                return;
            }
            break;
        }
        case M3U8: {
            QTextStream input(&file);

            while (!input.atEnd()) {
                const QString line = input.readLine().trimmed();

                if (!line.isEmpty() && !line.startsWith('#')) {
                    filePaths << line;
                }
            }
            break;
        }
    }

    if (filePaths.isEmpty()) {
        QMessageBox::information(
            this,
            tr("No Files Found"),
            tr("No valid tracks were found in the playlist.")
        );
        return;
    }

    const QString fileName = filePath.fileName();
    const u8 index = addTab(fileName.sliced(0, fileName.lastIndexOf('.')));
    auto* trackTree = tree(index);
    trackTree->fillTable(filePaths);
}