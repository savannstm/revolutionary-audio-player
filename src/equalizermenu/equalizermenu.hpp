#pragma once

#include "audiostreamer.hpp"
#include "audioworker.hpp"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

constexpr i8 MAX_DB = 20;
constexpr i8 MIN_DB = -20;
constexpr u8 GAIN_EDIT_WIDTH = 32;

class EqualizerMenu : public QDialog {
    Q_OBJECT

   public:
    explicit EqualizerMenu(QPushButton* parentButton, AudioWorker* audioWorker);

    void setEqInfo(
        bool enabled,
        u8 bandIndex,
        db_gains_array gains,
        frequencies_array frequencies
    );
    auto getEqInfo() -> tuple<bool, u8, db_gains_array, frequencies_array>;

   private:
    void buildBands(u8 bands);

    AudioWorker* audioWorker;
    QVBoxLayout* layout = new QVBoxLayout(this);

    QWidget* topContainer = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topContainer);

    QWidget* middleContainer = new QWidget(this);
    QHBoxLayout* middleLayout = new QHBoxLayout(middleContainer);

    QWidget* bottomContainer = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomContainer);

    QLabel* bandsLabel = new QLabel("Bands:", topContainer);
    QComboBox* bandsSelect = new QComboBox(topContainer);

    QPushButton* eqToggleButton = new QPushButton(bottomContainer);
    array<QSlider*, EQ_BANDS_N> sliders;
};
