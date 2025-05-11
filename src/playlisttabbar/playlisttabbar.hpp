#pragma once

#include "aliases.hpp"
#include "enums.hpp"
#include "playlisttab.hpp"

#include <QHBoxLayout>
#include <QWidget>

// TODO: Implement tab dragging
// TODO: Implement scrolling

class PlaylistTabBar : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistTabBar(QWidget* parent = nullptr);
    ~PlaylistTabBar() override;

    void addTab(const QString& label);
    void insertTab(i8 index, QString label, bool closable);
    void removeTab(i8 index);
    void setCurrentIndex(i8 index);
    [[nodiscard]] auto currentIndex() const -> i8;
    [[nodiscard]] auto tabCount() const -> i8;
    [[nodiscard]] auto tabLabel(i8 index) const -> QString;
    void setTabLabel(i8 index, const QString& label);

   signals:
    void indexChanged(i8 index);
    void tabAdded(i8 index);
    void tabRemoved(i8 index);
    void addButtonClicked(i8 index);
    void tabsRemoved(TabRemoveMode mode, i8 index);
    void exportPlaylistRequested(i8 index, PlaylistFileType playlistType);
    void importPlaylistRequested(PlaylistFileType playlistType);

   private:
    [[nodiscard]] auto tabAt(i8 index) const -> PlaylistTab*;
    [[nodiscard]] auto tabIndex(const PlaylistTab* tab) const -> i8;
    void handleTabClicked(PlaylistTab* tab);

    QHBoxLayout* layout = new QHBoxLayout(this);
    QVector<PlaylistTab*> tabs;
    i8 currentIndex_ = -1;
    i8 previousIndex = -1;
};
