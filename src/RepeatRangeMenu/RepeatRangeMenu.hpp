#pragma once

#include "CustomInput.hpp"
#include "DurationConversions.hpp"
#include "RangeSlider.hpp"
#include "TimeValidator.hpp"

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>

struct SkipSection {
    u16 start;
    u16 end;
};

class RepeatRangeMenu : public QDialog {
    Q_OBJECT

   public:
    explicit RepeatRangeMenu(QWidget* const parent = nullptr) :
        QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog) {
        skipSections_.reserve(4);
        auto* const mainLayout = new QHBoxLayout(this);

        auto* const verticalLayout = new QVBoxLayout(this);
        auto* const treeWidgetLayout = new QVBoxLayout(this);

        startTimeLayout->addWidget(startTimeLabel);
        startTimeLayout->addWidget(startTimeInput);

        endTimeLayout->addWidget(endTimeLabel);
        endTimeLayout->addWidget(endTimeInput);

        verticalLayout->addWidget(startTimeContainer);
        verticalLayout->addWidget(endTimeContainer);
        verticalLayout->addWidget(rangeSlider);

        treeWidgetLayout->addWidget(
            new QLabel(tr("Skip sections"), treeWidget)
        );
        treeWidgetLayout->addWidget(treeWidget);

        auto* const addButton = new QPushButton(this);
        addButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));

        treeWidgetLayout->addWidget(addButton);

        treeWidget->setHeaderLabels({ tr("Start Time"), tr("End Time") });

        mainLayout->addLayout(verticalLayout);
        mainLayout->addLayout(treeWidgetLayout);

        startTimeInput->setValidator(timeValidator);
        endTimeInput->setValidator(timeValidator);

        connect(
            rangeSlider,
            &RangeSlider::lowValueChanged,
            this,
            [&](const u16 second) -> void {
            startTimeInput->setText(secsToMins(second));
            startSecond_ = second;
        }
        );

        connect(
            rangeSlider,
            &RangeSlider::highValueChanged,
            this,
            [&](const u16 second) -> void {
            endTimeInput->setText(secsToMins(second));
            endSecond_ = second;
        }
        );

        connect(
            startTimeInput,
            &CustomInput::editingFinished,
            this,
            [&] -> void {
            rangeSlider->setLowValue(timeToSecs(startTimeInput->text()));
        }
        );

        connect(startTimeInput, &CustomInput::inputRejected, this, [&] -> void {
            startTimeInput->setText(secsToMins(rangeSlider->lowValue()));
        });

        connect(startTimeInput, &CustomInput::unfocused, this, [&] -> void {
            startTimeInput->setText(secsToMins(rangeSlider->lowValue()));
        });

        connect(endTimeInput, &CustomInput::editingFinished, this, [&] -> void {
            rangeSlider->setHighValue(timeToSecs(endTimeInput->text()));
        });

        connect(endTimeInput, &CustomInput::inputRejected, this, [&] -> void {
            endTimeInput->setText(secsToMins(rangeSlider->highValue()));
        });

        connect(endTimeInput, &CustomInput::unfocused, this, [&] -> void {
            endTimeInput->setText(secsToMins(rangeSlider->highValue()));
        });

        connect(addButton, &QPushButton::pressed, this, [&] -> void {
            auto* const item = new QTreeWidgetItem();
            item->setText(0, "00:00");
            item->setText(1, "00:00");
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            treeWidget->addTopLevelItem(item);
            skipSections_.emplace_back(0, 0);
        });

        connect(
            treeWidget,
            &QTreeWidget::itemDoubleClicked,
            this,
            [&](QTreeWidgetItem* const item, const i32 column) -> void {
            treeWidget->editItem(item, column);
        }
        );

        connect(
            treeWidget,
            &QTreeWidget::itemChanged,
            this,
            [&](QTreeWidgetItem* const item, const i32 column) -> void {
            QString text = item->text(column);
            i32 idx = 0;

            const QValidator::State state = timeValidator->validate(text, idx);

            if (state == QValidator::Invalid) {
                item->setText(column, "00:00");
            } else {
                u16 seconds = timeToSecs(text);

                if (seconds > endSecond_) {
                    item->setText(column, secsToMins(endSecond_));
                    seconds = endSecond_;
                }

                if (column == 0) {
                    skipSections_[0].start = seconds;
                } else {
                    skipSections_[0].end = seconds;
                }
            }
        }
        );
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

    [[nodiscard]] auto skipSections() const -> const vector<SkipSection>& {
        return skipSections_;
    }

   private:
    u16 startSecond_ = 0;
    u16 endSecond_ = 0;

    TimeValidator* const timeValidator = new TimeValidator(this);
    RangeSlider* const rangeSlider = new RangeSlider(this);

    QWidget* const endTimeContainer = new QWidget(this);
    QHBoxLayout* const endTimeLayout = new QHBoxLayout(endTimeContainer);

    QLabel* const endTimeLabel = new QLabel(tr("End time:"), endTimeContainer);
    CustomInput* const endTimeInput =
        new CustomInput(u"00:00"_s, endTimeContainer);

    QWidget* const startTimeContainer = new QWidget(this);
    QHBoxLayout* const startTimeLayout = new QHBoxLayout(startTimeContainer);

    QLabel* const startTimeLabel =
        new QLabel(tr("Start time:"), startTimeContainer);
    CustomInput* const startTimeInput =
        new CustomInput(u"00:00"_s, startTimeContainer);

    QTreeWidget* const treeWidget = new QTreeWidget(this);
    vector<SkipSection> skipSections_;
};
