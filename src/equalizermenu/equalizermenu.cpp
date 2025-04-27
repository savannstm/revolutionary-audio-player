#include "equalizermenu.hpp"

#include "audiostreamer.hpp"
#include "custominput.hpp"

#include <QIntValidator>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <Qt>

EqualizerMenu::EqualizerMenu(
    QPushButton* parentButton,
    AudioWorker* audioWorker
) :
    QDialog(parentButton, Qt::FramelessWindowHint | Qt::Dialog),
    audioWorker(audioWorker) {
    bandsSelect->addItems({ "3", "5", "10" });
    bandsSelect->setCurrentIndex(2);

    connect(
        bandsSelect,
        &QComboBox::currentTextChanged,
        this,
        [=, this](const QString& text) {
        const u8 bandsCount = text.toUInt();
        audioWorker->setBands(bandsCount);
        buildBands(bandsCount);
    }
    );

    topLayout->setAlignment(Qt::AlignHCenter);
    topLayout->addWidget(bandsLabel);
    topLayout->addWidget(bandsSelect);
    layout->addWidget(topContainer);

    buildBands(bandsSelect->itemText(2).toUInt());

    eqToggleButton->setText("Equalizer disabled");
    eqToggleButton->setCheckable(true);
    connect(
        eqToggleButton,
        &QPushButton::toggled,
        this,
        [=, this](const bool checked) {
        eqToggleButton->setText(
            checked ? "Equalizer enabled" : "Equalizer disabled"
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

    for (u8 band = 0; band < bands; band++) {
        auto* sliderContainer = new QWidget(this);
        auto* sliderLayout = new QVBoxLayout(sliderContainer);
        sliderLayout->setAlignment(Qt::AlignHCenter);

        auto* dbContainer = new QWidget(sliderContainer);
        auto* dbLayout = new QHBoxLayout(dbContainer);
        dbLayout->setAlignment(Qt::AlignHCenter);

        auto* dbInput = new CustomInput(
            QString::number(audioWorker->gain(band)),
            sliderContainer
        );
        const auto* validator = new QIntValidator(MIN_DB, MAX_DB, dbInput);
        dbInput->setValidator(validator);
        dbInput->setFixedWidth(GAIN_EDIT_WIDTH);

        auto* dbLabel = new QLabel(u"dB"_s, sliderContainer);

        dbLayout->addWidget(dbInput);
        dbLayout->addWidget(dbLabel);
        sliderLayout->addWidget(dbContainer, 0, Qt::AlignHCenter);

        auto* slider = new QSlider(Qt::Orientation::Vertical, sliderContainer);
        slider->setRange(MIN_DB, MAX_DB);

        connect(dbInput, &CustomInput::returnPressed, dbInput, [=, this] {
            slider->setValue(dbInput->text().toInt());
        });

        connect(
            slider,
            &QSlider::valueChanged,
            slider,
            [=, this](const i8 dbGain) {
            dbInput->setText(QString::number(dbGain));
            audioWorker->setGain(dbGain, band);
        }
        );

        auto* hzLabel = new QLabel(
            u"%1Hz"_s.arg(audioWorker->bands()[band]),
            sliderContainer
        );

        sliderLayout->addWidget(slider, 0, Qt::AlignHCenter);
        sliderLayout->addWidget(hzLabel, 0, Qt::AlignHCenter);

        middleLayout->addWidget(sliderContainer);
        sliders[band] = slider;
    }

    layout->insertWidget(1, middleContainer);
}

auto EqualizerMenu::getEqualizerInfo()
    -> tuple<bool, u8, const vector<i8>&, const vector<f32>&> {
    return { eqToggleButton->isChecked(),
             bandsSelect->currentIndex(),
             audioWorker->gains(),
             audioWorker->bands() };
}

void EqualizerMenu::setEqualizerInfo(
    const bool enabled,
    const u8 bandIndex,
    const vector<i8>& gains,
    const vector<f32>& frequencies
) {
    eqToggleButton->setChecked(enabled);
    bandsSelect->setCurrentIndex(bandIndex);

    for (u8 band = 0; band < bandsSelect->currentText().toUInt(); band++) {
        sliders[band]->setValue(gains[band]);
    }
};