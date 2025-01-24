#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QAudioOutput>
#include <QDebug>
#include <QMainWindow>
#include <QMediaPlayer>

constexpr std::uint8_t DEFAULT_VOLUME = 100;
const char *SOURCE = getenv("TRACK");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      player(new QMediaPlayer(this)), audioOutput(new QAudioOutput(this)) {
    ui->setupUi(this);

    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(SOURCE));

    audioOutput->setVolume(DEFAULT_VOLUME);

    QObject::connect(player, &QMediaPlayer::mediaStatusChanged, this,
                     [&](QMediaPlayer::MediaStatus status) {
                         if (status == QMediaPlayer::LoadedMedia) {
                             qDebug("playing");
                             player->play();
                         } else {
                             qDebug("not playing");
                         }
                     });
}

MainWindow::~MainWindow() { delete ui; }
