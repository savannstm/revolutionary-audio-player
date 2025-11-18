#include "DeviceMonitor.hpp"

DeviceMonitor::DeviceMonitor(QObject* const parent) : QObject(parent) {
#ifdef Q_OS_WINDOWS
    deviceMonitor->initialize();
#endif

    connect(
        deviceMonitor,
#ifdef Q_OS_WINDOWS
        &WindowsDeviceMonitor::defaultDeviceChanged
#elifdef Q_OS_LINUX
        &PulseAudioDeviceMonitor::defaultDeviceChanged
#endif
        ,
        this,
        &DeviceMonitor::defaultDeviceChanged
    );

    connect(
        deviceMonitor,
#ifdef Q_OS_WINDOWS
        &WindowsDeviceMonitor::deviceAdded
#elifdef Q_OS_LINUX
        &PulseAudioDeviceMonitor::deviceAdded
#endif
        ,
        this,
        &DeviceMonitor::deviceAdded
    );

    connect(
        deviceMonitor,
#ifdef Q_OS_WINDOWS
        &WindowsDeviceMonitor::deviceRemoved
#elifdef Q_OS_LINUX
        &PulseAudioDeviceMonitor::deviceRemoved
#endif
        ,
        this,
        &DeviceMonitor::deviceRemoved
    );
}

DeviceMonitor::~DeviceMonitor() {
    delete deviceMonitor;
}