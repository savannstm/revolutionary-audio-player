#include "WindowsDeviceMonitor.hpp"

#ifdef Q_OS_WINDOWS
#include <QDebug>
#include <comdef.h>

WindowsDeviceMonitor::WindowsDeviceMonitor(QObject* parent) : QObject(parent) {}

WindowsDeviceMonitor::~WindowsDeviceMonitor() {
    shutdown();
}

auto WindowsDeviceMonitor::initialize() -> bool {
    CoInitialize(nullptr);

    HRESULT result = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator
    );

    if (SUCCEEDED(result)) {
        result = deviceEnumerator->RegisterEndpointNotificationCallback(this);

        if (FAILED(result)) {
            deviceEnumerator->Release();
            deviceEnumerator = nullptr;
        }
    }

    return SUCCEEDED(result);
}

void WindowsDeviceMonitor::shutdown() {
    if (deviceEnumerator != nullptr) {
        deviceEnumerator->UnregisterEndpointNotificationCallback(this);
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }
}

auto WindowsDeviceMonitor::AddRef() -> ULONG {
    return InterlockedIncrement(&refCount);
}

auto WindowsDeviceMonitor::Release() -> ULONG {
    const ULONG ulRef = InterlockedDecrement(&refCount);

    if (ulRef == 0) {
        delete this;
    }

    return ulRef;
}

auto WindowsDeviceMonitor::QueryInterface(REFIID riid, VOID** ppvInterface)
    -> HRESULT {
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
        *ppvInterface = (IUnknown*)this;
        AddRef();
        return S_OK;
    }

    *ppvInterface = nullptr;
    return E_NOINTERFACE;
}

auto WindowsDeviceMonitor::OnDeviceAdded(LPCWSTR wDeviceID) -> HRESULT {
    if (isOutputDevice(wDeviceID)) {
        const QString name = getDeviceName(wDeviceID);
        const QString deviceID = QString::fromWCharArray(wDeviceID);
        emit deviceAdded(name, deviceID);
    }

    return S_OK;
}

auto WindowsDeviceMonitor::OnDeviceRemoved(const LPCWSTR wDeviceID) -> HRESULT {
    if (isOutputDevice(wDeviceID)) {
        const QString name = getDeviceName(wDeviceID);
        const QString deviceID = QString::fromWCharArray(wDeviceID);
        emit deviceRemoved(name, deviceID);
    }

    return S_OK;
}

auto WindowsDeviceMonitor::OnDeviceStateChanged(
    const LPCWSTR wDeviceID,
    const DWORD newState
) -> HRESULT {
    if (isOutputDevice(wDeviceID)) {
        const QString name = getDeviceName(wDeviceID);
        const QString deviceID = QString::fromWCharArray(wDeviceID);
        emit deviceStateChanged(name, deviceID, newState);
    }

    return S_OK;
}

auto WindowsDeviceMonitor::OnDefaultDeviceChanged(
    const EDataFlow flow,
    const ERole role,
    const LPCWSTR wDeviceID
) -> HRESULT {
    if (role == eMultimedia && isOutputDevice(wDeviceID)) {
        const QString name = getDeviceName(wDeviceID);
        const QString deviceID = QString::fromWCharArray(wDeviceID);
        emit defaultDeviceChanged(name);
    }

    return S_OK;
}

auto WindowsDeviceMonitor::OnPropertyValueChanged(
    const LPCWSTR wDeviceID,
    const PROPERTYKEY key
) -> HRESULT {
    return S_OK;
}

auto WindowsDeviceMonitor::getDeviceName(const LPCWSTR wDeviceID) -> QString {
    QString deviceName;

    if (deviceEnumerator == nullptr) {
        return deviceName;
    }

    IMMDevice* device = nullptr;

    HRESULT result = deviceEnumerator->GetDevice(wDeviceID, &device);

    if (SUCCEEDED(result)) {
        IPropertyStore* propStore = nullptr;
        result = device->OpenPropertyStore(STGM_READ, &propStore);

        if (SUCCEEDED(result)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);
            result = propStore->GetValue(PKEY_Device_FriendlyName, &varName);

            if (SUCCEEDED(result) && varName.vt == VT_LPWSTR) {
                deviceName = QString::fromWCharArray(varName.pwszVal);
            }

            PropVariantClear(&varName);
            propStore->Release();
        }

        device->Release();
    }

    return deviceName;
}

auto WindowsDeviceMonitor::isOutputDevice(const LPCWSTR wDeviceID) -> bool {
    if (deviceEnumerator == nullptr) {
        return false;
    }

    IMMDevice* device = nullptr;
    HRESULT result = deviceEnumerator->GetDevice(wDeviceID, &device);

    if (SUCCEEDED(result)) {
        EDataFlow dataFlow;

        IMMEndpoint* endpoint = nullptr;
        device->QueryInterface(__uuidof(IMMEndpoint), (void**)&endpoint);

        result = endpoint->GetDataFlow(&dataFlow);

        endpoint->Release();
        device->Release();

        return SUCCEEDED(result) && (dataFlow == eRender);
    }

    return false;
}
#endif