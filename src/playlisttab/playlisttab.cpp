#include "playlisttab.hpp"

#include "aliases.hpp"
#include "constants.hpp"
#include "playlisttablabel.hpp"

#include <QMenu>

PlaylistTab::PlaylistTab(const QString& text, bool closable, QWidget* parent) :
    QPushButton(parent),
    label_(new PlaylistTabLabel(text, parent)) {
    layout_->setContentsMargins(TAB_MARGINS);

    if (closable) {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        layout_->addWidget(label_);

        connect(
            tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::removeTabRequested
        );

        connect(
            label_,
            &PlaylistTabLabel::clicked,
            this,
            &PlaylistTab::handleMousePress
        );

        connect(label_, &PlaylistTabLabel::unfocused, this, [&] {
            label_->deselect();
            label_->setReadOnly(true);
            label_->setStyleSheet("background: transparent;");
            label_->setFixedWidth(labelTextWidth());

            setFixedSize(layout_->sizeHint());
        });

        connect(
            this,
            &PlaylistTab::rightClicked,
            this,
            &PlaylistTab::createContextMenu
        );

        connect(
            label_,
            &PlaylistTabLabel::rightClicked,
            this,
            &PlaylistTab::createContextMenu
        );
    } else {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        addTab = true;

        setStyleSheet("border: none;");
        tabButton->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(
            tabButton,
            &QToolButton::customContextMenuRequested,
            this,
            [=, this] {
            auto* menu = new QMenu(this);

            const QAction* importXSPFAction =
                menu->addAction(tr("Import .xspf"));
            const QAction* importM3U8Action =
                menu->addAction(tr("Import .m3u8"));

            const QAction* selectedAction =
                menu->exec(mapToGlobal(QPoint(0, this->height())));
            delete menu;

            if (selectedAction == nullptr) {
                return;
            }

            emit importPlaylistRequested(
                selectedAction == importXSPFAction ? XSPF : M3U8
            );
        }
        );

        connect(
            tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::addButtonClicked
        );
    }

    tabButton->setFixedSize(PLAYLIST_TAB_CLOSE_BUTTON_SIZE);
    layout_->addWidget(tabButton);

    label_->setFixedWidth(labelTextWidth());
    setFixedSize(layout_->sizeHint());

    setCheckable(true);

    connect(label_, &QLineEdit::returnPressed, this, [this] {
        label_->setReadOnly(true);
        label_->setStyleSheet("background: transparent;");
        label_->setFixedWidth(labelTextWidth());

        setFixedSize(layout_->sizeHint());
    });
}

auto PlaylistTab::labelTextWidth() -> i32 {
    return QFontMetrics(label_->font()).horizontalAdvance(label_->text()) +
           TAB_LABEL_RIGHT_MARGIN;
}

void PlaylistTab::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        emit rightClicked();
        return;
    }

    handleMousePress();
}

void PlaylistTab::handleMousePress() {
    if (!addTab) {
        if (!this->isChecked()) {
            this->setChecked(true);
        }

        emit clicked();
    }
}

void PlaylistTab::createContextMenu() {
    auto* menu = new QMenu(this);

    const QAction* renameTabAction = menu->addAction(tr("Rename Tab"));
    const QAction* removeToLeftAction =
        menu->addAction(tr("Remove All Tabs To Left"));
    const QAction* removeOtherTabs =
        menu->addAction(tr("Remove All Other Tabs"));
    const QAction* removeToRightAction =
        menu->addAction(tr("Remove All Tabs To Right"));
    const QAction* exportXSPFAction = menu->addAction(tr("Export .xspf"));
    const QAction* exportM3U8Action = menu->addAction(tr("Export .m3u8"));

    const QAction* selectedAction =
        menu->exec(mapToGlobal(QPoint(0, this->height())));
    delete menu;

    if (selectedAction == nullptr) {
        return;
    }

    if (selectedAction == renameTabAction) {
        label_->setReadOnly(false);
        label_->setStyleSheet(QString());
        label_->setFocus();
        label_->selectAll();
    } else if (selectedAction == exportXSPFAction ||
               selectedAction == exportM3U8Action) {
        emit exportPlaylistRequested(
            selectedAction == exportXSPFAction ? XSPF : M3U8
        );
    } else {
        // NOLINTBEGIN Code is kinda understanable
        emit removeTabsRequested(
            selectedAction == removeToLeftAction    ? TabRemoveMode::ToLeft
            : selectedAction == removeToRightAction ? TabRemoveMode::ToRight
                                                    : TabRemoveMode::Other
        );
        // NOLINTEND
    }
}