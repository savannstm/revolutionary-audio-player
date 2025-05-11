#include "playlisttabbar.hpp"

#include "enums.hpp"
#include "playlisttab.hpp"

PlaylistTabBar::PlaylistTabBar(QWidget* parent) : QWidget(parent) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    insertTab(0, QString(), false);  // insert add tab
}

PlaylistTabBar::~PlaylistTabBar() {
    qDeleteAll(tabs);
}

void PlaylistTabBar::addTab(const QString& label) {
    insertTab(tabCount(), label, true);
}

void PlaylistTabBar::insertTab(
    const i8 index,
    QString label,
    const bool closable
) {
    if (closable && label.isEmpty()) {
        label = tr("Playlist %1").arg(tabCount());
    }

    auto* tab = new PlaylistTab(label, closable, this);

    tabs.insert(index, tab);
    layout->insertWidget(index, tab);

    if (closable) {
        connect(tab, &PlaylistTab::removeTabRequested, this, [=, this] {
            removeTab(tabIndex(tab));
        });

        connect(tab, &PlaylistTab::clicked, this, [=, this] {
            handleTabClicked(tab);
        });

        connect(
            tab,
            &PlaylistTab::removeTabsRequested,
            this,
            [=, this](const TabRemoveMode mode) {
            switch (mode) {
                case Other:
                case ToRight:
                    for (i8 i = as<i8>(tabCount() - 1); i > index; i--) {
                        removeTab(i);
                    }

                    if (mode == ToRight) {
                        break;
                    }
                case ToLeft:
                    for (i8 i = as<i8>(index - 1); i >= 0; i--) {
                        removeTab(i);
                    }
                    break;
            }

            emit tabsRemoved(mode, tabIndex(tab));
        }
        );

        connect(
            tab,
            &PlaylistTab::exportPlaylistRequested,
            this,
            [=, this](const PlaylistFileType playlistType) {
            emit exportPlaylistRequested(tabIndex(tab), playlistType);
        }
        );

    } else {
        connect(tab, &PlaylistTab::addButtonClicked, this, [=, this] {
            const i8 tabs = tabCount();
            insertTab(tabs, QString(), true);
        });

        connect(
            tab,
            &PlaylistTab::importPlaylistRequested,
            this,
            &PlaylistTabBar::importPlaylistRequested
        );
    }

    emit tabAdded(as<i8>(tabCount() - 1));

    if (tabCount() == 1) {
        previousIndex = 0;
        setCurrentIndex(0);
    }
}

void PlaylistTabBar::removeTab(const i8 index) {
    PlaylistTab* tab = tabs.takeAt(index);
    layout->removeWidget(tab);
    delete tab;

    if (previousIndex >= index) {
        previousIndex--;
    }

    emit tabRemoved(index);

    i8 nextIndex = currentIndex_;

    if (currentIndex_ == 0 && tabCount() != 0) {
        nextIndex = 0;
    } else if (currentIndex_ >= index) {
        nextIndex = as<i8>(currentIndex_ - 1);
    }

    setCurrentIndex(nextIndex);
}

void PlaylistTabBar::setCurrentIndex(const i8 index) {
    emit indexChanged(index);

    if (index == -1) {
        currentIndex_ = -1;
        return;
    }

    if (previousIndex != -1) {
        tabs[previousIndex]->setChecked(false);
    }

    currentIndex_ = index;
    previousIndex = index;
    tabs[index]->setChecked(true);
}

auto PlaylistTabBar::currentIndex() const -> i8 {
    return currentIndex_;
}

auto PlaylistTabBar::tabCount() const -> i8 {
    return as<i8>(tabs.size() - 1);  // ignore the add tab
}

auto PlaylistTabBar::tabAt(const i8 index) const -> PlaylistTab* {
    return tabs[index];
}

auto PlaylistTabBar::tabIndex(const PlaylistTab* tab) const -> i8 {
    return as<i8>(tabs.indexOf(tab));
}

void PlaylistTabBar::handleTabClicked(PlaylistTab* tab) {
    setCurrentIndex(tabIndex(tab));
}

auto PlaylistTabBar::tabLabel(const i8 index) const -> QString {
    return tabs[index]->label();
}

void PlaylistTabBar::setTabLabel(const i8 index, const QString& label) {
    tabs[index]->setLabel(label);
}
