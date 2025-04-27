#include "playlisttabbar.hpp"

#include "playlisttab.hpp"

#include <QMenu>

PlaylistTabBar::PlaylistTabBar(QWidget* parent) : QWidget(parent) {
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setSpacing(4);
    insertTab(0, "", false);  // insert add tab
}

PlaylistTabBar::~PlaylistTabBar() {
    qDeleteAll(tabs);
}

void PlaylistTabBar::addTab(const QString& label) {
    insertTab(static_cast<i8>(count() - 1), label, true);
}

void PlaylistTabBar::insertTab(
    const i8 index,
    const QString& label,
    const bool closable
) {
    auto* tab = new PlaylistTab(label, closable, this);

    tabs.insert(index, tab);
    layout.insertWidget(index, tab);

    connect(tab, &PlaylistTab::clicked, this, [=, this] {
        handleTabClicked(tab);
    });

    connect(
        tab,
        &PlaylistTab::removeTabs,
        this,
        [=, this](const RemoveMode mode) {
        emit removeTabs(mode, tabIndex(tab));
    }
    );

    if (closable) {
        connect(tab, &PlaylistTab::removeTabRequested, this, [=, this] {
            emit removeTabRequested(tabIndex(tab));
        });
    } else {
        connect(tab, &PlaylistTab::addButtonClicked, this, [=, this] {
            emit tabAdded(static_cast<i8>(count() - 1));
            addTab(tr("Playlist %1").arg(count()));
        });
    }

    if (count() == 2) {
        previousIndex = 0;
        setCurrentIndex(0);
    }
}

void PlaylistTabBar::removeTab(const i8 index) {
    PlaylistTab* tab = tabs.takeAt(index);
    layout.removeWidget(tab);
    delete tab;

    if (previousIndex > index) {
        previousIndex--;
    }

    setCurrentIndex(
        static_cast<i8>(
            currentIndex_ == count() - 1 ? currentIndex_ - 1 : currentIndex_
        )
    );
}

void PlaylistTabBar::setCurrentIndex(const i8 index) {
    if (index == -1) {
        return;
    }

    currentIndex_ = index;
    tabs[index]->setChecked(true);
    emit indexChanged(index);
}

auto PlaylistTabBar::currentIndex() const -> i8 {
    return currentIndex_;
}

auto PlaylistTabBar::count() const -> i8 {
    return static_cast<i8>(tabs.size());
}

auto PlaylistTabBar::tabAt(const i8 index) const -> PlaylistTab* {
    return tabs[index];
}

auto PlaylistTabBar::tabIndex(const PlaylistTab* tab) const -> i8 {
    return static_cast<i8>(tabs.indexOf(tab));
}

void PlaylistTabBar::handleTabClicked(PlaylistTab* tab) {
    if (previousIndex != -1) {
        tabAt(previousIndex)->setChecked(false);
    }

    const i8 newIndex = tabIndex(tab);
    setCurrentIndex(newIndex);

    previousIndex = newIndex;
}

auto PlaylistTabBar::tabText(const i8 index) const -> QString {
    return tabs[index]->label();
}