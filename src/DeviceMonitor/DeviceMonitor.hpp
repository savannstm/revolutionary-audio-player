// Useful class, but since we do everything with libraries, it's not currently
// used
// May come in handy someday

#pragma once

#include <QObject>

#ifdef Q_OS_WINDOWS
#include "WindowsDeviceMonitor.hpp"
#elifdef Q_OS_LINUX
#include "PulseAudioDeviceMonitor.hpp"
#endif

class DeviceMonitor : public QObject {
    Q_OBJECT

   public:
    explicit DeviceMonitor(QObject* parent);
    ~DeviceMonitor() override;

   signals:
    void defaultDeviceChanged(const QString& deviceName);
    void deviceAdded(const QString& deviceName, const QString& deviceID);
    void deviceRemoved(const QString& deviceName, const QString& deviceID);
    void deviceStateChanged(const QString& deviceName, const QString& deviceID);

   private:
#ifdef Q_OS_WINDOWS
    WindowsDeviceMonitor* deviceMonitor = new WindowsDeviceMonitor(this);
#elifdef Q_OS_LINUX
    PulseAudioDeviceMonitor* deviceMonitor = new PulseAudioDeviceMonitor(this);
#endif
};