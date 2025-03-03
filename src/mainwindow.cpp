// local
#include "mainwindow.h"

#include "./ui_mainwindow.h"
#include "aboutwindow.h"
#include "equalizer.h"
#include "indexset.h"
#include "musicitem.h"

// std
#include <algorithm>
#include <filesystem>
#include <print>
#include <random>
#include <ranges>

// qt
#include <QAction>
#include <QApplication>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QDebug>
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMediaFormat>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTreeView>

// audio
#include <sndfile.h>

// other
#include <taglib/fileref.h>

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

using fs::path;
using std::array;
using std::string;
using std::to_string;
using std::vector;

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

enum Direction : u8 { Forward, BackwardRandom, Backward, Random };

constexpr static u8 DEFAULT_VOLUME = 100;
constexpr static array<cstr, 5> EXTENSIONS = {".mp3", ".flac", ".opus", ".aac",
                                              ".wav"};

static std::random_device rng;
static std::mt19937 gen(rng());
static IndexSet<u32> playHistory;

inline void fillTable(QStandardItemModel *treeModel,
                      const vector<path> &paths) {
    for (const auto [row, path] : views::enumerate(paths)) {
        for (u32 col = 0; col < 3; col++) {
            auto *item = new MusicItem();

#ifdef _WIN32
            const TagLib::FileRef file(path.wstring().c_str());
#else
            const TagLib::FileRef file(path.string().c_str());
#endif

            const TagLib::Tag *tag = file.tag();

            item->setEditable(false);
            item->setPath(path);

            string content;

            switch (col) {
                case 0: {
                    cstr title = tag->title().toCString(true);
                    content = title;
                    break;
                }
                case 1: {
                    cstr artist = tag->artist().toCString(true);
                    content = artist;
                    break;
                }
                case 2: {
                    item->setData(tag->track(), Qt::EditRole);
                    break;
                }
                default:
                    break;
            }

            if (col != 2) {
                item->setText(content.c_str());
            }

            treeModel->setItem(static_cast<i32>(row), static_cast<i32>(col),
                               item);
        }
    }

    treeModel->sort(2);
}

auto inline formatSecondsToMinutes(const u64 secs) -> string {
    constexpr u64 sixty = 60;
    const u64 minutes = secs / sixty;
    const u64 seconds = secs % sixty;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = to_string(minutes) + ":" + secondsPadded;
    return formatted;
}

inline void updatePosition(const u32 pos, QMediaPlayer *player,
                           QPlainTextEdit *input) {
    player->setPosition(pos);

    const string timer = formatSecondsToMinutes(pos / 1000) + "/" +
                         formatSecondsToMinutes(player->duration() / 1000);

    input->setPlainText(timer.c_str());
}

