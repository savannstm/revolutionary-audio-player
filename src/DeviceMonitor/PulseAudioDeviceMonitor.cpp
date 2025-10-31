// TODO
#include "PulseAudioDeviceMonitor.hpp"

PulseAudioDeviceMonitor::PulseAudioDeviceMonitor(QObject* parent) :
    QObject(parent) {}

PulseAudioDeviceMonitor::~PulseAudioDeviceMonitor() = default;

auto PulseAudioDeviceMonitor::initialize() -> bool {
    return false;
}