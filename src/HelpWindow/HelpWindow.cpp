#include "HelpWindow.hpp"

auto HelpWindow::setupUi() -> Ui::HelpWindow* {
    auto* ui_ = new Ui::HelpWindow();
    ui_->setupUi(this);
    return ui_;
}

HelpWindow::~HelpWindow() {
    delete ui;
}