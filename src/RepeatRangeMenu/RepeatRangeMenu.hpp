#pragma once

#include "CustomInput.hpp"
#include "DurationConversions.hpp"
#include "RangeSlider.hpp"
#include "TimeValidator.hpp"

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>

class RepeatRangeMenu : public QDialog {
    Q_OBJECT

   public:
    explicit RepeatRangeMenu(QWidget* parent = nullptr) :
        QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog) {
        auto* mainLayout = new QVBoxLayout(this);

        startTimeLayout->addWidget(startTimeLabel);
        startTimeLayout->addWidget(startTimeInput);

        endTimeLayout->addWidget(endTimeLabel);
        endTimeLayout->addWidget(endTimeInput);

        mainLayout->addWidget(startTimeContainer);
        mainLayout->addWidget(endTimeContainer);
        mainLayout->addWidget(rangeSlider);

        startTimeInput->setValidator(new TimeValidator());
        endTimeInput->setValidator(new TimeValidator());

        connect(
            rangeSlider,
            &RangeSlider::lowValueChanged,
            this,
            [&](const u16 second) {
            startTimeInput->setText(secsToMins(second));
            startSecond_ = second;
        }
        );

        connect(
            rangeSlider,
            &RangeSlider::highValueChanged,
            this,
            [&](const u16 second) {
            endTimeInput->setText(secsToMins(second));
            endSecond_ = second;
        }
        );

        connect(startTimeInput, &CustomInput::editingFinished, this, [&] {
            rangeSlider->setLowValue(timeToSecs(startTimeInput->text()));
        });

        connect(startTimeInput, &CustomInput::inputRejected, this, [&] {
            startTimeInput->setText(secsToMins(rangeSlider->lowValue()));
        });

        connect(startTimeInput, &CustomInput::unfocused, this, [&] {
            startTimeInput->setText(secsToMins(rangeSlider->lowValue()));
        });

        connect(endTimeInput, &CustomInput::editingFinished, this, [&] {
            rangeSlider->setHighValue(timeToSecs(endTimeInput->text()));
        });

        connect(endTimeInput, &CustomInput::inputRejected, this, [&] {
            endTimeInput->setText(secsToMins(rangeSlider->highValue()));
        });

        connect(endTimeInput, &CustomInput::unfocused, this, [&] {
            endTimeInput->setText(secsToMins(rangeSlider->highValue()));
        });
    }

    void setDuration(const u16 seconds) {
        endSecond_ = seconds;

        rangeSlider->setMaximum(seconds);
        rangeSlider->setHighValue(seconds);

        endTimeInput->setText(secsToMins(seconds));
    }

    void setStartSecond(const u16 second) {
        startSecond_ = second;

        rangeSlider->setLowValue(0);

        startTimeInput->setText(u"00:00"_s);
    }

    [[nodiscard]] auto startSecond() const -> u16 { return startSecond_; }

    [[nodiscard]] auto endSecond() const -> u16 { return endSecond_; }

   private:
    u16 startSecond_ = 0;
    u16 endSecond_ = 0;

    RangeSlider* rangeSlider = new RangeSlider(this);

    QWidget* endTimeContainer = new QWidget(this);
    QHBoxLayout* endTimeLayout = new QHBoxLayout(endTimeContainer);

    QLabel* endTimeLabel = new QLabel(tr("End time:"), endTimeContainer);
    CustomInput* endTimeInput = new CustomInput(u"00:00"_s, endTimeContainer);

    QWidget* startTimeContainer = new QWidget(this);
    QHBoxLayout* startTimeLayout = new QHBoxLayout(startTimeContainer);

    QLabel* startTimeLabel = new QLabel(tr("Start time:"), startTimeContainer);
    CustomInput* startTimeInput =
        new CustomInput(u"00:00"_s, startTimeContainer);
};
