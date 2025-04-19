#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audiostreamer.hpp"
#include "constants.hpp"
#include "customslider.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"
#include "searchinput.hpp"
#include "tracktree.hpp"

#include <QAction>
#include <QAudioSink>
#include <QCloseEvent>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTreeView>
#include <random>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Add equalizer menu
// TODO: Add tracks caching

constexpr u16 TRACK_PROPERTIES_ARRAY_SIZE = UINT8_MAX + 1;

constexpr auto
propertyHash(const u8 charA, const u8 charB, const u8 charC, const u8 charD)
    -> u8 {
    return static_cast<u8>(charA + charB + charC + charD);
}

constexpr auto initTrackProperties()
    -> array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> {
    array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> arr{};

    arr[propertyHash('T', 'i', 'l', 'e')] = Title;
    arr[propertyHash('A', 'r', 's', 't')] = Artist;
    arr[propertyHash('A', 'l', 'u', 'm')] = Album;
    arr[propertyHash('T', 'r', 'e', 'r')] = TrackNumber;
    arr[propertyHash('A', 'l', 's', 't')] = AlbumArtist;
    arr[propertyHash('G', 'e', 'r', 'e')] = Genre;
    arr[propertyHash('Y', 'e', 'a', 'r')] = Year;
    arr[propertyHash('D', 'u', 'o', 'n')] = Duration;
    arr[propertyHash('C', 'o', 'e', 'r')] = Composer;
    arr[propertyHash('B', 'P', 'P', 'M')] = BPM;
    arr[propertyHash('L', 'a', 'g', 'e')] = Language;
    arr[propertyHash('D', 'i', 'e', 'r')] = DiscNumber;
    arr[propertyHash('C', 'o', 'n', 't')] = Comment;
    arr[propertyHash('P', 'u', 'e', 'r')] = Publisher;
    arr[propertyHash('B', 'i', 't', 'e')] = Bitrate;
    arr[propertyHash('S', 'a', 't', 'e')] = SampleRate;
    arr[propertyHash('C', 'h', 'l', 's')] = Channels;
    arr[propertyHash('F', 'o', 'a', 't')] = Format;

    return arr;
}

constexpr array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> TRACK_PROPERTIES =
    initTrackProperties();

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void filesDropped(const vector<string>& filePaths);
    void trackDataReady(const metadata_array& metadata, const string& path);
    void trackDataProcessed();

   private:
    // UI - Widgets
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    QMenu* trayIconMenu = new QMenu(this);
    TrackTree* trackTree;
    MusicHeader* trackTreeHeader;
    MusicModel* trackTreeModel = new MusicModel(this);
    QLabel* progressLabel;
    QLabel* volumeLabel;
    SearchInput* searchTrackInput = new SearchInput(this);

    // UI - Buttons
    QPushButton* playButton;
    QPushButton* stopButton;
    QPushButton* backwardButton;
    QPushButton* forwardButton;
    ActionButton* repeatButton;
    ActionButton* randomButton;

    // UI - Sliders
    CustomSlider* volumeSlider;
    CustomSlider* progressSlider;

    // UI - Icons
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");

    // Actions & Menus
    QMenu* fileMenu;
    QAction* exitAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QAction* aboutAction;
    QAction* forwardAction;
    QAction* backwardAction;
    QAction* repeatAction;
    QAction* pauseAction;
    QAction* resumeAction;
    QAction* stopAction;
    QAction* randomAction;

    // Settings
    QSettings* settings =
        new QSettings("com.savannstm.rap", "com.savannstm.rap");

    // UI State
    QStringList headerLabels = { "Title", "Artist", "Track Number" };
    QString lastDir =
        settings->value("lastOpenedDir", QDir::homePath()).toString();

    // Playback / Audio
    AudioStreamer* audioStreamer = new AudioStreamer();
    QAudioSink* audioSink;
    f32 volumeGain = 1.0;
    string audioDuration = "0:00";

    // Control Logic
    RepeatMode repeat = RepeatMode::Off;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;
    IndexSet playHistory = IndexSet();

    // Threads
    QThreadPool* threadPool = new QThreadPool(this);

    // Equalizer
    bool eqEnabled = false;
    array<u8, EQ_BANDS_N> eqGains = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    // Search
    vector<QModelIndex> searchMatches;
    usize searchMatchesPosition = 0;
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    string previousSearchPrompt;

    // Miscellaneous
    std::random_device randdevice;

    // Inline utility functions
    inline void fillTable(const string& filePath);
    inline void fillTable(const vector<string>& paths);
    inline void fillTable(const walk_dir& read_dir);
    inline void jumpToTrack(Direction direction, bool clicked);
    inline void updatePlaybackPosition();
    inline void playTrack(const QModelIndex& index);
    inline void stopPlayback();
    static inline auto getTrackProperty(cstr str, usize len) -> TrackProperty;
};
