#include "aboutwindow.hpp"

#include "version.h"

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* ui_ = new Ui::AboutWindow();
    ui_->setupUi(this);
    return ui_;
}

AboutWindow::AboutWindow(MainWindow* parent) : QDialog(parent) {
    connect(parent, &MainWindow::retranslated, this, [&] {
        ui->retranslateUi(this);
    });

    ui->versionLabel->setText(u"RAP v%1"_s.arg(APP_VERSION));
    ui->iconLabel->setPixmap(
        QPixmap(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );
}

AboutWindow::~AboutWindow() {
    delete ui;
}