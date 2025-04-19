#include "equalizermenu.hpp"

#include "audiostreamer.hpp"
#include "custominput.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QVBoxLayout>
#include <QValidator>
#include <QWidgetAction>

constexpr i8 MAX_DB = 20;
constexpr i8 MIN_DB = -20;
constexpr u8 GAIN_EDIT_WIDTH = 32;

EqualizerMenu::EqualizerMenu(
    QPushButton* parentButton,
    AudioWorker* audioWorker
) :
    QMenu(parentButton) {
    auto* container = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(container);
    auto* hLayout = new QHBoxLayout();

    QVector<QWidget*> bandWidgets;

    for (u8 i = 0; i < EQ_BANDS_N; i++) {
        const i8 dbGain = audioWorker->getGain(i);

        auto* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignHCenter);

        auto* bandWidget = new QWidget();
        auto* bandLayout = new QVBoxLayout(bandWidget);
        bandLayout->setAlignment(Qt::AlignHCenter);

        auto* gainEdit = new CustomInput(QString::number(dbGain));
        gainEdit->setFixedWidth(GAIN_EDIT_WIDTH);
        gainEdit->setAlignment(Qt::AlignRight);
        gainEdit->setValidator(new QIntValidator(MIN_DB, MAX_DB, gainEdit));

        auto* dbLabel = new QLabel("dB");

        auto* slider = new QSlider(Qt::Vertical);
        slider->setRange(MIN_DB, MAX_DB);
        slider->setValue(dbGain);

        auto* bottomLabel =
            new QLabel(QString::fromStdString(format("{} Hz", FREQUENCIES[i])));

        connect(slider, &QSlider::valueChanged, this, [=](const i8 gain) {
            gainEdit->setText(QString::number(gain));
            audioWorker->setGain(gain, i);
            qDebug() << "value changed";
        });

        connect(gainEdit, &CustomInput::editingFinished, this, [=] {
            slider->setValue(gainEdit->text().toInt());
        });

        auto* topLabelLayout = new QHBoxLayout();
        topLabelLayout->addWidget(gainEdit);
        topLabelLayout->addWidget(dbLabel);

        bandLayout->addLayout(topLabelLayout, 0);
        bandLayout->addWidget(slider, 0, Qt::AlignHCenter);
        bandLayout->addWidget(bottomLabel, 0, Qt::AlignHCenter);

        vLayout->addWidget(bandWidget);
        hLayout->addLayout(vLayout);

        bandWidgets.append(bandWidget);
    }

    for (auto* widget : bandWidgets) {
        widget->setFixedWidth(bandWidgets.last()->sizeHint().width());
    }

    auto* toggleButton = new QPushButton("Toggle Equalizer");
    toggleButton->setCheckable(true);
    toggleButton->setChecked(audioWorker->isEqEnabled());

    connect(toggleButton, &QPushButton::toggled, this, [=](const bool checked) {
        audioWorker->setEqEnabled(checked);
    });

    mainLayout->addLayout(hLayout);
    mainLayout->addWidget(toggleButton, 0, Qt::AlignHCenter);

    auto* widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(container);
    this->addAction(widgetAction);

    this->exec(parentButton->mapToGlobal(QPoint(0, parentButton->height())));
    this->deleteLater();
}
