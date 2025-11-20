#include "EqualizerMenu.hpp"

#include "Constants.hpp"
#include "CustomInput.hpp"
#include "DoubleValidator.hpp"
#include "EqualizerPresets.hpp"
#include "IntValidator.hpp"
#include "Invalidator.hpp"
#include "Settings.hpp"

#include <QKeyEvent>
#include <QSlider>

EqualizerMenu::EqualizerMenu(
    const shared_ptr<Settings>& settings_,
    QWidget* const parent
) :
    QDialog(parent, Qt::FramelessWindowHint | Qt::Popup),
    settings(settings_->equalizer) {
    connect(
        bandSelect,
        &QComboBox::currentTextChanged,
        this,
        &EqualizerMenu::changeBands
    );

    connect(
        presetSelect,
        &QComboBox::currentIndexChanged,
        this,
        &EqualizerMenu::selectPreset
    );

    connect(
        toggleButton,
        &QPushButton::toggled,
        this,
        &EqualizerMenu::toggleEqualizer
    );

    connect(
        resetGainsButton,
        &QPushButton::pressed,
        this,
        &EqualizerMenu::resetGains
    );

    connect(
        newPresetButton,
        &QPushButton::pressed,
        this,
        &EqualizerMenu::createNewPreset
    );

    connect(deletePresetButton, &QPushButton::pressed, this, [this] -> void {
        const u16 index = presetSelect->currentIndex();
        presetSelect->setCurrentIndex(0);
        presetSelect->removeItem(index);
    });

    // Prevent inserting new entry on editingFinished
    presetSelect->setValidator(new Invalidator(presetSelect));

    // TODO: Inefficient, connect to line edit directly.
    // Although who cares
    connect(
        presetSelect,
        &QComboBox::editTextChanged,
        this,
        [this](const QString& text) -> void {
        const i32 currentIndex = presetSelect->currentIndex();
        const QString previousText = presetSelect->itemText(currentIndex);

        if (text == previousText) {
            return;
        }

        const QVector<i8> previousPreset =
            settings.presets[settings.bandCount][previousText];

        if (!settings.presets[settings.bandCount].contains(text)) {
            settings.presets[settings.bandCount].insert(text, QVector<i8>());
        }

        std::swap(
            settings.presets[settings.bandCount][previousText],
            settings.presets[settings.bandCount][text]
        );

        presetSelect->setItemText(currentIndex, text);
        settings.presets[settings.bandCount].remove(previousText);
    }
    );
}

