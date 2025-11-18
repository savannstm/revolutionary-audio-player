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
    ui->versionLabel->setText(u"RAP v"_s + APP_VERSION);
    ui->iconLabel->setPixmap(QPixmap(
        QApplication::applicationDirPath() + '/' +
        PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    ));
    ui->qtVersionLabel->setText(u"Qt "_s + QT_VERSION_STR);
    ui->ffmpegVersionLabel->setText(u"FFmpeg "_s + FFMPEG_VERSION);

#ifdef PROJECTM
    ui->projectMVersionLabel->setText(u"ProjectM "_s + PROJECTM_VERSION_STRING);
#endif
}

AboutWindow::~AboutWindow() {
    delete ui;
}