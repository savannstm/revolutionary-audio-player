#include "playlisttab.hpp"

#include "playlisttablabel.hpp"

#include <QMenu>

constexpr QMargins TAB_MARGINS = { 8, 4, 8, 4 };
constexpr u8 LINE_EDIT_MARGIN = 8;

PlaylistTab::PlaylistTab(const QString& text, bool closable, QWidget* parent) :
    QPushButton(parent),
    label_(new PlaylistTabLabel(text, parent)) {
    layout_->setContentsMargins(TAB_MARGINS);

    connect(
        label_,
        &PlaylistTabLabel::clicked,
        this,
        &PlaylistTab::handleMousePress
    );

    connect(label_, &PlaylistTabLabel::rightClicked, this, [&] {
        // TODO: Implement actions

        auto* menu = new QMenu(this);
        auto* removeAction = new QAction("Remove");

        menu->addAction(removeAction);
        auto* selectedAction =
            menu->exec(mapToGlobal(QPoint(0, this->height())));

        menu->deleteLater();

        if (selectedAction == removeAction) {}
    });

    if (closable) {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        connect(
            tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::closeButtonClicked
        );
        layout_->addWidget(label_);
    } else {
        tabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        connect(
            tabButton,
            &QToolButton::clicked,
            this,
            &PlaylistTab::addButtonClicked
        );
        addTab = true;
    }

    tabButton->setFixedSize(CLOSE_BUTTON_SIZE);
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