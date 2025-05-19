#include "playlisttab.hpp"

#include "aliases.hpp"
#include "constants.hpp"
#include "playlisttablabel.hpp"

#include <QDrag>
#include <QMenu>
#include <QMimeData>

PlaylistTab::PlaylistTab(const QString& text, bool closable, QWidget* parent) :
    QPushButton(parent),
    label_(new PlaylistTabLabel(text, parent)) {
    setObjectName(text);
    setContextMenuPolicy(Qt::CustomContextMenu);

    layout_->setContentsMargins(TAB_MARGINS);
    label_->installEventFilter(this);

    if (closable) {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        layout_->addWidget(label_);

        connect(this, &QPushButton::released, this, &PlaylistTab::selectTab);

        connect(
            this,
            &QPushButton::customContextMenuRequested,
            this,
            &PlaylistTab::createContextMenu
        );

        connect(
            tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::removeTabRequested
        );

        connect(
            label_,
            &QLineEdit::returnPressed,
            this,
            &PlaylistTab::deselectLabel
        );
    } else {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        addTab = true;

        setStyleSheet("border: none;");
        tabButton->setContextMenuPolicy(Qt::CustomContextMenu);

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
}

auto PlaylistTab::labelTextWidth() -> i32 {
    return QFontMetrics(label_->font()).horizontalAdvance(label_->text()) +
           TAB_LABEL_RIGHT_MARGIN;
}

void PlaylistTab::selectTab() {
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

auto PlaylistTab::eventFilter(QObject* obj, QEvent* event) -> bool {
    if (obj == label_) {
        switch (event->type()) {
            case QEvent::MouseButtonRelease: {
                const auto* mouseEvent = as<QMouseEvent*>(event);

                if ((mouseEvent->buttons() & Qt::LeftButton) != 0) {
                    return false;
                }

                selectTab();
                return true;
                break;
            }
            case QEvent::FocusOut:
                deselectLabel();
                return true;
                break;
            case QEvent::ContextMenu:
                createContextMenu();
                return true;
                break;
            case QEvent::MouseMove: {
                const auto* mouseEvent = as<QMouseEvent*>(event);

                if ((mouseEvent->buttons() & Qt::LeftButton) != 0) {
                    return false;
                }

                grab();
                return true;
                break;
            }
            default:
                break;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void PlaylistTab::deselectLabel() {
    setObjectName(label_->text());

    label_->deselect();
    label_->setReadOnly(true);
    label_->setStyleSheet("background: transparent;");
    label_->setFixedWidth(labelTextWidth());

    setFixedSize(layout_->sizeHint());
}

void PlaylistTab::mouseMoveEvent(QMouseEvent* event) {
    grab();
    QPushButton::mouseMoveEvent(event);
}

void PlaylistTab::grab() {
    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData();

    mimeData->setText(objectName());
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
}