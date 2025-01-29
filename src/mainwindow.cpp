// local
#include "mainwindow.h"

#include "musicitem.h"
#include "ui_mainwindow.h"

// std
#include <filesystem>
#include <print>
#include <ranges>

// qt
#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QAudioOutput>
#include <QDebug>
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>

using std::array;
using std::string;
using std::to_string;
using std::vector;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using str = char *;
using cstr = const char *;
using f32 = float;
using f64 = double;
using path = fs::path;

constexpr u8 DEFAULT_VOLUME = 100;
constexpr array<cstr, 5> EXTENSIONS = {".mp3", ".flac", ".opus", ".aac",
                                       ".wav"};

enum Direction : u8 { Forward, Backward };

auto restorePaths(AudioCache *cache, QStandardItemModel *tracksModel) -> void {
    vector<path> restored = cache->loadPaths();

    for (const auto [i, path] : views::enumerate(restored)) {
        auto *item = new MusicItem(path.filename().string().c_str());

        item->setEditable(false);
        item->setPath(path.string().c_str());

        tracksModel->setItem(static_cast<i32>(i), 0, item);
    }
}

auto formatSecondsToMinutes(u64 secs) -> string {
    constexpr u64 sixty = 60;
    const u64 minutes = secs / sixty;
    const u64 seconds = secs % sixty;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = to_string(minutes) + ":" + secondsPadded;
    return formatted;
}

void jumpToTrack(QTableView *tracksTable, QStandardItemModel *tracksModel,
                 QMediaPlayer *player, QPushButton *playButton,
                 Direction direction) {
    const auto idx = tracksTable->currentIndex();
    const u16 nextIdx =
        direction == Direction::Forward ? idx.row() + 1 : idx.row() - 1;
    tracksTable->selectRow(nextIdx);

    const QString data =
        tracksModel->data(tracksTable->currentIndex(), Qt::DisplayRole)
            .toString();
    const auto *item = static_cast<MusicItem *>(
        tracksModel->itemFromIndex(tracksTable->currentIndex()));

    if (item != nullptr) {
        cstr path = item->getPath();
        player->setSource(QUrl::fromLocalFile(path));

        if (playButton->icon().name() == "media-playback-pause") {
            player->play();
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      player(new QMediaPlayer(this)),
      audioOutput(new QAudioOutput(this)),
      exitAction(new QAction("Exit")),
      openFileAction(new QAction("Open File")),
      openFolderAction(new QAction("Open Folder")),
      tracksModel(new QStandardItemModel(this)),
      trayIcon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);

    audioOutput->setVolume(DEFAULT_VOLUME);
    player->setAudioOutput(audioOutput);

    static auto *slider = ui->horizontalSlider;
    static auto *input = ui->textEdit;
    static auto *playButton = ui->playButton;
    static auto *stopButton = ui->stopButton;
    static auto *backwardButton = ui->backwardButton;
    static auto *forwardButton = ui->forwardButton;
    static auto *tracksTable = ui->tracksTable;

    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::Battery));
    trayIcon->setVisible(true);

    connect(trayIcon, &QSystemTrayIcon::messageClicked, []() {});

    tracksTable->setModel(tracksModel);
    tracksTable->verticalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    tracksTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    restorePaths(&cache, tracksModel);

    connect(tracksTable, &QTableView::clicked, this,
            [&](const QModelIndex &index) {
                const QString data =
                    tracksModel->data(index, Qt::DisplayRole).toString();
                const auto *item =
                    static_cast<MusicItem *>(tracksModel->itemFromIndex(index));

                if (item) {
                    cstr path = item->getPath();
                    player->setSource(QUrl::fromLocalFile(path));
                }
            });

    connect(openFolderAction, &QAction::triggered, this, [&]() {
        const auto directory = QFileDialog::getExistingDirectory(
            this, "Select Directory", QDir::homePath(),
            QFileDialog::ShowDirsOnly);

        const auto entries = fs::recursive_directory_iterator(
            directory.toStdString(),
            fs::directory_options::skip_permission_denied);

        auto music_entries = views::filter(entries, [](auto &entry) {
            return (entry.is_regular_file() &&
                    ranges::any_of(EXTENSIONS, [&entry](cstr ext) {
                        return entry.path().filename().string().ends_with(ext);
                    }));
        });

        vector<string> vec = {};

        for (const auto [i, entry] : views::enumerate(music_entries)) {
            const path path = entry.path();
            auto *item = new MusicItem(path.filename().string().c_str());

            item->setEditable(false);
            item->setPath(path.string().c_str());
            vec.push_back(path.string());

            tracksModel->setItem(static_cast<i32>(i), 0, item);
        }

        cache.savePaths(vec);
    });
    ui->menuFile->addAction(openFolderAction);

    connect(playButton, &QPushButton::clicked, this, [&]() {
        if (!player->source().isEmpty()) {
            const auto currentIcon = playButton->icon();

            if (currentIcon.name() == "media-playback-start") {
                player->play();
                playButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
            } else {
                player->pause();
                playButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
            }
        }
    });

    connect(exitAction, &QAction::triggered, this, []() { std::exit(0); });
    ui->menuFile->addAction(exitAction);

    connect(stopButton, &QPushButton::clicked, this, [&]() {
        player->stop();
        playButton->setIcon(
            QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    });

    connect(ui->forwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(tracksTable, tracksModel, player, playButton,
                    Direction::Forward);
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(tracksTable, tracksModel, player, playButton,
                    Direction::Backward);
    });

    connect(player, &QMediaPlayer::metaDataChanged, this, [&]() {
        const auto metadata = player->metaData();
        const auto metadataKeys = metadata.keys();
    });

    connect(player, &QMediaPlayer::mediaStatusChanged, this,
            [&](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::LoadedMedia) {
                    const u64 duration = static_cast<u64>(player->duration());
                    slider->setRange(0, static_cast<i32>(duration));

                    const string timer =
                        "0:00/" + formatSecondsToMinutes(duration / 1000);

                    input->setText(timer.c_str());
                } else if (status == QMediaPlayer::EndOfMedia) {
                    jumpToTrack(tracksTable, tracksModel, player, playButton,
                                Direction::Forward);
                }
            });

    connect(player, &QMediaPlayer::positionChanged, this, [&](i64 pos) {
        const string timer = formatSecondsToMinutes(pos / 1000) + "/" +
                             formatSecondsToMinutes(player->duration() / 1000);

        input->setText(timer.c_str());
        slider->setSliderPosition(static_cast<i32>(pos));
    });

    connect(slider, &QSlider::sliderMoved, this,
            [&](u32 pos) { player->setPosition(pos); });
}

MainWindow::~MainWindow() { delete ui; }
