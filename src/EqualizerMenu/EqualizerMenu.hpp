#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FWD.hpp"

#include <QDialog>

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
        const shared_ptr<Settings>& settings_,
        QWidget* parent = nullptr
    );

    ~EqualizerMenu() override;

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

    inline void importPreset();
    inline void exportPreset();

    inline void savePreset(
        const QString& presetName,
        optional<Bands> bandCount = nullopt,
        optional<array<i8, THIRTY_BANDS>> bands = nullopt
    );

    inline auto setupUi() -> Ui::EqualizerMenu*;

    array<SliderContainer, THIRTY_BANDS> sliders;

    Ui::EqualizerMenu* const ui;

    EqualizerSettings& settings;

    QVBoxLayout* const layout;

    QWidget* const firstSliderRow;
    QHBoxLayout* const firstSliderLayout;

    QWidget* const secondSliderRow;
    QHBoxLayout* const secondSliderLayout;

    QLabel* const bandSelectLabel;
    QComboBox* const bandSelect;

    QLabel* const presetSelectLabel;
    QComboBox* const presetSelect;

    QPushButton* const resetGainsButton;
    QPushButton* const newPresetButton;
    QPushButton* const deletePresetButton;

    QPushButton* const importPresetButton;
    QPushButton* const exportPresetButton;

    QPushButton* const toggleButton;

    u8 previousPresetIndex = 0;
};
