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
    explicit RepeatRangeMenu(QWidget* parent = nullptr);

    void setDuration(const u16 seconds) {
        startSecond_ = 0;
        endSecond_ = seconds;

        treeWidget->clear();

        rangeSlider->setLowValue(0);
        rangeSlider->setMaximum(seconds);
        rangeSlider->setHighValue(seconds);

        startTimeInput->setText(u"00:00"_s);
        endTimeInput->setText(secsToMins(seconds));
    }

    [[nodiscard]] auto startSecond() const -> u16 { return startSecond_; }

    [[nodiscard]] auto endSecond() const -> u16 { return endSecond_; }

    [[nodiscard]] auto skipSections() const -> const vector<SkipSection>& {
        return skipSections_;
    }

   private:
    inline void updateSkipSection(QTreeWidgetItem* item, i32 column);
    inline void addSkipSection();
    inline void removeSkipSection();

    vector<SkipSection> skipSections_;

    QHBoxLayout* const mainLayout = new QHBoxLayout(this);

    QWidget* const sliderLayoutWidget = new QWidget(this);
    QVBoxLayout* const sliderLayout = new QVBoxLayout(sliderLayoutWidget);

    QWidget* const treeLayoutWidget = new QWidget(this);
    QVBoxLayout* const treeWidgetLayout = new QVBoxLayout(treeLayoutWidget);

    TimeValidator* const timeValidator = new TimeValidator(sliderLayoutWidget);
    RangeSlider* const rangeSlider = new RangeSlider(sliderLayoutWidget);

    QWidget* const endTimeContainer = new QWidget(sliderLayoutWidget);
    QHBoxLayout* const endTimeLayout = new QHBoxLayout(endTimeContainer);

    QLabel* const endTimeLabel = new QLabel(tr("End time:"), endTimeContainer);
    CustomInput* const endTimeInput =
        new CustomInput(u"00:00"_s, endTimeContainer);

    QWidget* const startTimeContainer = new QWidget(sliderLayoutWidget);
    QHBoxLayout* const startTimeLayout = new QHBoxLayout(startTimeContainer);

    QLabel* const startTimeLabel =
        new QLabel(tr("Start time:"), startTimeContainer);
    CustomInput* const startTimeInput =
        new CustomInput(u"00:00"_s, startTimeContainer);

    QTreeWidget* const treeWidget = new QTreeWidget(treeLayoutWidget);

    QWidget* const buttonsContainer = new QWidget(treeLayoutWidget);
    QHBoxLayout* const buttonsContainerLayout =
        new QHBoxLayout(buttonsContainer);

    QPushButton* const addButton = new QPushButton(
        QIcon::fromTheme(QIcon::ThemeIcon::ListAdd),
        QString(),
        buttonsContainer
    );

    QPushButton* const removeButton = new QPushButton(
        QIcon::fromTheme(QIcon::ThemeIcon::ListRemove),
        QString(),
        buttonsContainer
    );

    u16 startSecond_ = 0;
    u16 endSecond_ = 0;
};
