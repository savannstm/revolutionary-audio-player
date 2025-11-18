// TODO
#include "PulseAudioDeviceMonitor.hpp"

#ifdef Q_OS_LINUX
PulseAudioDeviceMonitor::PulseAudioDeviceMonitor(QObject* const parent) :
    QObject(parent) {
    mainloop = pa_mainloop_new();
    pa_mainloop_api* const mainloop_api = pa_mainloop_get_api(mainloop);
    context = pa_context_new(mainloop_api, "Device Monitor");

    pa_context_set_state_callback(
        context,
        &PulseAudioDeviceMonitor::contextStateCallback,
        this
    );
    pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    mainloopThread->start();
}

PulseAudioDeviceMonitor::~PulseAudioDeviceMonitor() {
    if (context) {
        pa_context_disconnect(context);
        pa_context_unref(context);
    }

    if (mainloop) {
        pa_mainloop_quit(mainloop, 0);
        pa_mainloop_free(mainloop);
    }
}

void PulseAudioDeviceMonitor::contextStateCallback(
    pa_context* context,
    void* userdata
) {
    auto* self = as<PulseAudioDeviceMonitor*>(userdata);

    switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY:
            qDebug() << "PulseAudio context ready.";

            // Subscribe to sink (output) events
            pa_context_set_subscribe_callback(
                context,
                &PulseAudioDeviceMonitor::subscribeCallback,
                self
            );

            pa_context_subscribe(
                context,
                pa_subscription_mask_t(
                    PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER
                ),
                nullptr,
                nullptr
            );
            break;

        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            qDebug() << "PulseAudio context lost or terminated.";
            break;

        default:
            break;
    }
}

void PulseAudioDeviceMonitor::subscribeCallback(
    pa_context* context,
    pa_subscription_event_type_t type,
    u32 idx,
    void* userdata
) {
    auto* self = as<PulseAudioDeviceMonitor*>(userdata);
    auto facility = type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    auto operation = type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
        switch (operation) {
            case PA_SUBSCRIPTION_EVENT_NEW:
                pa_context_get_sink_info_by_index(
                    context,
                    idx,
                    &PulseAudioDeviceMonitor::sinkInfoCallback,
                    self
                );
                break;
            case PA_SUBSCRIPTION_EVENT_REMOVE:
                emit self->deviceRemoved(QString("sink #%1").arg(idx));
                break;
            case PA_SUBSCRIPTION_EVENT_CHANGE:
                pa_context_get_sink_info_by_index(
                    context,
                    idx,
                    &PulseAudioDeviceMonitor::sinkInfoCallback,
                    self
                );
                break;
        }
    } else if (facility == PA_SUBSCRIPTION_EVENT_SERVER) {
        // Default sink might have changed
        pa_context_get_server_info(
            context,
            [](pa_context* /* context */,
               const pa_server_info* info,
               void* userdata) {
            auto* self = as<PulseAudioDeviceMonitor*>(userdata);

            emit self->defaultDeviceChanged(
                QString::fromUtf8(info->default_sink_name)
            );
        },
            self
        );
    }
}

void PulseAudioDeviceMonitor::sinkInfoCallback(
    pa_context* context,
    const pa_sink_info* info,
    i32 eol,
    void* userdata
) {
    if (eol > 0 || !info) {
        return;
    }

    auto* self = as<PulseAudioDeviceMonitor*>(userdata);

    // TODO: Distinguish add and change
    emit self->deviceChanged(QString::fromUtf8(info->name));
}
#endif