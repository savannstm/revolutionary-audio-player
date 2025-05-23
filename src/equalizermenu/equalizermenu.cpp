#include "equalizermenu.hpp"

#include "constants.hpp"
#include "custominput.hpp"
#include "doublevalidator.hpp"
#include "equalizerpresets.hpp"
#include "intvalidator.hpp"
#include "invalidator.hpp"
#include "settings.hpp"

#include <QKeyEvent>
#include <QSlider>

EqualizerMenu::EqualizerMenu(
    QWidget* parent,
    AudioWorker* audioWorker_,
    shared_ptr<Settings> settings_
) :
    QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog),
    audioWorker(audioWorker_),
    settings(std::move(settings_)) {
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
        enableEqualizerButton,
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

    connect(deletePresetButton, &QPushButton::pressed, this, [&] {
        const u16 index = presetSelect->currentIndex();
        presetSelect->setCurrentIndex(0);
        presetSelect->removeItem(index);
    });

    // Prevent inserting new entry on editingFinished
    presetSelect->setValidator(new Invalidator(presetSelect));

    // TODO: Inefficient, connect to line edit directly.
    connect(
        presetSelect,
        &QComboBox::editTextChanged,
        this,
        [&](const QString& text) {
        const i32 currentIndex = presetSelect->currentIndex();
        const QString previousText = presetSelect->itemText(currentIndex);

        if (text == previousText) {
            return;
        }

        const QVector<i8> previousPreset =
            eqSettings.presets[bandCount][previousText];

        if (!eqSettings.presets[bandCount].contains(text)) {
            eqSettings.presets[bandCount].insert(text, QVector<i8>());
        }

        std::swap(
            eqSettings.presets[bandCount][previousText],
            eqSettings.presets[bandCount][text]
        );

        presetSelect->setItemText(currentIndex, text);
        eqSettings.presets[bandCount].remove(previousText);
    }
    );
}

auto EqualizerMenu::createSliderElement(
    const u8 band,
    const FrequencyArray& frequencies,
    QWidget* parent
) const -> SliderContainer {
    // Slider container
    auto* sliderContainer = new QWidget(parent);
    auto* sliderLayout = new QVBoxLayout(sliderContainer);
    sliderLayout->setAlignment(Qt::AlignCenter);

    auto* slider = new QSlider(Qt::Vertical, sliderContainer);
    slider->setRange(MIN_GAIN, MAX_GAIN);

    // dB Container
    auto* dbContainer = new QWidget(sliderContainer);
    auto* dbLayout = new QHBoxLayout(dbContainer);
    dbLayout->setAlignment(Qt::AlignCenter);

    auto* dbInput =
        new CustomInput(QString::number(audioWorker->gain(band)), dbContainer);
    auto* intValidator = new IntValidator(MIN_GAIN, MAX_GAIN, dbInput);
    dbInput->setValidator(intValidator);
    dbInput->setFixedWidth(GAIN_INPUT_FIXED_WIDTH);

    connect(dbInput, &CustomInput::inputRejected, this, [&, band] {
        onDbInputRejected(band);
    });

    connect(dbInput, &CustomInput::editingFinished, this, [&, band] {
        onDbInputEditingFinished(band);
    });

    connect(dbInput, &CustomInput::unfocused, this, [&, band] {
        onDbInputUnfocused(band);
    });

    auto* dbLabel = new QLabel(tr("dB"), dbContainer);

    dbLayout->addWidget(dbInput);
    dbLayout->addWidget(dbLabel);
    sliderLayout->addWidget(dbContainer, 0, Qt::AlignCenter);

    // Adding slider to the center
    sliderLayout->addWidget(slider, 0, Qt::AlignCenter);

    // Hz container
    auto* hzContainer = new QWidget(sliderContainer);
    auto* hzLayout = new QHBoxLayout(hzContainer);
    hzLayout->setAlignment(Qt::AlignCenter);

    auto* hzLabel =
        new QLabel(tr("%1 Hz").arg(frequencies[band]), sliderContainer);

    hzLayout->addWidget(hzLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(hzContainer, 0, Qt::AlignCenter);

    connect(slider, &QSlider::valueChanged, this, [&, band](const i8 value) {
        onSliderValueChanged(value, band);
    });

    return { .container = sliderContainer,
             .slider = slider,
             .dbLabel = dbLabel,
             .dbInput = dbInput,
             .hzLabel = hzLabel };
}

void EqualizerMenu::buildBands() {
    audioWorker->setBandCount(bandCount);

    const auto firstSliderRowChildren = firstSliderRow->children();
    for (QObject* child : firstSliderRowChildren) {
        QWidget* childWidget = qobject_cast<QWidget*>(child);
        delete childWidget;
    }

    const auto secondSliderRowChildren = secondSliderRow->children();
    for (QObject* child : secondSliderRowChildren) {
        QWidget* childWidget = qobject_cast<QWidget*>(child);
        delete childWidget;
    }

    const FrequencyArray& frequencies = audioWorker->frequencies();

    for (const u8 band : range(0, bandCount)) {
        QWidget* parent =
            band < MAX_BANDS_COUNT / 2 ? firstSliderRow : secondSliderRow;
        QHBoxLayout* parentLayout =
            band < MAX_BANDS_COUNT / 2 ? firstSliderLayout : secondSliderLayout;

        const auto elements = createSliderElement(band, frequencies, parent);

        parentLayout->addWidget(elements.container);
        sliders[band] = elements;
    }

    firstSliderRow->resize(firstSliderLayout->sizeHint());
    secondSliderRow->resize(secondSliderLayout->sizeHint());

    if (bandCount < MAX_BANDS_COUNT / 2) {
        secondSliderRow->hide();
    } else {
        secondSliderRow->show();
    }

    layout->invalidate();
    layout->activate();
    resize(layout->sizeHint());
}

void EqualizerMenu::saveSettings() {
    eqSettings.enabled = enableEqualizerButton->isChecked();
    eqSettings.bandIndex = as<u8>(bandSelect->currentIndex());

    const u16 presetIndex = as<u16>(presetSelect->currentIndex());
    eqSettings.presetIndex = presetIndex;

    if (presetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(presetIndex);

        eqSettings.presets[bandCount][presetName] = QVector<i8>(bandCount);

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, bandCount))) {
            eqSettings.presets[bandCount][presetName][idx] =
                as<i8>(container.dbInput->text().toInt());
        }
    }
}

