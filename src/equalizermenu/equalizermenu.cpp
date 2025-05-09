#include "equalizermenu.hpp"

#include "constants.hpp"
#include "custominput.hpp"

#include <QIntValidator>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

EqualizerMenu::EqualizerMenu(
    QPushButton* parentButton,
    AudioWorker* audioWorker
) :
    QDialog(parentButton, Qt::FramelessWindowHint | Qt::Dialog),
    audioWorker(audioWorker) {
    bandsSelect->addItems({ "3", "5", "10", "18", "30" });
    bandsSelect->setCurrentIndex(2);

    connect(
        bandsSelect,
        &QComboBox::currentTextChanged,
        this,
        [=, this](const QString& count) {
        const u8 bandsCount = count.toUInt();
        audioWorker->setBandCount(bandsCount);
        buildBands(bandsCount);
    }
    );

    topLayout->setAlignment(Qt::AlignHCenter);
    topLayout->addWidget(bandsLabel);
    topLayout->addWidget(bandsSelect);
    layout->addWidget(topContainer);

    eqToggleButton->setText(tr("Equalizer disabled"));
    eqToggleButton->setCheckable(true);
    connect(
        eqToggleButton,
        &QPushButton::toggled,
        this,
        [=, this](const bool checked) {
        eqToggleButton->setText(
            checked ? tr("Equalizer enabled") : tr("Equalizer disabled")
        );
        audioWorker->toggleEqualizer(checked);
    }
    );
    bottomLayout->setAlignment(Qt::AlignHCenter);
    bottomLayout->addWidget(eqToggleButton);
    layout->addWidget(bottomContainer);
}

void EqualizerMenu::buildBands(const u8 bands) {
    delete middleContainer;

    middleContainer = new QWidget(this);
    middleLayout = new QHBoxLayout(middleContainer);

    const auto frequencies = audioWorker->bands();

    for (u8 band = 0; band < bands; band++) {
        auto* sliderContainer = new QWidget(this);
        auto* sliderLayout = new QVBoxLayout(sliderContainer);
        sliderLayout->setAlignment(Qt::AlignHCenter);

        auto* dbContainer = new QWidget(sliderContainer);
        auto* dbLayout = new QHBoxLayout(dbContainer);
        dbLayout->setAlignment(Qt::AlignHCenter);

        auto* hzContainer = new QWidget(sliderContainer);
        auto* hzLayout = new QHBoxLayout(hzContainer);
        hzLayout->setAlignment(Qt::AlignHCenter);

        auto* dbInput = new CustomInput(
            QString::number(audioWorker->gain(band)),
            sliderContainer
        );
        const auto* validator = new QIntValidator(MIN_DB, MAX_DB, dbInput);
        dbInput->setValidator(validator);
        dbInput->setFixedWidth(GAIN_INPUT_FIXED_WIDTH);

        auto* dbLabel = new QLabel(tr("dB"), sliderContainer);

        dbLayout->addWidget(dbInput);
        dbLayout->addWidget(dbLabel);
        sliderLayout->addWidget(dbContainer, 0, Qt::AlignHCenter);

        auto* slider = new QSlider(Qt::Vertical, sliderContainer);
        slider->setRange(MIN_DB, MAX_DB);

        auto* hzInput = new CustomInput(
            QString::number(frequencies[band]),
            sliderContainer
        );
        auto* hzLabel = new QLabel(tr("Hz"), sliderContainer);

        hzLayout->addWidget(hzInput);
        hzLayout->addWidget(hzLabel);

        sliderLayout->addWidget(slider, 0, Qt::AlignHCenter);
        sliderLayout->addWidget(hzContainer, 0, Qt::AlignHCenter);

        connect(dbInput, &CustomInput::returnPressed, this, [=, this] {
            slider->setValue(dbInput->text().toInt());
            audioWorker->setGain(as<i8>(dbInput->text().toInt()), band);
        });

        connect(
            slider,
            &QSlider::valueChanged,
            this,
            [=, this](const i8 dbGain) {
            dbInput->setText(QString::number(dbGain));
            audioWorker->setGain(dbGain, band);
        }
        );

        connect(hzInput, &CustomInput::returnPressed, this, [=, this] {
            audioWorker->setFrequency(hzInput->text().toFloat(), band);
        });

        middleLayout->addWidget(sliderContainer);
        sliders[band] = slider;
    }

    layout->insertWidget(1, middleContainer);
}

auto EqualizerMenu::equalizerInfo() -> EqualizerInfo {
    return { .enabled = eqToggleButton->isChecked(),
             .bandIndex = as<u8>(bandsSelect->currentIndex()),
             .gains = audioWorker->gains(),
             .frequencies = audioWorker->bands() };
}

void EqualizerMenu::setEqualizerInfo(
    const bool enabled,
    const u8 bandIndex,
    const array<i8, THIRTY_BANDS>& gains,
    const array<f32, THIRTY_BANDS>& frequencies
) {
    eqToggleButton->setChecked(enabled);
    bandsSelect->setCurrentIndex(bandIndex);

    const u8 bandCount = bandsSelect->currentText().toUInt();
    audioWorker->setBandCount(bandCount);
    buildBands(bandCount);

    for (u8 band = 0; band < bandCount; band++) {
        sliders[band]->setValue(gains[band]);
        audioWorker->setGain(gains[band], band);
    }
};