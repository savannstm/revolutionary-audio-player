#pragma once

#include "audioworker.hpp"

#include <QMenu>
#include <QPushButton>

// TODO: Add bands configuration: 3-band, 5-band and 10-band
// TODO?: Maybe allow to change frequencies of bands.

class EqualizerMenu : public QMenu {
    Q_OBJECT

   public:
    EqualizerMenu(QPushButton* parentButton, AudioWorker* audioWorker);
};
