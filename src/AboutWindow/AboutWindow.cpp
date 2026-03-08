#include "AboutWindow.hpp"

#include "Constants.hpp"
#include "ui_AboutWindow.h"
#include "version.h"

#include <libavutil/ffversion.h>

#ifdef PROJECTM
#include <projectM-4/version.h>
#endif

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* const ui_ = new Ui::AboutWindow();
    ui_->setupUi(this);
    return ui_;
}

AboutWindow::AboutWindow(QWidget* const parent) :
    QDialog(parent),
    ui(setupUi()) {
    ui->versionLabel->setText(u"RAP v"_s + APP_VERSION);
    ui->iconLabel->setPixmap(QPixmap(u":/logo.png"_s));
    ui->qtVersionLabel->setText(u"Qt "_s + qVersion());
    ui->ffmpegVersionLabel->setText(u"FFmpeg "_s + FFMPEG_VERSION);

#ifdef PROJECTM
    ui->projectMVersionLabel->setText(u"ProjectM "_s + PROJECTM_VERSION_STRING);
#endif
}

AboutWindow::~AboutWindow() {
    delete ui;
}

void AboutWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QDialog::changeEvent(event);
}
