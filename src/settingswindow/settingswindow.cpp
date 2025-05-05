#include "settingswindow.hpp"

#include <QStyleFactory>
#include <utility>

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
}

SettingsWindow::SettingsWindow(shared_ptr<Settings> settings, QWidget* parent) :
    QDialog(parent),
    settings(std::move(settings)) {
    for (const QString& style : QStyleFactory::keys()) {
        styleSelect->addItem(style);
    }

    styleSelect->setCurrentText(
        APPLICATION_STYLES[settings->flags.applicationStyle].toString()
    );
    dragDropSelect->setCurrentIndex(settings->flags.dragNDropMode);
    playlistNamingSelect->setCurrentIndex(settings->flags.playlistNaming);

    connect(
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [&](const QString& itemText) {
        QApplication::setStyle(itemText);

        for (const auto [idx, style] : views::enumerate(APPLICATION_STYLES)) {
            if (itemText == style) {
                settings->flags.applicationStyle = as<Style>(idx);
                break;
            }
        }
    }
    );

    connect(
        dragDropSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) {
        settings->flags.dragNDropMode = as<DragDropMode>(index);
    }
    );

    connect(
        playlistNamingSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) {
        settings->flags.playlistNaming = as<PlaylistNaming>(index);
    }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}