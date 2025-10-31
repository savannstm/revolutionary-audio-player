#include "DeviceMonitor.hpp"

#ifdef Q_OS_WINDOWS
#include "WindowsDeviceMonitor.hpp"
#endif

DeviceMonitor::DeviceMonitor(QObject* parent) : QObject(parent) {
    deviceMonitor->initialize();

#ifdef Q_OS_WINDOWS
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