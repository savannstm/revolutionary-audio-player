#include "RepeatRangeMenu.hpp"

RepeatRangeMenu::RepeatRangeMenu(QWidget* const parent) :
    QDialog(parent, Qt::FramelessWindowHint | Qt::Popup) {
    skipSections_.reserve(4);

    startTimeLayout->addWidget(startTimeLabel);
    startTimeLayout->addWidget(startTimeInput);

    endTimeLayout->addWidget(endTimeLabel);
    endTimeLayout->addWidget(endTimeInput);

    sliderLayout->addWidget(startTimeContainer);
    sliderLayout->addWidget(endTimeContainer);
    sliderLayout->addWidget(rangeSlider);

    treeWidgetLayout->addWidget(new QLabel(tr("Skip sections"), treeWidget));
    treeWidgetLayout->addWidget(treeWidget);

    buttonsContainerLayout->addWidget(addButton);
    buttonsContainerLayout->addWidget(removeButton);

    treeWidgetLayout->addWidget(buttonsContainer);

    treeWidget->setHeaderLabels({ tr("Start Time"), tr("End Time") });

    mainLayout->addWidget(sliderLayoutWidget);
    mainLayout->addWidget(treeLayoutWidget);

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

    connect(startTimeInput, &CustomInput::editingFinished, this, [&] -> void {
        rangeSlider->setLowValue(timeToSecs(startTimeInput->text()));
    });

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

    connect(removeButton, &QPushButton::pressed, this, [&] -> void {
        const QModelIndex currentIndex = treeWidget->currentIndex();

        if (currentIndex.isValid()) {
            treeWidget->model()->removeRow(currentIndex.row());
            skipSections_.erase(skipSections_.begin() + currentIndex.row());
        }
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
            return;
        }

        u16 seconds = timeToSecs(text);

        if (seconds > endSecond_) {
            item->setText(column, secsToMins(endSecond_));
            seconds = endSecond_;
        }

        if (column == 0) {
            skipSections_[0].start = seconds;
        } else {
            if (seconds > skipSections_[0].start) {
                skipSections_[0].end = skipSections_[0].start;
                return;
            }

            skipSections_[0].end = seconds;
        }
    }
    );
}
