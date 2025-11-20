#pragma once

#include "CustomInput.hpp"
#include "Settings.hpp"
#include "ui_EqualizerMenu.h"

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

// TODO: Import/export presets

class EqualizerMenu : public QDialog {
    Q_OBJECT

   public:
    explicit EqualizerMenu(
        const shared_ptr<Settings>& settings_,
        QWidget* parent = nullptr
    );

    ~EqualizerMenu() override { delete ui; }

    void saveSettings();
    void keyPressEvent(QKeyEvent* event) override;
    void retranslate();
    void loadSettings();

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

    inline void changeBands(const QString& count);
    inline void buildBands();

    inline auto createSliderElement(u8 band, QWidget* parent)
        -> SliderContainer;

    auto setupUi() -> Ui::EqualizerMenu* {
        auto* const ui_ = new Ui::EqualizerMenu();
        ui_->setupUi(this);
        return ui_;
    }

    array<SliderContainer, THIRTY_BANDS> sliders;

    Ui::EqualizerMenu* const ui = setupUi();

    EqualizerSettings& settings;

    QVBoxLayout* const layout = ui->verticalLayout;

    QWidget* const firstSliderRow = ui->firstSliderRow;
    QHBoxLayout* const firstSliderLayout = ui->firstSliderLayout;

    QWidget* const secondSliderRow = ui->secondSliderRow;
    QHBoxLayout* const secondSliderLayout = ui->secondSliderLayout;

    QLabel* const bandSelectLabel = ui->bandSelectLabel;
    QComboBox* const bandSelect = ui->bandSelect;

    QLabel* const presetSelectLabel = ui->presetSelectLabel;
    QComboBox* const presetSelect = ui->presetSelect;

    QPushButton* const resetGainsButton = ui->resetGainsButton;
    QPushButton* const newPresetButton = ui->newPresetButton;
    QPushButton* const deletePresetButton = ui->deletePresetButton;

    QPushButton* const toggleButton = ui->toggleButton;

    u8 previousPresetIndex = 0;
};
