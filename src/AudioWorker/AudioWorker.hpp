#pragma once

#include "AudioStreamer.hpp"
#include "Settings.hpp"

#include <QObject>
#include <miniaudio.h>

enum PlaybackAction : u8 {
    None,
    Seek,
};

class AudioWorker : public QObject {
    Q_OBJECT

   public:
    explicit AudioWorker(
        shared_ptr<Settings>,
        f32* peakVisualizerBuffer,
        f32* visualizerBufferData,
        QObject* parent = nullptr
    );

    ~AudioWorker() override;

    void start(const QString& path, u16 startSecond = UINT16_MAX);

    void stop() {
        if (deviceInitialized) {
            ma_device_stop(&device);
        }
    }

    void resume() {
        if (deviceInitialized) {
            ma_device_start(&device);
        }
    }

    void seekSecond(const u16 second) {
        seekTargetSecond = second;
        playbackAction.store(PlaybackAction::Seek, std::memory_order_release);
    }

    constexpr void setVolume(const f32 volume) {
        volume_ = volume;

        if (deviceInitialized) {
            ma_device_set_master_volume(&device, volume);
        }
    }

    [[nodiscard]] auto state() const -> ma_device_state {
        return ma_device_get_state(&device);
    }

    void changeAudioDevice() {
        if (deviceInitialized) {
            const bool started = state() == ma_device_state_started;

            ma_device_uninit(&device);

            if (started) {
                startDevice();
            }
        }
    }

    void changeGain(const u8 band) { audioStreamer->changeGain(band); }

    void togglePeakVisualizer(const bool enabled) {
        audioStreamer->togglePeakVisualizer(enabled);
    }

    void toggleVisualizer(const bool enabled) {
        audioStreamer->toggleVisualizer(enabled);
    }

    [[nodiscard]] auto channels() -> AudioChannels {
        return audioStreamer->channels();
    }

   signals:
    void progressUpdated(u16 second);
    void processedSamples();
    void streamEnded();
    void audioProperties(u32 sampleRate, AudioChannels channels);

   private:
    static inline void dataCallback(
        ma_device* device,
        void* output,
        const void* /* input */,
        u32 sampleCount
    );

    void startDevice();

    shared_ptr<Settings> settings;
    AudioStreamer* audioStreamer = nullptr;

    ma_device device;
    ma_device_config deviceConfig;
    bool deviceInitialized = false;
    f32 volume_ = 1.0F;

    atomic<PlaybackAction> playbackAction = PlaybackAction::None;
    u16 seekTargetSecond = 0;
};
