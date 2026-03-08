#pragma once

#include <QObject>

#ifdef Q_OS_LINUX
#include "Aliases.hpp"

#include <pulse/pulseaudio.h>

#include <QThread>

class PulseAudioDeviceMonitor : public QObject {
    Q_OBJECT

   public:
    explicit PulseAudioDeviceMonitor(QObject* parent = nullptr);
    ~PulseAudioDeviceMonitor() override;

   signals:
    void deviceAdded(const QString& deviceName);
    void deviceRemoved(const QString& deviceName);
    void deviceChanged(const QString& deviceName);
    void defaultDeviceChanged(const QString& deviceName);

   private:
    enum class SinkOp : u8 {
        Added,
        Changed
    };

    HashMap<u32, QString> deviceMap;

    pa_mainloop* mainloop = nullptr;
    pa_context* context = nullptr;
    QThread* const mainloopThread = QThread::create([this] -> void {
        i32 ret;
        pa_mainloop_run(mainloop, &ret);
    });

    static void contextStateCallback(pa_context* context, void* userdata);
    static void subscribeCallback(
        pa_context* context,
        pa_subscription_event_type_t type,
        u32 idx,
        void* self_
    );
    void handleSinkInfo(SinkOp operation, const pa_sink_info* info);
};
#endif