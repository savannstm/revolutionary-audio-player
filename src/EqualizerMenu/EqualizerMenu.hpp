#pragma once

#include "Constants.hpp"
#include "CustomInput.hpp"
#include "Settings.hpp"
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
        shared_ptr<Settings> settings,
        QWidget* parent = nullptr
    );

    ~EqualizerMenu() override { delete ui; }

    void saveSettings();
    void keyPressEvent(QKeyEvent* event) override;

    void retranslate() {
        ui->retranslateUi(this);

        if (eqSettings.enabled) {
            toggleButton->setText(tr("Enabled"));
        }

        const span<const f32> frequencies = span(
            getFrequenciesForBands(eqSettings.bandCount),
            eqSettings.bandCount
        );

        for (const auto& [container, frequency] : views::zip(
                 views::take(sliders, eqSettings.bandCount),
                 views::take(frequencies, eqSettings.bandCount)
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

   signals:
    void gainChanged(u8 band);

   private:
    inline void updateGain(
        const QString& string,
        QSlider* slider,
        CustomInput* input,
        u8 band
    );

    inline void onDbInputRejected(u8 band);
    inline void onDbInputEditingFinished(u8 band);
    inline void onDbInputUnfocused(u8 band);

    inline void onSliderValueChanged(i8 gain, u8 band);

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

    inline auto createSliderElement(u8 band, QWidget* parent)
        -> SliderContainer;

    Ui::EqualizerMenu* ui = setupUi();

    u8 previousPresetIndex = 0;

    shared_ptr<Settings> settings;
    EqualizerSettings& eqSettings;

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
