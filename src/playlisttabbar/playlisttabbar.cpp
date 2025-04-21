#include "playlisttabbar.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QToolButton>

using namespace Qt::Literals::StringLiterals;

PlaylistTabBar::PlaylistTabBar(QTabWidget* parent) : QTabBar(parent) {
    setTabsClosable(true);
    addAddTab();
}

void PlaylistTabBar::addAddTab() {
    const i32 addTabIndex = QTabBar::addTab("");
    auto* addButton = new QToolButton();
    addButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
    addButton->setFixedSize(BUTTON_SIZE);

    connect(addButton, &QToolButton::pressed, this, [this] {
        emit playlistAdded();
    });

    setTabButton(addTabIndex, ButtonPosition::RightSide, addButton);
}

auto PlaylistTabBar::addPlaylistTab(const QString& label) -> i32 {
    return insertPlaylistTab(count() - 1, label);
}

auto PlaylistTabBar::insertPlaylistTab(i32 index, const QString& label) -> i32 {
    QTabBar::insertTab(index, label);
    attachCloseButton(index);
    return index;
}

void PlaylistTabBar::attachCloseButton(i32 index) {
    if (!tabsClosable() || isAddTab(index)) {
        return;
    }

    auto* closeButton = new QToolButton();
    closeButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
    closeButton->setFixedSize(BUTTON_SIZE);

    connect(closeButton, &QToolButton::pressed, this, [this, closeButton] {
        removeTabByCloseButton(closeButton);
    });

    setTabButton(index, ButtonPosition::RightSide, closeButton);
}

void PlaylistTabBar::removeTabByCloseButton(QToolButton* button) {
    for (i32 i = 0; i < count(); i++) {
        if (tabButton(i, ButtonPosition::RightSide) == button) {
            if (!isAddTab(i)) {
                removeTab(i);
            }
            break;
        }
    }
}

auto PlaylistTabBar::isAddTab(const i32 index) const -> bool {
    return index == count() - 1;
}

void PlaylistTabBar::mousePressEvent(QMouseEvent* event) {
    const i32 clickedIndex = tabAt(event->pos());

    if (isAddTab(clickedIndex)) {
        return;
    }

    if (event->button() == Qt::RightButton && clickedIndex != -1) {
        QMenu contextMenu;
        const QAction* renameAction = contextMenu.addAction("Rename Tab");

        QAction* selected = contextMenu.exec(event->globalPosition().toPoint());
        if (selected == renameAction) {
            emit renameTabRequested(clickedIndex);
        }
        return;
    }

    QTabBar::mousePressEvent(event);
}