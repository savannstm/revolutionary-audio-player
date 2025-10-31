// TODO
#pragma once

#include <QObject>

class PulseAudioDeviceMonitor : public QObject {
   public:
    explicit PulseAudioDeviceMonitor(QObject* parent = nullptr);
    ~PulseAudioDeviceMonitor() override;

    auto initialize() -> bool;
};