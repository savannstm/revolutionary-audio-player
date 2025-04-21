#include "settingswindow.hpp"

auto SettingsWindow::setupUi() -> bool {
    ui->setupUi(this);
    return true;
}

SettingsWindow::SettingsWindow(QWidget* parent) : QDialog(parent) {}

SettingsWindow::~SettingsWindow() {
    delete ui;
}