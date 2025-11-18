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
    shared_ptr<Settings> settings_,
    QWidget* const parent
) :
    QDialog(parent, Qt::FramelessWindowHint | Qt::Popup),
    settings(std::move(settings_)),
    eqSettings(settings->equalizer) {
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

    connect(deletePresetButton, &QPushButton::pressed, this, [&] -> void {
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
        [&](const QString& text) -> void {
        const i32 currentIndex = presetSelect->currentIndex();
        const QString previousText = presetSelect->itemText(currentIndex);

        if (text == previousText) {
            return;
        }

        const QVector<i8> previousPreset =
            eqSettings.presets[eqSettings.bandCount][previousText];

        if (!eqSettings.presets[eqSettings.bandCount].contains(text)) {
            eqSettings.presets[eqSettings.bandCount].insert(
                text,
                QVector<i8>()
            );
        }

        std::swap(
            eqSettings.presets[eqSettings.bandCount][previousText],
            eqSettings.presets[eqSettings.bandCount][text]
        );

        presetSelect->setItemText(currentIndex, text);
        eqSettings.presets[eqSettings.bandCount].remove(previousText);
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
        new CustomInput(QString::number(eqSettings.gains[band]), dbContainer);
    auto* const intValidator = new IntValidator(MIN_GAIN, MAX_GAIN, dbInput);
    dbInput->setValidator(intValidator);
    dbInput->setFixedWidth(GAIN_INPUT_FIXED_WIDTH);

    connect(dbInput, &CustomInput::inputRejected, this, [&, band] -> void {
        onDbInputRejected(band);
    });

    connect(dbInput, &CustomInput::editingFinished, this, [&, band] -> void {
        onDbInputEditingFinished(band);
    });

    connect(dbInput, &CustomInput::unfocused, this, [&, band] -> void {
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
        QString::number(eqSettings.frequencies[band]) + tr(" Hz"),
        sliderContainer
    );

    hzLayout->addWidget(hzLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(hzContainer, 0, Qt::AlignCenter);

    connect(
        slider,
        &QSlider::valueChanged,
        this,
        [&, band](const i8 gain) -> void { onSliderValueChanged(gain, band); }
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

    eqSettings.frequencies = getFrequenciesForBands(eqSettings.bandCount);
    eqSettings.gains.fill(0);

    for (const u8 band : range(0, u8(eqSettings.bandCount))) {
        QWidget* const parent =
            band < u8(Bands::Thirty) / 2 ? firstSliderRow : secondSliderRow;
        QHBoxLayout* const parentLayout = band < u8(Bands::Thirty) / 2
                                              ? firstSliderLayout
                                              : secondSliderLayout;

        const SliderContainer elements = createSliderElement(band, parent);

        parentLayout->addWidget(elements.container);
        sliders[band] = elements;
    }

    firstSliderRow->resize(firstSliderLayout->sizeHint());
    secondSliderRow->resize(secondSliderLayout->sizeHint());

    if (u8(eqSettings.bandCount) < (u8(Bands::Thirty) / 2)) {
        secondSliderRow->hide();
    } else {
        secondSliderRow->show();
    }

    layout->invalidate();
    layout->activate();
    resize(layout->sizeHint());
}

void EqualizerMenu::saveSettings() {
    eqSettings.bandIndex = u8(bandSelect->currentIndex());

    const u16 presetIndex = u16(presetSelect->currentIndex());
    eqSettings.presetIndex = presetIndex;

    if (presetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(presetIndex);

        eqSettings.presets[eqSettings.bandCount][presetName] =
            QVector<i8>(isize(eqSettings.bandCount));

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, u8(eqSettings.bandCount)))) {
            eqSettings.presets[eqSettings.bandCount][presetName][idx] =
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

    eqSettings.gains[band] = gain;
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

    eqSettings.gains[band] = gain;
    emit gainChanged(band);
}

void EqualizerMenu::changeBands(const QString& count) {
    eqSettings.bandCount = Bands(count.toUInt());
    buildBands();

    presetSelect->setCurrentIndex(0);

    switch (eqSettings.bandCount) {
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

    presetSelect->addItems(
        settings->equalizer.presets[eqSettings.bandCount].keys()
    );
}

void EqualizerMenu::selectPreset(const i32 index) {
    if (previousPresetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(previousPresetIndex);

        eqSettings.presets[eqSettings.bandCount][presetName] =
            QVector<i8>(isize(eqSettings.bandCount));

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, u8(eqSettings.bandCount)))) {
            eqSettings.presets[eqSettings.bandCount][presetName][idx] =
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

        if (!eqSettings.presets[eqSettings.bandCount].contains(presetName)) {
            eqSettings.presets[eqSettings.bandCount].insert(
                presetName,
                QVector<i8>(isize(eqSettings.bandCount))
            );
        }
    }

    for (const auto& [idx, elements] :
         views::enumerate(views::take(sliders, u8(eqSettings.bandCount)))) {
        i8 gain = 0;

        if (index < DEFAULT_PRESET_COUNT) {
            switch (eqSettings.bandCount) {
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
            gain = eqSettings.presets[eqSettings.bandCount][presetName][idx];
        }

        elements.slider->setValue(gain);
    }

    previousPresetIndex = index;
}

void EqualizerMenu::toggleEqualizer(const bool checked) {
    eqSettings.enabled = checked;
    toggleButton->setText(checked ? tr("Enabled") : tr("Disabled"));
}

void EqualizerMenu::resetGains() {
    for (const auto& container :
         views::take(sliders, u8(eqSettings.bandCount))) {
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

    if (eqSettings.enabled) {
        toggleButton->setText(tr("Enabled"));
    }

    const span<const f32> frequencies = span(
        getFrequenciesForBands(eqSettings.bandCount),
        usize(eqSettings.bandCount)
    );

    for (const auto& [container, frequency] : views::zip(
             views::take(sliders, u8(eqSettings.bandCount)),
             views::take(frequencies, u8(eqSettings.bandCount))
         )) {
        container.dbLabel->setText(tr("dB"));
        container.hzLabel->setText(QString::number(frequency) + tr(" Hz"));
    }
}

void EqualizerMenu::loadSettings() {
    toggleButton->setChecked(settings->equalizer.enabled);
    bandSelect->setCurrentIndex(settings->equalizer.bandIndex);

    changeBands(bandSelect->currentText());

    presetSelect->setCurrentIndex(settings->equalizer.presetIndex);
}
