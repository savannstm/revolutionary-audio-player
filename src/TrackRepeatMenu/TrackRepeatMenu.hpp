#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

struct SkipSection {
    u16 start;
    u16 end;
};

class TrackRepeatMenu : public QDialog {
    Q_OBJECT

   public:
    explicit TrackRepeatMenu(QWidget* parent = nullptr);

    void setDuration(u16 seconds);

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

    QHBoxLayout* const mainLayout;

    QWidget* const sliderLayoutWidget;
    QVBoxLayout* const sliderLayout;

    QWidget* const treeLayoutWidget;
    QVBoxLayout* const treeWidgetLayout;

    TimeValidator* const timeValidator;
    RangeSlider* const rangeSlider;

    QWidget* const endTimeContainer;
    QHBoxLayout* const endTimeLayout;
    QLabel* const endTimeLabel;
    CustomInput* const endTimeInput;

    QWidget* const startTimeContainer;
    QHBoxLayout* const startTimeLayout;
    QLabel* const startTimeLabel;
    CustomInput* const startTimeInput;

    QTreeWidget* const treeWidget;

    QWidget* const buttonsContainer;
    QHBoxLayout* const buttonsContainerLayout;
    QPushButton* const addButton;
    QPushButton* const removeButton;

    u16 startSecond_ = 0;
    u16 endSecond_ = 0;
};
