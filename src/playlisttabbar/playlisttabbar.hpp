#pragma once

#include "aliases.hpp"

#include <QTabBar>
#include <QTabWidget>

constexpr QSize BUTTON_SIZE = QSize(16, 16);

class PlaylistTabBar : public QTabBar {
    Q_OBJECT

   public:
    explicit PlaylistTabBar(QTabWidget* parent = nullptr);

    auto addPlaylistTab(const QString& label) -> i32;
    auto insertPlaylistTab(i32 index, const QString& label) -> i32;

   signals:
    void playlistAdded();
    void renameTabRequested(i32 index);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    void addAddTab();
    void attachCloseButton(i32 index);
    void removeTabByCloseButton(QToolButton* button);
    [[nodiscard]] auto isAddTab(i32 index) const -> bool;
};