void EqualizerMenu::keyPressEvent(QKeyEvent* event) {
    event->ignore();
}

void EqualizerMenu::updateGain(
    const QString& string,
    QSlider* slider,
    CustomInput* input,
    const u8 band
) const {
    const i8 value = as<i8>(string.toInt());
    slider->setValue(value);
    audioWorker->setGain(value, band);
    input->clearFocus();
}

void EqualizerMenu::onDbInputRejected(const u8 band) const {
    QSlider* slider = sliders[band].slider;
    CustomInput* input = sliders[band].dbInput;
    const auto* validator = as<const IntValidator*>(input->validator());

    QString string = input->text();
    i32 pos = 0;

    if (validator->validate(string, pos) == QValidator::Invalid) {
        return;
    }

    validator->fixup(string);
    input->setText(string);

    updateGain(string, slider, input, band);
}

void EqualizerMenu::onDbInputEditingFinished(const u8 band) const {
    QSlider* slider = sliders[band].slider;
    CustomInput* input = sliders[band].dbInput;

    updateGain(input->text(), slider, input, band);
}

void EqualizerMenu::onDbInputUnfocused(const u8 band) const {
    QSlider* slider = sliders[band].slider;
    CustomInput* input = sliders[band].dbInput;
    const auto* validator = as<const IntValidator*>(input->validator());

    QString string = input->text();
    validator->fixup(string);
    input->setText(string);

    updateGain(string, slider, input, band);
}

void EqualizerMenu::onSliderValueChanged(const i8 value, const u8 band) const {
    CustomInput* dbInput = sliders[band].dbInput;

    dbInput->setText(QString::number(value));
    audioWorker->setGain(value, band);
}

void EqualizerMenu::changeBands(const QString& count) {
    bandCount = count.toUInt();
    buildBands();

    presetSelect->setCurrentIndex(0);

    switch (bandCount) {
        case THREE_BANDS:
        case FIVE_BANDS:
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
        settings->equalizerSettings.presets[bandCount].keys()
    );
}

void EqualizerMenu::selectPreset(i32 index) {
    if (previousPresetIndex > DEFAULT_PRESET_COUNT) {
        const QString presetName = presetSelect->itemText(previousPresetIndex);

        eqSettings.presets[bandCount][presetName] = QVector<i8>(bandCount);

        for (const auto& [idx, container] :
             views::enumerate(views::take(sliders, bandCount))) {
            eqSettings.presets[bandCount][presetName][idx] =
                as<i8>(container.dbInput->text().toInt());
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

        if (!eqSettings.presets[bandCount].contains(presetName)) {
            eqSettings.presets[bandCount].insert(
                presetName,
                QVector<i8>(bandCount)
            );
        }
    }

    for (const auto& [idx, elements] :
         views::enumerate(views::take(sliders, bandCount))) {
        i8 gain = 0;

        if (index < DEFAULT_PRESET_COUNT) {
            switch (bandCount) {
                case TEN_BANDS:
                    gain = PRESETS_10[index][idx];
                    break;
                case EIGHTEEN_BANDS:
                    // TODO
                    break;
                case THIRTY_BANDS:
                    break;
                default:
                    return;
            }
        } else {
            const u16 newIndex = index - DEFAULT_PRESET_COUNT;
            gain = eqSettings.presets[bandCount][presetName][idx];
        }

        elements.slider->setValue(gain);
    }

    previousPresetIndex = index;
}

void EqualizerMenu::toggleEqualizer(const bool checked) {
    enableEqualizerButton->setText(checked ? tr("Enabled") : tr("Disabled"));
    audioWorker->toggleEqualizer(checked);
}

void EqualizerMenu::resetGains() {
    for (const auto& container : views::take(sliders, bandCount)) {
        container.slider->setValue(0);
        container.dbInput->setText("0");
    }
}

void EqualizerMenu::createNewPreset() {
    presetSelect->addItem(tr("New preset"));
    presetSelect->setCurrentIndex(presetSelect->count() - 1);
}
