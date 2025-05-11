#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidget>

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistView(QWidget* parent = nullptr);

    [[nodiscard]] auto addTab(
        const QString& label = QString(),
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties =
            TRACK_TREE_DEFAULT_COLUMN_PROPERTIES,
        const array<bool, TRACK_PROPERTY_COUNT>& columnStates =
            TRACK_TREE_DEFAULT_COLUMN_STATES
    ) -> i8;
    [[nodiscard]] auto tabCount() const -> i8;
    void setCurrentIndex(i8 index);
    [[nodiscard]] auto currentIndex() const -> i8;
    void createTabPage(i8 index);
    [[nodiscard]] auto createPage(
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties =
            TRACK_TREE_DEFAULT_COLUMN_PROPERTIES,
        const array<bool, TRACK_PROPERTY_COUNT>& columnStates =
            TRACK_TREE_DEFAULT_COLUMN_STATES
    ) -> QWidget*;
    void setBackgroundImage(i8 index, const QString& path) const;
    [[nodiscard]] auto backgroundImage(i8 index) const -> QLabel*;
    [[nodiscard]] auto currentBackgroundImage() const -> QLabel*;
    [[nodiscard]] auto tree(i8 index) const -> TrackTree*;
    [[nodiscard]] auto currentTree() const -> TrackTree*;
    [[nodiscard]] auto tabLabel(i8 index) const -> QString;
    void setTabLabel(i8 index, const QString& label);
    [[nodiscard]] auto currentPage() const -> QWidget*;
    [[nodiscard]] auto page(i8 index) const -> QWidget*;
    void removeBackgroundImage(i8 index) const;
    [[nodiscard]] auto backgroundImagePath(i8 index) const -> QString;
    [[nodiscard]] auto hasBackgroundImage(i8 index) const -> bool;
    void removeTabPages(TabRemoveMode mode, i8 index);
    void removeTabPage(i8 index);

   signals:
    void renameTabRequested(i8 index);
    void closeTabRequested(i8 index);
    void indexChanged(i8 index);
    void tabRemoved(i8 index);

   private:
    void changePage(i8 index);
    static inline auto exportXSPF(
        const QString& outputPath,
        const vector<MetadataMap>& metadataVector
    ) -> result<bool, QString>;
    static inline auto exportM3U8(
        const QString& outputPath,
        const vector<MetadataMap>& metadataVector
    ) -> result<bool, QString>;
    inline void exportPlaylist(i8 index, PlaylistFileType playlistType);
    inline void importPlaylist(PlaylistFileType playlistType);

    static constexpr auto trackPropertyToTag(TrackProperty property)
        -> QString {
        switch (property) {
            case TrackProperty::Title:
                return "title";
            case TrackProperty::Artist:
                return "creator";
            case TrackProperty::Album:
                return "album";
            case TrackProperty::AlbumArtist:
                return "albumArtist";
            case TrackProperty::Genre:
                return "genre";
            case TrackProperty::Duration:
                return "duration";
            case TrackProperty::TrackNumber:
                return "trackNum";
            case TrackProperty::Comment:
                return "annotation";
            case TrackProperty::Path:
                return "location";
            case TrackProperty::Composer:
                return "composer";
            case TrackProperty::Publisher:
                return "publisher";
            case TrackProperty::Year:
                return "year";
            case TrackProperty::BPM:
                return "bpm";
            case TrackProperty::Language:
                return "language";
            case TrackProperty::DiscNumber:
                return "disc";
            case TrackProperty::Bitrate:
                return "bitrate";
            case TrackProperty::SampleRate:
                return "samplerate";
            case TrackProperty::Channels:
                return "channels";
            case TrackProperty::Format:
                return "format";
            default:
                return {};
                break;
        }
    };

    PlaylistTabBar* tabBar = new PlaylistTabBar(this);
    QStackedWidget* stackedWidget = new QStackedWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
};