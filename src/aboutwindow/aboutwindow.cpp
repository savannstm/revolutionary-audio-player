#include "aboutwindow.hpp"

#include "aliases.hpp"
#include "version.h"

#include <QMainWindow>

auto AboutWindow::setupUi() -> bool {
    ui->setupUi(this);
    return true;
}

AboutWindow::AboutWindow(QWidget* parent) : QDialog(parent) {
    ui->versionLabel->setText(format("RAP v{}", APP_VERSION).c_str());
}

AboutWindow::~AboutWindow() {
    delete ui;
}