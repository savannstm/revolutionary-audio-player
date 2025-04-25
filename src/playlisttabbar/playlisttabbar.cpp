#include "playlisttabbar.hpp"

PlaylistTabBar::PlaylistTabBar(QWidget* parent) : QWidget(parent) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
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
    layout->insertWidget(index, tab);

    connect(
        tab,
        &PlaylistTab::clicked,
        this,
        &PlaylistTabBar::handleTabClicked
    );

    if (closable) {
        connect(tab, &PlaylistTab::closeButtonClicked, this, [=, this] {
            emit closeButtonClicked(static_cast<i8>(tabs.indexOf(tab)));
        });
    } else {
        connect(tab, &PlaylistTab::addButtonClicked, this, [this] {
            emit tabAdded(static_cast<i8>(count() - 1));
            addTab(u"Playlist %1"_s.arg(count()));
        });
    }

    if (count() == 2) {
        setCurrentIndex(0);
    }
}

void PlaylistTabBar::removeTab(const i8 index) {
    PlaylistTab* tab = tabs.takeAt(index);
    layout->removeWidget(tab);
    delete tab;

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

void PlaylistTabBar::handleTabClicked() {
    auto* senderTab = static_cast<PlaylistTab*>(sender());

    for (const auto& tab : tabs) {
        if (tab == senderTab) {
            continue;
        }

        tab->setChecked(false);
    }

    const i8 index = static_cast<i8>(tabs.indexOf(senderTab));

    if (index != -1) {
        setCurrentIndex(index);
    }
}

auto PlaylistTabBar::tabText(const i8 index) const -> QString {
    return tabs[index]->label();
}