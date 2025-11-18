#pragma once

#include "AudioStreamer.hpp"
#include "Settings.hpp"

#include <QObject>
#include <miniaudio.h>

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

    void pause() {
        if (deviceInitialized) {
            ma_device_stop(&device);
        }
    }

    void stop() {
        if (deviceInitialized) {
            ma_device_stop(&device);
        }

        audioStreamer->reset();
    }

    void resume() {
        if (deviceInitialized) {
            ma_device_start(&device);
        }
    }

    void seekSecond(const u16 second) {
        seeked = true;

        audioStreamer->seekSecond(second);
        ma_device_start(&device);
    }

    constexpr void setVolume(const f32 volume) {
        volume_ = volume;

        if (deviceInitialized) {
            ma_device_set_master_volume(&device, volume);
        }
    }

    [[nodiscard]] auto state() const -> ma_device_state {
        if (!deviceInitialized) {
            return ma_device_state_uninitialized;
        }

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
        peakVisualizerEnabled = enabled;
    }

    void toggleVisualizer(const bool enabled) { visualizerEnabled = enabled; }

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

    void prepareBuffer() { audioStreamer->prepareBuffer(); }

    void endStream() {
        ma_device_stop(&device);
        emit streamEnded();
    }

    ma_device device;
    ma_device_config deviceConfig;

    shared_ptr<Settings> settings;

    AudioStreamer* audioStreamer = nullptr;
    usize buffersIndex = 0;
    usize bufferOffset = 0;

    f32* peakVisualizerBuffer;
    f32* visualizerBuffer;

    f32 volume_ = 1.0F;

    u16 lastPlaybackSecond = 0;
    u16 seekTargetSecond = 0;

    bool peakVisualizerEnabled = false;
    bool visualizerEnabled = false;

    bool deviceInitialized = false;
    bool seeked = false;
};