auto EqualizerMenu::createSliderElement(const u8 band, QWidget* const parent)
    -> SliderContainer {
    // Slider container
    auto* const sliderContainer = new QWidget(parent);
    auto* const sliderLayout = new QVBoxLayout(sliderContainer);
    sliderLayout->setAlignment(Qt::AlignCenter);

    auto* const slider = new QSlider(Qt::Vertical, sliderContainer);
    slider->setRange(MIN_GAIN, MAX_GAIN);

    // dB Container
    auto* const dbContainer = new QWidget(sliderContainer);
    auto* const dbLayout = new QHBoxLayout(dbContainer);
    dbLayout->setAlignment(Qt::AlignCenter);

    auto* const dbInput =
        new CustomInput(QString::number(settings.gains[band]), dbContainer);
    auto* const intValidator = new IntValidator(MIN_GAIN, MAX_GAIN, dbInput);
    dbInput->setValidator(intValidator);
    dbInput->setFixedWidth(GAIN_INPUT_FIXED_WIDTH);

    connect(dbInput, &CustomInput::inputRejected, this, [this, band] -> void {
        onDbInputRejected(band);
    });

    connect(dbInput, &CustomInput::editingFinished, this, [this, band] -> void {
        onDbInputEditingFinished(band);
    });

    connect(dbInput, &CustomInput::unfocused, this, [this, band] -> void {
        onDbInputUnfocused(band);
    });

    auto* const dbLabel = new QLabel(tr("dB"), dbContainer);

    dbLayout->addWidget(dbInput);
    dbLayout->addWidget(dbLabel);
    sliderLayout->addWidget(dbContainer, 0, Qt::AlignCenter);

    // Adding slider to the center
    sliderLayout->addWidget(slider, 0, Qt::AlignCenter);

    // Hz container
    auto* const hzContainer = new QWidget(sliderContainer);
    auto* const hzLayout = new QHBoxLayout(hzContainer);
    hzLayout->setAlignment(Qt::AlignCenter);

    auto* const hzLabel = new QLabel(
        QString::number(settings.frequencies[band]) + tr(" Hz"),
        sliderContainer
    );

    hzLayout->addWidget(hzLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(hzContainer, 0, Qt::AlignCenter);

    connect(
        slider,
        &QSlider::valueChanged,
        this,
        [this, band](const i8 gain) -> void {
        onSliderValueChanged(gain, band);
    }
    );

    return { .container = sliderContainer,
             .slider = slider,
             .dbLabel = dbLabel,
             .dbInput = dbInput,
             .hzLabel = hzLabel };
}

void EqualizerMenu::buildBands() {
    const QObjectList firstSliderRowChildren = firstSliderRow->children();
    for (QObject* const child : firstSliderRowChildren) {
        const auto* childWidget = qobject_cast<QWidget*>(child);
        delete childWidget;
    }

    const QObjectList secondSliderRowChildren = secondSliderRow->children();
    for (QObject* const child : secondSliderRowChildren) {
        const auto* childWidget = qobject_cast<QWidget*>(child);
        delete childWidget;
    }

    settings.frequencies = getFrequenciesForBands(settings.bandCount);
    settings.gains.fill(0);

    for (const u8 band : range(0, u8(settings.bandCount))) {
        QWidget* const parent =
            band < THIRTY_BANDS / 2 ? firstSliderRow : secondSliderRow;
        QHBoxLayout* const parentLayout =
            band < THIRTY_BANDS / 2 ? firstSliderLayout : secondSliderLayout;

        const SliderContainer elements = createSliderElement(band, parent);

        parentLayout->addWidget(elements.container);
        sliders[band] = elements;
    }

    firstSliderRow->resize(firstSliderLayout->sizeHint());
    secondSliderRow->resize(secondSliderLayout->sizeHint());

    if (u8(settings.bandCount) < (THIRTY_BANDS / 2)) {
        secondSliderRow->hide();
    } else {
        secondSliderRow->show();
    }

    layout->invalidate();
    layout->activate();
    resize(layout->sizeHint());
}

void EqualizerMenu::saveSettings() {
    settings.bandIndex = u8(bandSelect->currentIndex());

    const u16 presetIndex = u16(presetSelect->currentIndex());
    settings.presetIndex = presetIndex;

    if (presetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(presetIndex);

        settings.presets[settings.bandCount][presetName] =
            QVector<i8>(isize(settings.bandCount));

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, u8(settings.bandCount)))) {
            settings.presets[settings.bandCount][presetName][idx] =
                i8(container.dbInput->text().toInt());
        }
    }
}

void EqualizerMenu::keyPressEvent(QKeyEvent* const event) {
    event->ignore();
}

void EqualizerMenu::updateGain(
    const QString& string,
    QSlider* const slider,
    CustomInput* const input,
    const u8 band
) {
    const i8 gain = i8(string.toInt());
    slider->setValue(gain);
    input->clearFocus();

    settings.gains[band] = gain;
    emit gainChanged(band);
}

void EqualizerMenu::onDbInputRejected(const u8 band) {
    QSlider* const slider = sliders[band].slider;
    CustomInput* const input = sliders[band].dbInput;
    const auto* const validator = as<const IntValidator*>(input->validator());

    QString string = input->text();
    i32 pos = 0;

    if (validator->validate(string, pos) == QValidator::Invalid) {
        return;
    }

    validator->fixup(string);
    input->setText(string);

    updateGain(string, slider, input, band);
}

void EqualizerMenu::onDbInputEditingFinished(const u8 band) {
    QSlider* const slider = sliders[band].slider;
    CustomInput* const input = sliders[band].dbInput;

    updateGain(input->text(), slider, input, band);
}

void EqualizerMenu::onDbInputUnfocused(const u8 band) {
    QSlider* const slider = sliders[band].slider;
    CustomInput* const input = sliders[band].dbInput;
    const auto* const validator = as<const IntValidator*>(input->validator());

    QString string = input->text();
    validator->fixup(string);
    input->setText(string);

    updateGain(string, slider, input, band);
}

