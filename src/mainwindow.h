#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QAudioOutput>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QTextEdit>
#include <fstream>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class AudioCache {
   private:
    std::string cachePath;

   public:
    AudioCache(std::string cacheFilePath = "audio_cache.txt")
        : cachePath(std::move(cacheFilePath)) {}

    void savePaths(const std::vector<std::string>& paths) {
        std::ofstream cacheFile(cachePath);

        if (!cacheFile) {
            std::cerr << "Failed to open cache file for writing" << '\n';
            return;
        }

        for (const auto& path : paths) {
            cacheFile << path << '\n';
        }

        cacheFile.close();
    }

    auto loadPaths() -> std::vector<std::string> {
        std::vector<std::string> paths;
        std::ifstream cacheFile(cachePath);

        if (!cacheFile) {
            return paths;
        }

        std::string line;
        while (std::getline(cacheFile, line)) {
            if (!line.empty()) {
                paths.push_back(line);
            }
        }
        cacheFile.close();

        return paths;
    }

    void clearCache() {
        std::ofstream cacheFile(cachePath, std::ofstream::trunc);
        cacheFile.close();
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   private:
    Ui::MainWindow* ui;
    QMediaPlayer* player;
    QAudioOutput* audioOutput;
    QSlider* slider;
    QTextEdit* input;
    QPushButton* playButton;
    QPushButton* stopButton;
    QPushButton* nextButton;
    QPushButton* previousButton;
    QAction* exitAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QStandardItemModel* tracksModel;
    AudioCache cache;
};

#endif  // MAINWINDOW_H
