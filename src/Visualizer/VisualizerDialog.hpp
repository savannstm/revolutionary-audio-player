#pragma once

#include "Constants.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QDialog>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#endif

class VisualizerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit VisualizerDialog(
        const shared_ptr<Settings>& settings_,
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

    VisualizerSettings& settings;

    QCheckBox* useRandomPresetsCheckbox;
    QLineEdit* randomPresetDirInput;

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
    i32 shmFd = -1;
#endif
};