void EqualizerMenu::onSliderValueChanged(const i8 gain, const u8 band) {
    CustomInput* const dbInput = sliders[band].dbInput;
    dbInput->setText(QString::number(gain));

    settings.gains[band] = gain;
    emit gainChanged(band);
}

void EqualizerMenu::changeBands(const QString& count) {
    settings.bandCount = Bands(count.toUInt());
    buildBands();

    presetSelect->setCurrentIndex(0);

    switch (settings.bandCount) {
        case Bands::Three:
        case Bands::Five:
            presetSelect->setEnabled(false);
            newPresetButton->setEnabled(false);
            deletePresetButton->setEnabled(false);
            break;
        default:
            presetSelect->setEnabled(true);
            newPresetButton->setEnabled(true);

            while (presetSelect->count() > DEFAULT_PRESET_COUNT) {
                presetSelect->removeItem(presetSelect->count() - 1);
            }
            break;
    }

    presetSelect->addItems(settings.presets[settings.bandCount].keys());
}

void EqualizerMenu::selectPreset(const i32 index) {
    if (previousPresetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(previousPresetIndex);

        settings.presets[settings.bandCount][presetName] =
            QVector<i8>(isize(settings.bandCount));

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, u8(settings.bandCount)))) {
            settings.presets[settings.bandCount][presetName][idx] =
                i8(container.dbInput->text().toInt());
        }
    }

    QString presetName;

    if (index < DEFAULT_PRESET_COUNT) {
        presetSelect->setEditable(false);
        deletePresetButton->setEnabled(false);
    } else {
        presetSelect->setEditable(true);
        presetName = presetSelect->itemText(index);
        deletePresetButton->setEnabled(true);

        if (!settings.presets[settings.bandCount].contains(presetName)) {
            settings.presets[settings.bandCount].insert(
                presetName,
                QVector<i8>(isize(settings.bandCount))
            );
        }
    }

    for (const auto& [idx, elements] :
         views::enumerate(views::take(sliders, u8(settings.bandCount)))) {
        i8 gain = 0;

        if (index < DEFAULT_PRESET_COUNT) {
            switch (settings.bandCount) {
                case Bands::Ten:
                    gain = PRESETS_10[index][idx];
                    break;
                case Bands::Eighteen:
                    gain = PRESETS_18[index][idx];
                    break;
                case Bands::Thirty:
                    gain = PRESETS_30[index][idx];
                    break;
                default:
                    return;
            }
        } else {
            const u16 newIndex = index - DEFAULT_PRESET_COUNT;
            gain = settings.presets[settings.bandCount][presetName][idx];
        }

        elements.slider->setValue(gain);
    }

    previousPresetIndex = index;
}

void EqualizerMenu::toggleEqualizer(const bool checked) {
    settings.enabled = checked;
    toggleButton->setText(checked ? tr("Enabled") : tr("Disabled"));
}

void EqualizerMenu::resetGains() {
    for (const auto& container : views::take(sliders, u8(settings.bandCount))) {
        container.slider->setValue(0);
        container.dbInput->setText(u"0"_s);
    }
}

void EqualizerMenu::createNewPreset() {
    presetSelect->addItem(tr("New preset"));
    presetSelect->setCurrentIndex(presetSelect->count() - 1);
}

void EqualizerMenu::retranslate() {
    ui->retranslateUi(this);

    if (settings.enabled) {
        toggleButton->setText(tr("Enabled"));
    }

    const span<const f32> frequencies = span(
        getFrequenciesForBands(settings.bandCount),
        usize(settings.bandCount)
    );

    for (const auto& [container, frequency] : views::zip(
             views::take(sliders, u8(settings.bandCount)),
             views::take(frequencies, u8(settings.bandCount))
         )) {
        container.dbLabel->setText(tr("dB"));
        container.hzLabel->setText(QString::number(frequency) + tr(" Hz"));
    }
}

void EqualizerMenu::loadSettings() {
    toggleButton->setChecked(settings.enabled);
    bandSelect->setCurrentIndex(settings.bandIndex);

    changeBands(bandSelect->currentText());

    presetSelect->setCurrentIndex(settings.presetIndex);
}
