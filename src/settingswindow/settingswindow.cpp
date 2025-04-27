#include "settingswindow.hpp"

#include "aliases.hpp"

#include <QStyleFactory>

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
}

SettingsWindow::SettingsWindow(QWidget* parent) : QDialog(parent) {
    for (const QString& style : QStyleFactory::keys()) {
        styleSelect->addItem(style);
    }

    connect(
        styleSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const i32 index) {
        QApplication::setStyle(styleSelect->itemText(index));
    }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}