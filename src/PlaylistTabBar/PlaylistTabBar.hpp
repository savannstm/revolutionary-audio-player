#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QWidget>

class PlaylistTabBar : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistTabBar(QWidget* parent = nullptr);
    ~PlaylistTabBar() override;

    void addTab(const QString& label);

    void insertTab(u8 index, QString label, bool closable);

    void removeTab(u8 index);

    void setCurrentIndex(i8 index);

    [[nodiscard]] constexpr auto currentIndex() const -> i8 {
        return currentIndex_;
    }

    [[nodiscard]] auto tabCount() const -> u8 {
        return u8(tabs_.size() - 1);  // ignore the add tab
    };

    [[nodiscard]] auto tabLabel(u8 index) const -> QString;

    void setTabLabel(u8 index, const QString& label);

    [[nodiscard]] constexpr auto minimumSizeHint() const -> QSize override {
        return { 0, 0 };
    }

    void setScrollAreaWidth(u16 width);

    [[nodiscard]] auto tabColor(u8 index) const -> const QString&;

    void setTabColor(u8 index, const QString& color);

    [[nodiscard]] auto tabAt(const u8 index) const -> PlaylistTab* {
        return tabs_[index];
    };

    [[nodiscard]] auto tabIndex(const PlaylistTab* tab) const -> i8 {
        return i8(tabs_.indexOf(tab));
    };

    [[nodiscard]] auto tabs() const -> const QVector<PlaylistTab*>& {
        return tabs_;
    }

   signals:
    void indexChanged(i8 index);
    void tabAdded(u8 index);
    void addButtonClicked(u8 index);
    void tabsRemoved(TabRemoveMode mode, u8 index);
    void filesDropped(QDropEvent* event);

   protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   private:
    inline void removeTabs(TabRemoveMode mode, u8 index);

    QFrame* const indicatorLine;
    QScrollArea* const scrollArea;
    QWidget* const tabContainer;
    QHBoxLayout* const tabContainerLayout;
    QHBoxLayout* const mainLayout;

    QVector<PlaylistTab*> tabs_;

    i8 currentIndex_ = -1;
    i8 previousIndex = -1;
};
