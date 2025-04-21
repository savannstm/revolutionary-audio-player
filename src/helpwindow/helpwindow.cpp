#include "helpwindow.hpp"

auto HelpWindow::setupUi() -> bool {
    ui->setupUi(this);
    return true;
}

HelpWindow::HelpWindow(QWidget* parent) : QDialog(parent) {}

HelpWindow::~HelpWindow() {
    delete ui;
}