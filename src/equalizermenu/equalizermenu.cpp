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
        audioWorker->setEqEnabled(checked);
    }
    );
    bottomLayout->setAlignment(Qt::AlignHCenter);
    bottomLayout->addWidget(eqToggleButton);
    layout->addWidget(bottomContainer);

    this->move(parentButton->mapToGlobal(QPoint(0, parentButton->height())));
    this->show();
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
            QString::number(audioWorker->getGain(band)),
            sliderContainer
        );
        const auto* validator = new QIntValidator(MIN_DB, MAX_DB, dbInput);
        dbInput->setValidator(validator);
        dbInput->setFixedWidth(GAIN_EDIT_WIDTH);

        auto* dbLabel = new QLabel("dB", sliderContainer);

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
            format("{}Hz", audioWorker->bands()[band]).c_str(),
            sliderContainer
        );

        sliderLayout->addWidget(slider, 0, Qt::AlignHCenter);
        sliderLayout->addWidget(hzLabel, 0, Qt::AlignHCenter);

        middleLayout->addWidget(sliderContainer);
        sliders[band] = slider;
    }

    layout->insertWidget(1, middleContainer);
}

auto EqualizerMenu::getEqInfo()
    -> tuple<bool, u8, db_gains_array, frequencies_array> {
    return { eqToggleButton->isChecked(),
             bandsSelect->currentIndex(),
             audioWorker->gains(),
             audioWorker->bands() };
}

void EqualizerMenu::setEqInfo(
    const bool enabled,
    const u8 bandIndex,
    const db_gains_array gains,
    const frequencies_array frequencies
) {
    eqToggleButton->setChecked(enabled);
    bandsSelect->setCurrentIndex(bandIndex);

    for (u8 band = 0; band < bandsSelect->currentText().toUInt(); band++) {
        sliders[band]->setValue(gains[band]);
    }
};