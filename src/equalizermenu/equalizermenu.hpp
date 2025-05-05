#pragma once

#include "audioworker.hpp"
#include "constants.hpp"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

// TODO: Implement changeable frequencies for each band

class EqualizerMenu : public QDialog {
    Q_OBJECT

   public:
    explicit EqualizerMenu(QPushButton* parentButton, AudioWorker* audioWorker);

    void setEqualizerInfo(
        bool enabled,
        u8 bandIndex,
        const array<i8, THIRTY_BANDS>& gains,
        const array<f32, THIRTY_BANDS>& frequencies
    );
    auto equalizerInfo() -> EqualizerInfo;

   private:
    inline void buildBands(u8 bands);

    AudioWorker* audioWorker;
    QVBoxLayout* layout = new QVBoxLayout(this);

    QWidget* topContainer = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topContainer);

    QWidget* middleContainer = new QWidget(this);
    QHBoxLayout* middleLayout = new QHBoxLayout(middleContainer);

    QWidget* bottomContainer = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomContainer);

    QLabel* bandsLabel = new QLabel(tr("Bands:"), topContainer);
    QComboBox* bandsSelect = new QComboBox(topContainer);

    QPushButton* eqToggleButton = new QPushButton(bottomContainer);
    array<QSlider*, THIRTY_BANDS> sliders;
};
