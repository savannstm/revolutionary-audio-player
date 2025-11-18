#include "PulseAudioDeviceMonitor.hpp"

#include "Logger.hpp"

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
    if (context != nullptr) {
        pa_context_disconnect(context);
        pa_context_unref(context);
    }

    if (mainloop != nullptr) {
        pa_mainloop_quit(mainloop, 0);
        pa_mainloop_free(mainloop);
    }
}

void PulseAudioDeviceMonitor::contextStateCallback(
    pa_context* const context,
    void* const userdata
) {
    auto* const self = as<PulseAudioDeviceMonitor*>(userdata);

    switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY:
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

            pa_context_get_sink_info_list(
                context,
                [](pa_context* const /* context */,
                   const pa_sink_info* const info,
                   const i32 eol,
                   void* const userdata) -> void {
                if (eol > 0 || !info) {
                    return;
                }

                auto* const self = as<PulseAudioDeviceMonitor*>(userdata);
                self->handleSinkInfo(SinkOp::Added, info);
            },
                self
            );
            break;

        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            LOG_ERROR(u"PulseAudio context failed."_s);
            break;

        default:
            break;
    }
}

void PulseAudioDeviceMonitor::subscribeCallback(
    pa_context* const context,
    const pa_subscription_event_type_t type,
    const u32 idx,
    void* const self_
) {
    auto* const self = as<PulseAudioDeviceMonitor*>(self_);
    const i32 facility = type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    const i32 operation = type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
        switch (operation) {
            case PA_SUBSCRIPTION_EVENT_NEW:
                pa_context_get_sink_info_by_index(
                    context,
                    idx,
                    [](pa_context* const context,
                       const pa_sink_info* const info,
                       const i32 eol,
                       void* const self_) -> void {
                    if (!info || eol > 0) {
                        return;
                    }

                    auto* const self = as<PulseAudioDeviceMonitor*>(self_);
                    self->handleSinkInfo(SinkOp::Added, info);
                },
                    self
                );
                break;

            case PA_SUBSCRIPTION_EVENT_REMOVE:
                emit self->deviceRemoved(self->deviceMap[idx]);
                self->deviceMap.erase(idx);
                break;

            case PA_SUBSCRIPTION_EVENT_CHANGE:
                pa_context_get_sink_info_by_index(
                    context,
                    idx,
                    [](pa_context* const context,
                       const pa_sink_info* const info,
                       const i32 eol,
                       void* const monitor) -> void {
                    if (!info || eol > 0) {
                        return;
                    }

                    auto* const self = as<PulseAudioDeviceMonitor*>(monitor);
                    self->handleSinkInfo(SinkOp::Changed, info);
                },
                    self
                );
                break;

            default:
                break;
        }
    } else if (facility == PA_SUBSCRIPTION_EVENT_SERVER) {
        pa_context_get_server_info(
            context,
            [](pa_context* const /* context */,
               const pa_server_info* const info,
               void* const self_) -> void {
            auto* const self = as<PulseAudioDeviceMonitor*>(self_);

            QString defaultName;
            if (info->default_sink_name) {
                defaultName = QString::fromUtf8(info->default_sink_name);
            }

            pa_context_get_sink_info_by_name(
                self->context,
                info->default_sink_name,
                [](pa_context* const /* context */,
                   const pa_sink_info* const info,
                   const i32 eol,
                   void* const monitor) -> void {
                if (!info || eol > 0) {
                    return;
                }

                auto* const self = as<PulseAudioDeviceMonitor*>(monitor);

                const QString readable = QString::fromUtf8(info->description);
                emit self->defaultDeviceChanged(readable);
            },
                self
            );
        },
            self
        );
    }
}

void PulseAudioDeviceMonitor::handleSinkInfo(
    const SinkOp operation,
    const pa_sink_info* const info
) {
    const QString systemName = QString::fromUtf8(info->name);
    const QString name = QString::fromUtf8(info->description);

    switch (operation) {
        case SinkOp::Added:
            deviceMap.emplace(info->index, name);
            emit deviceAdded(name);
            break;

        case SinkOp::Changed:
            emit deviceChanged(name);
            break;
    }
}

#endif