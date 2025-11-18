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

    void addTab(const QString& label) { insertTab(tabCount(), label, true); };

    void insertTab(u8 index, QString label, bool closable);

    void removeTab(const u8 index) {
        PlaylistTab* const tab = tabs.takeAt(index);
        tabContainerLayout->removeWidget(tab);
        delete tab;
    };

    void setCurrentIndex(i8 index);

    [[nodiscard]] constexpr auto currentIndex() const -> i8 {
        return currentIndex_;
    }

    [[nodiscard]] auto tabCount() const -> u8 {
        return u8(tabs.size() - 1);  // ignore the add tab
    };

    [[nodiscard]] auto tabLabel(const u8 index) const -> QString {
        return tabs[index]->labelText();
    };

    void setTabLabel(const u8 index, const QString& label) {
        tabs[index]->setLabelText(label);
    };

    [[nodiscard]] constexpr auto minimumSizeHint() const -> QSize override {
        return { 0, 0 };
    }

    void setScrollAreaWidth(const u16 width) {
        scrollArea->setMinimumWidth(width);
    }

    [[nodiscard]] auto tabColor(const u8 index) const -> const QString& {
        return tabs[index]->color();
    }

    void setTabColor(const u8 index, const QString& color) {
        tabs[index]->setColor(color);
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

    [[nodiscard]] auto tabAt(const u8 index) const -> PlaylistTab* {
        return tabs[index];
    };

    [[nodiscard]] auto tabIndex(const PlaylistTab* tab) const -> i8 {
        return i8(tabs.indexOf(tab));
    };

    QFrame* const indicatorLine = new QFrame(this);
    QScrollArea* const scrollArea = new QScrollArea(this);
    QWidget* const tabContainer = new QWidget(scrollArea);
    QHBoxLayout* const tabContainerLayout = new QHBoxLayout(tabContainer);
    QHBoxLayout* const mainLayout = new QHBoxLayout(this);

    QVector<PlaylistTab*> tabs;

    i8 currentIndex_ = -1;
    i8 previousIndex = -1;
};
