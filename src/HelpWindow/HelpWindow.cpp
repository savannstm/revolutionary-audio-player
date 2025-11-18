#include "HelpWindow.hpp"

auto HelpWindow::setupUi() -> Ui::HelpWindow* {
    auto* const ui_ = new Ui::HelpWindow();
    ui_->setupUi(this);
    return ui_;
}

HelpWindow::~HelpWindow() {
    delete ui;
}