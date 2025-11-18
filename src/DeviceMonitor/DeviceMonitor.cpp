#include "DeviceMonitor.hpp"

#ifdef Q_OS_WINDOWS
#include "WindowsDeviceMonitor.hpp"
#endif

DeviceMonitor::DeviceMonitor(QObject* const parent) : QObject(parent) {
#ifdef Q_OS_WINDOWS
    deviceMonitor->initialize();

    connect(
        deviceMonitor,
        &WindowsDeviceMonitor::defaultDeviceChanged,
        this,
        &DeviceMonitor::defaultDeviceChanged
    );
#endif
}

DeviceMonitor::~DeviceMonitor() {
    delete deviceMonitor;
}