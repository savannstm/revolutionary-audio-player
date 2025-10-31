#include "AboutWindow.hpp"

#include "version.h"

#include <libavutil/ffversion.h>

#ifdef PROJECTM
#include <projectM-4/version.h>
#endif

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* ui_ = new Ui::AboutWindow();
    ui_->setupUi(this);
    return ui_;
}

AboutWindow::AboutWindow(MainWindow* parent) : QDialog(parent) {
    connect(parent, &MainWindow::retranslated, this, [&] -> void {
        ui->retranslateUi(this);
    });

    ui->versionLabel->setText(u"RAP v%1"_s.arg(APP_VERSION));
    ui->iconLabel->setPixmap(
        QPixmap(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );
    ui->qtVersionLabel->setText(u"Qt %1"_s.arg(QT_VERSION_STR));
    ui->ffmpegVersionLabel->setText(u"FFmpeg %1"_s.arg(FFMPEG_VERSION));

#ifdef PROJECTM
    ui->projectMVersionLabel->setText(
        u"ProjectM %1"_s.arg(PROJECTM_VERSION_STRING)
    );
#endif
}

AboutWindow::~AboutWindow() {
    delete ui;
}