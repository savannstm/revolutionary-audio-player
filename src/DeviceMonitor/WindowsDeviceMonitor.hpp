#pragma once

// clang-format off
#include <QObject>
#include <QString>

#ifdef Q_OS_WINDOWS
#include <propsys.h>

#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
// clang-format on

class WindowsDeviceMonitor : public QObject, public IMMNotificationClient {
    Q_OBJECT

   public:
    explicit WindowsDeviceMonitor(QObject* parent = nullptr);
    ~WindowsDeviceMonitor() override;

    auto initialize() -> bool;
    void shutdown();

    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppvInterface) override;

    STDMETHOD(OnDeviceAdded)(LPCWSTR wDeviceID) override;
    STDMETHOD(OnDeviceRemoved)(LPCWSTR wDeviceID) override;
    STDMETHOD(OnDeviceStateChanged)(LPCWSTR wDeviceID, DWORD newState) override;
    STDMETHOD(OnDefaultDeviceChanged)(
        EDataFlow flow,
        ERole role,
        LPCWSTR wDeviceID
    ) override;
    STDMETHOD(OnPropertyValueChanged)(
        LPCWSTR wDeviceID,
        const PROPERTYKEY key
    ) override;

   signals:
    void deviceAdded(const QString& deviceName, const QString& deviceID);
    void deviceRemoved(const QString& deviceName, const QString& deviceID);
    void deviceStateChanged(
        const QString& deviceName,
        const QString& deviceID,
        DWORD state
    );
    void defaultDeviceChanged(const QString& deviceName);

   private:
    auto getDeviceName(LPCWSTR wDeviceID) -> QString;
    auto isOutputDevice(LPCWSTR wDeviceID) -> bool;

    ULONG refCount = 1;
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
};
#endif