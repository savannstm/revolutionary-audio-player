#pragma once

#include "FWD.hpp"
#include "Visualizer.hpp"

#include <QDialog>

class VisualizerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit VisualizerDialog(
        const shared_ptr<Settings>& settings_,
        QWidget* parent = nullptr
    );
    ~VisualizerDialog() override;

    void setChannels(u8 channels);
    void addSamples(const f32* samples);

   signals:
    void ready();

   private:
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

    Visualizer* visualizer;

    u8 channels;
};
