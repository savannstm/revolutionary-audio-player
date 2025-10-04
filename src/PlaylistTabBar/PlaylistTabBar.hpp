#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "PlaylistTab.hpp"

#include <QHBoxLayout>
#include <QScrollArea>
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

    [[nodiscard]] auto tabCount() const -> u8;
    [[nodiscard]] auto tabLabel(u8 index) const -> QString;
    void setTabLabel(u8 index, const QString& label);

    [[nodiscard]] constexpr auto minimumSizeHint() const -> QSize override {
        return { 0, 0 };
    }

    void setScrollAreaWidth(const u16 width) {
        scrollArea->setMinimumWidth(width);
    }

   signals:
    void indexChanged(i8 index);
    void tabAdded(u8 index);
    void addButtonClicked(u8 index);
    void tabsRemoved(TabRemoveMode mode, u8 index);
    void filesDropped(QDropEvent* event);

   protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        event->acceptProposedAction();
    }

    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   private:
    void removeTabs(TabRemoveMode mode, u8 index);
    [[nodiscard]] auto tabAt(u8 index) const -> PlaylistTab*;
    [[nodiscard]] auto tabIndex(const PlaylistTab* tab) const -> i8;

    QFrame* indicatorLine = new QFrame(this);
    QScrollArea* scrollArea = new QScrollArea(this);
    QWidget* tabContainer = new QWidget(scrollArea);
    QHBoxLayout* tabContainerLayout = new QHBoxLayout(tabContainer);
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    QVector<PlaylistTab*> tabs;

    i8 currentIndex_ = -1;
    i8 previousIndex = -1;
};
