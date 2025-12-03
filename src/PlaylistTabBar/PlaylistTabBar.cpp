#include "PlaylistTabBar.hpp"

#include "PlaylistTab.hpp"

#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QMimeData>
#include <QScrollArea>

PlaylistTabBar::PlaylistTabBar(QWidget* const parent) :
    QWidget(parent),
    indicatorLine(new QFrame(this)),
    scrollArea(new QScrollArea(this)),
    tabContainer(new QWidget(scrollArea)),
    tabContainerLayout(new QHBoxLayout(tabContainer)),
    mainLayout(new QHBoxLayout(this)) {
    setAcceptDrops(true);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    tabContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    tabContainerLayout->setContentsMargins(0, 0, 0, 0);
    tabContainerLayout->setSpacing(4);
    tabContainerLayout->setAlignment(Qt::AlignLeft);

    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setContentsMargins(0, 0, 0, 0);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    scrollArea->setMinimumWidth(screen()->size().width());
    scrollArea->setWidget(tabContainer);
    mainLayout->addWidget(scrollArea);

    indicatorLine->setFrameShape(QFrame::VLine);
    indicatorLine->setFrameShadow(QFrame::Plain);
    indicatorLine->setLineWidth(2);
    indicatorLine->hide();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    insertTab(0, QString(), false);  // insert add tab
}

PlaylistTabBar::~PlaylistTabBar() {
    qDeleteAll(tabs_);
}

void PlaylistTabBar::insertTab(
    const u8 index,
    QString label,
    const bool closable
) {
    if (closable && label.isEmpty()) {
        label = tr("Playlist ") + QString::number(tabCount());
    }

    auto* const tab = new PlaylistTab(label, closable, this);

    tabs_.insert(index, tab);
    tabContainerLayout->insertWidget(index, tab);

    if (closable) {
        connect(
            tab,
            &PlaylistTab::removeTabRequested,
            this,
            [this, tab] -> void {
            removeTabs(TabRemoveMode::Single, tabIndex(tab));
        }
        );

        connect(tab, &PlaylistTab::clicked, this, [this, tab] -> void {
            setCurrentIndex(tabIndex(tab));
        });

        connect(
            tab,
            &PlaylistTab::removeTabsRequested,
            this,
            [this, tab](const TabRemoveMode mode) -> void {
            removeTabs(mode, tabIndex(tab));
        }
        );
    } else {
        connect(tab, &PlaylistTab::addButtonClicked, this, [this] -> void {
            insertTab(tabCount(), QString(), true);
        });
    }

    emit tabAdded(tabCount() - 1);

    if (tabCount() == 1) {
        setCurrentIndex(0);
    }
}

void PlaylistTabBar::setCurrentIndex(const i8 index) {
    emit indexChanged(index);

    if (index == -1) {
        currentIndex_ = -1;
        return;
    }

    if (previousIndex != -1 && tabs_.size() > previousIndex) {
        tabs_[previousIndex]->setChecked(false);
    }

    currentIndex_ = index;
    previousIndex = index;
    tabs_[index]->setChecked(true);
}

void PlaylistTabBar::removeTabs(const TabRemoveMode mode, const u8 startIndex) {
    const u8 count = tabCount();

    if (mode != TabRemoveMode::Single && count == 0) {
        return;
    }

    i8 newIndex = currentIndex_;

    switch (mode) {
        case TabRemoveMode::Single:
            removeTab(startIndex);

            if (newIndex == 0) {
                if (count > 1) {
                    newIndex = 0;
                } else {
                    newIndex -= 1;
                }
            } else if (startIndex < newIndex) {
                newIndex -= 1;
            }
            break;
        case TabRemoveMode::Other:
        case TabRemoveMode::ToRight:
            for (i8 i = i8(count - 1); i > startIndex; i--) {
                removeTab(i);
            }

            if (mode == TabRemoveMode::ToRight) {
                break;
            }
        case TabRemoveMode::ToLeft:
            for (i8 i = i8(startIndex - 1); i >= 0; i--) {
                if (i < newIndex) {
                    newIndex -= 1;
                }

                removeTab(i);
            }
            break;
    }

    setCurrentIndex(newIndex);
    emit tabsRemoved(mode, startIndex);
}

void PlaylistTabBar::dropEvent(QDropEvent* const event) {
    const QPoint position = event->position().toPoint();
    const QString name = event->mimeData()->text();

    auto* const dragged = findChild<QWidget*>(name);

    if (dragged == nullptr) {
        emit filesDropped(event);
        return;
    }

    const u8 oldIndex = tabContainerLayout->indexOf(dragged);
    tabContainerLayout->removeWidget(dragged);

    u8 insertIndex = tabContainerLayout->count();

    for (const u8 idx : range(0, tabContainerLayout->count())) {
        const QWidget* const widget = tabContainerLayout->itemAt(idx)->widget();

        if (position.x() < widget->x() + (widget->width() / 2)) {
            insertIndex = idx;
            break;
        }
    }

    indicatorLine->hide();
    tabContainerLayout->insertWidget(insertIndex, dragged);

    event->acceptProposedAction();
}

void PlaylistTabBar::dragMoveEvent(QDragMoveEvent* const event) {
    const QPoint position = event->position().toPoint();
    u8 index = UINT8_MAX;

    for (const u8 idx : range(0, tabContainerLayout->count())) {
        const QWidget* widget = tabContainerLayout->itemAt(idx)->widget();

        if (position.x() < widget->x() + (widget->width() / 2)) {
            index = idx;
            break;
        }
    }

    if (index != UINT8_MAX) {
        if (index < tabContainerLayout->count()) {
            const QWidget* widget = tabContainerLayout->itemAt(index)->widget();
            const QPoint pos = widget->pos();

            indicatorLine->move(
                pos.x() - (indicatorLine->width() / 2),
                indicatorLine->height() / 2
            );
        } else {
            const u16 endPosX = tabContainerLayout->geometry().right();
            indicatorLine->move(endPosX, 0);
        }

        indicatorLine->show();
    } else {
        indicatorLine->hide();
    }

    event->acceptProposedAction();
}

void PlaylistTabBar::addTab(const QString& label) {
    insertTab(tabCount(), label, true);
};

void PlaylistTabBar::removeTab(const u8 index) {
    PlaylistTab* const tab = tabs_.takeAt(index);
    tabContainerLayout->removeWidget(tab);
    delete tab;
};

void PlaylistTabBar::setScrollAreaWidth(const u16 width) {
    scrollArea->setMinimumWidth(width);
}

void PlaylistTabBar::setTabColor(const u8 index, const QString& color) {
    tabs_[index]->setColor(color);
}

[[nodiscard]] auto PlaylistTabBar::tabColor(const u8 index) const
    -> const QString& {
    return tabs_[index]->color();
}

void PlaylistTabBar::setTabLabel(const u8 index, const QString& label) {
    tabs_[index]->setLabelText(label);
};

[[nodiscard]] auto PlaylistTabBar::tabLabel(const u8 index) const -> QString {
    return tabs_[index]->labelText();
};

void PlaylistTabBar::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}
