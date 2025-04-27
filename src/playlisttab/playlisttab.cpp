#include "playlisttab.hpp"

#include "playlisttablabel.hpp"

#include <QMenu>

constexpr QMargins TAB_MARGINS = { 8, 4, 8, 4 };
constexpr u8 LINE_EDIT_MARGIN = 8;

PlaylistTab::PlaylistTab(const QString& text, bool closable, QWidget* parent) :
    QPushButton(parent),
    label_(PlaylistTabLabel(text, parent)) {
    layout_.setContentsMargins(TAB_MARGINS);

    connect(
        &label_,
        &PlaylistTabLabel::clicked,
        this,
        &PlaylistTab::handleMousePress
    );

    connect(&label_, &PlaylistTabLabel::unfocused, this, [&] {
        label_.deselect();
        label_.setReadOnly(true);
        label_.setStyleSheet("background: transparent;");
        label_.setFixedWidth(labelTextWidth());

        setFixedSize(layout_.sizeHint());
    });

    connect(&label_, &PlaylistTabLabel::rightClicked, this, [&] {
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

        if (selectedAction == renameTabAction) {
            label_.setReadOnly(false);
            label_.setStyleSheet("");
            label_.setFocus();
            label_.selectAll();
        } else if (selectedAction != nullptr) {
            emit removeTabs(
                selectedAction == removeToLeftAction    ? RemoveMode::ToLeft
                : selectedAction == removeToRightAction ? RemoveMode::ToRight
                                                        : RemoveMode::Other
            );
        }

        delete menu;
    });

    if (closable) {
        tabButton.setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        layout_.addWidget(&label_);
        connect(
            &tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::removeTabRequested
        );
    } else {
        tabButton.setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        addTab = true;
        connect(
            &tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::addButtonClicked
        );
    }

    tabButton.setFixedSize(CLOSE_BUTTON_SIZE);
    layout_.addWidget(&tabButton);

    label_.setFixedWidth(labelTextWidth());
    setFixedSize(layout_.sizeHint());

    setCheckable(true);

    connect(&label_, &QLineEdit::returnPressed, this, [this] {
        label_.setReadOnly(true);
        label_.setStyleSheet("background: transparent;");
        label_.setFixedWidth(labelTextWidth());

        setFixedSize(layout_.sizeHint());
    });
}

auto PlaylistTab::labelTextWidth() -> i32 {
    return QFontMetrics(label_.font()).horizontalAdvance(label_.text()) +
           LINE_EDIT_MARGIN;
}

void PlaylistTab::mousePressEvent(QMouseEvent* event) {
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

auto PlaylistTab::labelPtr() -> PlaylistTabLabel* {
    return &label_;
}