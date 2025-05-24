#pragma once

#include "audioworker.hpp"
#include "constants.hpp"
#include "custominput.hpp"
#include "settings.hpp"
#include "ui_equalizermenu.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

QT_BEGIN_NAMESPACE

namespace Ui {
    class EqualizerMenu;
}  // namespace Ui

QT_END_NAMESPACE

struct SliderContainer {
    QWidget* container;
    QSlider* slider;
    QLabel* dbLabel;
    CustomInput* dbInput;
    QLabel* hzLabel;
};

class EqualizerMenu : public QDialog {
    Q_OBJECT

   public:
    explicit EqualizerMenu(
        QWidget* parent,
        AudioWorker* audioWorker_,
        shared_ptr<Settings> settings_
    );

    ~EqualizerMenu() override { delete ui; }

    void saveSettings();
    void keyPressEvent(QKeyEvent* event) override;

    void retranslate() {
        ui->retranslateUi(this);

        if (audioWorker->equalizerEnabled()) {
            toggleButton->setText(tr("Enabled"));
        }

        const FrequencyArray frequencies = audioWorker->frequencies();

        for (const auto& [container, frequency] : views::zip(
                 views::take(sliders, bandCount),
                 views::take(frequencies, bandCount)
             )) {
            container.dbLabel->setText(tr("dB"));
            container.hzLabel->setText(tr("%1 Hz").arg(frequency));
        }
    }

    void loadSettings() {
        toggleButton->setChecked(settings->equalizerSettings.enabled);
        bandSelect->setCurrentIndex(settings->equalizerSettings.bandIndex);

        changeBands(bandSelect->currentText());

        presetSelect->setCurrentIndex(settings->equalizerSettings.presetIndex);
    }

   private:
    inline void updateGain(
        const QString& string,
        QSlider* slider,
        CustomInput* input,
        u8 band
    ) const;

    inline void onDbInputRejected(u8 band) const;
    inline void onDbInputEditingFinished(u8 band) const;
    inline void onDbInputUnfocused(u8 band) const;

    inline void onSliderValueChanged(i8 value, u8 band) const;

    inline void selectPreset(i32 index);
    inline void toggleEqualizer(bool checked);
    inline void resetGains();
    inline void resetFrequencies();
    inline void resetAll();
    inline void createNewPreset();

    void changeBands(const QString& count);
    void buildBands();

    auto setupUi() -> Ui::EqualizerMenu* {
        auto* ui_ = new Ui::EqualizerMenu();
        ui_->setupUi(this);
        return ui_;
    }

    inline auto createSliderElement(
        u8 band,
        const FrequencyArray& frequencies,
        QWidget* parent
    ) const -> SliderContainer;

    AudioWorker* audioWorker;
    Ui::EqualizerMenu* ui = setupUi();

    u8 bandCount = TEN_BANDS;
    u8 previousPresetIndex = 0;

    shared_ptr<Settings> settings;
    EqualizerSettings& eqSettings = settings->equalizerSettings;

    QVBoxLayout* layout = ui->verticalLayout;

    QWidget* firstSliderRow = ui->firstSliderRow;
    QHBoxLayout* firstSliderLayout = ui->firstSliderLayout;

    QWidget* secondSliderRow = ui->secondSliderRow;
    QHBoxLayout* secondSliderLayout = ui->secondSliderLayout;

    QLabel* bandSelectLabel = ui->bandSelectLabel;
    QComboBox* bandSelect = ui->bandSelect;

    QLabel* presetSelectLabel = ui->presetSelectLabel;
    QComboBox* presetSelect = ui->presetSelect;

    QPushButton* resetGainsButton = ui->resetGainsButton;
    QPushButton* newPresetButton = ui->newPresetButton;
    QPushButton* deletePresetButton = ui->deletePresetButton;

    QPushButton* toggleButton = ui->toggleButton;
    array<SliderContainer, MAX_BANDS_COUNT> sliders;
};
