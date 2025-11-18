#pragma once

#include "Constants.hpp"
#include "Enums.hpp"
#include "Settings.hpp"

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#endif

// TODO: Random preset each new track

class VisualizerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit VisualizerDialog(
        shared_ptr<Settings> settings,
        QWidget* parent = nullptr
    );
    ~VisualizerDialog() override;

    void setChannels(AudioChannels channels);
    void addSamples(const f32* buffer);
    void clear();

   signals:
    void ready();

   private:
    auto initSharedMemory() -> bool;

    void loadPreset();
    void applySettings();
    void openPreset(const QString& path);

    shared_ptr<Settings> settings;

    QSpinBox* fpsSpin;
    QSpinBox* meshWidthSpin;
    QSpinBox* meshHeightSpin;

    QPushButton* presetButton;
    QPushButton* applyButton;

    QProcess* process = nullptr;
    VisualizerSharedData* sharedData = nullptr;

#ifdef Q_OS_WINDOWS
    HANDLE hMapFile = nullptr;
#else
    int shmFd = -1;
#endif
};
