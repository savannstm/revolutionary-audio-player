#include "helpwindow.hpp"

auto HelpWindow::setupUi() -> Ui::HelpWindow* {
    auto* ui = new Ui::HelpWindow();
    ui->setupUi(this);
    return ui;
}

HelpWindow::HelpWindow(QWidget* parent) : QDialog(parent) {}

HelpWindow::~HelpWindow() {
    delete ui;
}