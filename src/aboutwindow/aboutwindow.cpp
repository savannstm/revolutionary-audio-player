#include "aboutwindow.hpp"

#include "aliases.hpp"
#include "version.h"

#include <QMainWindow>

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* ui = new Ui::AboutWindow();
    ui->setupUi(this);
    return ui;
}

AboutWindow::AboutWindow(QWidget* parent) : QDialog(parent) {
    ui->versionLabel->setText(format("RAP v{}", APP_VERSION).c_str());
}

AboutWindow::~AboutWindow() {
    delete ui;
}