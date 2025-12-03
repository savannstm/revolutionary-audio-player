#include "PlaylistTab.hpp"

#include "Constants.hpp"
#include "PlaylistTabLabel.hpp"

#include <QColorDialog>
#include <QDrag>
#include <QHBoxLayout>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QToolButton>

PlaylistTab::PlaylistTab(
    const QString& text,
    bool closable,
    QWidget* const parent
) :
    QPushButton(parent),
    label_(new PlaylistTabLabel(text, parent)),
    tabButton(new QToolButton(this)),
    layout_(new QHBoxLayout(this)) {
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

        setStyleSheet(u"border: none;"_s);
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
    auto* const menu = new QMenu(this);

    const QAction* const renameTabAction = menu->addAction(tr("Rename Tab"));

    const QAction* const changeTabColor =
        menu->addAction(tr("Change Background Color"));

    menu->addSeparator();

    const QAction* const removeToLeftAction =
        menu->addAction(tr("Remove All Tabs To Left"));
    const QAction* const removeOtherTabs =
        menu->addAction(tr("Remove All Other Tabs"));
    const QAction* const removeToRightAction =
        menu->addAction(tr("Remove All Tabs To Right"));

    const QAction* const selectedAction =
        menu->exec(mapToGlobal(QPoint(0, this->height())));
    delete menu;

    if (selectedAction == nullptr) {
        return;
    }

    if (selectedAction == changeTabColor) {
        const QColor color = QColorDialog::getColor();
        const QString colorName = color.name();

        if (colorName == u"#ffffff"_qsv || colorName == u"#000000"_qsv) {
            setStyleSheet(QString());
        } else {
            setColor(colorName);
        }
    } else if (selectedAction == renameTabAction) {
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

auto PlaylistTab::eventFilter(QObject* const obj, QEvent* const event) -> bool {
    if (obj == label_) {
        switch (event->type()) {
            case QEvent::MouseButtonRelease: {
                const auto* const mouseEvent = as<QMouseEvent*>(event);

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
            case QEvent::MouseButtonPress: {
                auto* mouseEvent = as<QMouseEvent*>(event);
                mousePressEvent(mouseEvent);
                return true;
                break;
            }
            case QEvent::MouseMove: {
                auto* mouseEvent = as<QMouseEvent*>(event);
                mouseMoveEvent(mouseEvent);
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
    label_->setStyleSheet(u"background: transparent;"_s);
    label_->setFixedWidth(labelTextWidth());

    setFixedSize(layout_->sizeHint());
}

void PlaylistTab::mousePressEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->pos();
        dragging = false;
    }
}

void PlaylistTab::mouseMoveEvent(QMouseEvent* const event) {
    if ((event->buttons() & Qt::LeftButton) != 0 && !dragging &&
        (event->pos() - dragStartPos).manhattanLength() > START_DRAG_DISTANCE) {
        dragging = true;
        grab();
    }

    QPushButton::mouseMoveEvent(event);
}

void PlaylistTab::grab() {
    auto* const drag = new QDrag(this);
    auto* const mimeData = new QMimeData();

    mimeData->setText(objectName());
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
}

void PlaylistTab::setColor(const QString& color) {
    color_ = color;

    const auto qcolor = QColor(color);
    const u8 brightness =
        u8((0.299F * f32(qcolor.red())) + (0.587F * f32(qcolor.green())) +
           (0.114F * f32(qcolor.blue())));

    setStyleSheet(
        u"PlaylistTab { background-color: %1; }\nPlaylistTabLabel { color: %2 }\nQToolButton { color: %2; background-color: %3; }"_s
            .arg(color)
            .arg(
                (brightness > BRIGHTNESS_THRESHOLD) ? u"#000000"_s
                                                    : u"#FFFFFF"_s
            )
            .arg(qcolor.lighter(LIGHTNESS_FACTOR).name())
    );
}

void PlaylistTab::setLabelText(const QString& label) {
    label_->setText(label);
};

[[nodiscard]] auto PlaylistTab::labelText() const -> QString {
    return label_->text();
}