inline void jumpToTrack(QTreeView *trackTree, QStandardItemModel *treeModel,
                        QMediaPlayer *player, QPushButton *playButton,
                        Direction direction) {
    const auto idx = trackTree->currentIndex();

    u64 nextIdx;
    const u64 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == treeModel->rowCount() - 1) {
                player->stop();
                return;
            }

            nextIdx = row + 1;
            break;
        case Direction::BackwardRandom: {
            if (playHistory.empty()) {
                goto backward;
            }

            const u64 lastPlayed = playHistory.last();
            playHistory.remove(lastPlayed);
            nextIdx = lastPlayed;
            break;
        }
        case Direction::Backward:
        backward:
            if (row == 0) {
                return;
            }

            nextIdx = row - 1;
            break;
        case Direction::Random:
            playHistory.insert(row);

            const u64 totalTracks = trackTree->model()->rowCount();

            do {
                std::uniform_int_distribution<u64> dist(0, totalTracks - 1);
                nextIdx = dist(gen);
            } while (playHistory.contains(nextIdx));
            break;
    }

    const auto index = trackTree->model()->index(static_cast<i32>(nextIdx), 0);
    trackTree->setCurrentIndex(index);

    const auto data = treeModel->data(index, Qt::DisplayRole).toString();
    const auto *item =
        static_cast<MusicItem *>(treeModel->itemFromIndex(index));

    cstr path = item->getPath();
    player->setSource(QUrl::fromLocalFile(path));

    if (playButton->icon().name() == "media-playback-pause") {
        player->play();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      player(new QMediaPlayer(this)),
      audioOutput(new QAudioOutput(this)),
      tracksModel(new QStandardItemModel(this)),
      trayIcon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);

    audioOutput->setVolume(DEFAULT_VOLUME);
    player->setAudioOutput(audioOutput);

    static auto *progressSlider = ui->progressSlider;
    static auto *progressTimer = ui->progressTimer;
    static auto *playButton = ui->playButton;
    static auto *stopButton = ui->stopButton;
    static auto *backwardButton = ui->backwardButton;
    static auto *forwardButton = ui->forwardButton;
    static auto *repeatButton = ui->repeatButton;
    static auto *randomButton = ui->randomButton;

    static auto *fileMenu = ui->menuFile;
    static auto *exitAction = ui->actionExit;
    static auto *openFileAction = ui->actionOpenFile;
    static auto *openFolderAction = ui->actionOpenFolder;
    static auto *aboutAction = ui->actionAbout;

    static bool repeat = false;
    static bool random = false;

    static auto *trackTree = ui->tracksTable;
    static auto *header = trackTree->header();

    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    tracksModel->setHorizontalHeaderLabels({"Title", "Author", "No."});
    trackTree->setModel(tracksModel);

    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    trayIcon->show();

    static auto *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayIconMenu);

    static auto *settings = new QSettings("revolutionary-audio-player",
                                          "revolutionary-audio-player");

    static auto lastDir =
        settings->value("lastOpenedDir", QDir::homePath()).toString();

    static QModelIndex currentIndex;

    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    connect(trayIcon, &QSystemTrayIcon::activated, [&](const auto reason) {
        switch (reason) {
            case QSystemTrayIcon::ActivationReason::Trigger:
                if (player->isPlaying()) {
                    player->pause();
                } else {
                    player->play();
                }
                break;
            case QSystemTrayIcon::ActivationReason::DoubleClick:
                this->show();
            default:
                break;
        }
    });

    connect(repeatButton, &QPushButton::clicked, []() { repeat = !repeat; });
    connect(randomButton, &QPushButton::clicked, []() {
        random = !random;

        if (!random) {
            playHistory.clear();
        }
    });

    connect(trackTree, &QTreeView::pressed, [&](const QModelIndex &index) {
        switch (QApplication::mouseButtons()) {
            case Qt::RightButton: {
                QMenu menu(this);

                QAction *action1 = menu.addAction("Option 1");
                QAction *action2 = menu.addAction("Option 2");

                connect(action1, &QAction::triggered, this,
                        [&]() { tracksModel->removeRow(index.row()); });

                menu.exec(QCursor::pos());
                break;
            }
            case Qt::LeftButton: {
                if (index != currentIndex) {
                    const auto data =
                        tracksModel->data(index, Qt::DisplayRole).toString();
                    const auto *item = static_cast<MusicItem *>(
                        tracksModel->itemFromIndex(index));

                    if (item) {
                        cstr path = item->getPath();
                        player->setSource(QUrl::fromLocalFile(path));

                        if (playButton->icon().name() ==
                            "media-playback-pause") {
                            player->play();
                        }
                    }

                    currentIndex = index;
                }
                break;
            }
            default:
                break;
        }
    });

    connect(header, &QHeaderView::sectionClicked, [&](const i32 index) {
        if (header->sortIndicatorOrder() == Qt::SortOrder::AscendingOrder) {
            tracksModel->sort(index, Qt::SortOrder::DescendingOrder);
        } else {
            tracksModel->sort(index, Qt::SortOrder::AscendingOrder);
        }
    });

    connect(openFolderAction, &QAction::triggered, this, [&]() {
        const auto directory = QFileDialog::getExistingDirectory(
            this, "Select Directory", lastDir, QFileDialog::ShowDirsOnly);

        if (directory == QDir::rootPath() || directory == QDir::homePath() ||
            directory == nullptr) {
            return;
        }

        settings->setValue("lastOpenedDir", directory);

        const auto entries = fs::recursive_directory_iterator(
            directory.toStdString(),
            fs::directory_options::skip_permission_denied);

        auto music_entries = views::filter(entries, [](auto &entry) {
            return (entry.is_regular_file() &&
                    ranges::any_of(EXTENSIONS, [&entry](cstr ext) {
                        return entry.path().string().ends_with(ext);
                    }));
        });

        vector<path> vec_entries;

        ranges::transform(music_entries, std::back_inserter(vec_entries),
                          [](auto &entry) { return entry.path(); });

        fillTable(tracksModel, vec_entries);
    });

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

    connect(stopButton, &QPushButton::clicked, this, [&]() {
        player->stop();
        playButton->setIcon(
            QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    });

    connect(ui->forwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, player, playButton,
                    random ? Direction::Random : Direction::Forward);
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, player, playButton,
                    random ? Direction::BackwardRandom : Direction::Backward);
    });

    connect(player, &QMediaPlayer::mediaStatusChanged, this,
            [&](const QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::LoadedMedia) {
                    const i64 duration = player->duration();
                    progressSlider->setRange(0, static_cast<i32>(duration));

                    const string timer =
                        "0:00/" + formatSecondsToMinutes(duration / 1000);

                    progressTimer->setPlainText(timer.c_str());
                } else if (status == QMediaPlayer::EndOfMedia) {
                    if (repeat) {
                        player->setPosition(0);
                        player->play();
                    } else {
                        jumpToTrack(
                            trackTree, tracksModel, player, playButton,
                            random ? Direction::Random : Direction::Forward);
                    }
                }
            });

    connect(player, &QMediaPlayer::positionChanged, this, [&](const i32 pos) {
        if (!progressSlider->isSliderDown()) {
            const string timer =
                formatSecondsToMinutes(pos / 1000) + "/" +
                formatSecondsToMinutes(player->duration() / 1000);

            progressTimer->setPlainText(timer.c_str());
            progressSlider->setSliderPosition(pos);
        }
    });

    connect(progressSlider, &CustomSlider::mouseMoved, this,
            [&](const u32 pos) { updatePosition(pos, player, progressTimer); });
    connect(progressSlider, &CustomSlider::mousePressed, this,
            [&](const u32 pos) { updatePosition(pos, player, progressTimer); });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file =
            QFileDialog::getOpenFileName(this, "Select Directory", lastDir);

        if (file == nullptr) {
            return;
        }

        const auto entries = fs::recursive_directory_iterator(
            file.toStdString(), fs::directory_options::skip_permission_denied);

        vector<path> vec;
        vec.emplace_back(file.toStdString());

        fillTable(tracksModel, vec);
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        AboutWindow aboutWindow(this);
        aboutWindow.show();
    });
}

MainWindow::~MainWindow() { delete ui; }
