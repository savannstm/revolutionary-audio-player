#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FWD.hpp"

#include <QObject>
#include <miniaudio.h>

class AudioWorker : public QObject {
    Q_OBJECT

   public:
    explicit AudioWorker(
        shared_ptr<Settings>,
        f32* spectrumVisualizerBuffer,
        f32* visualizerBufferData,
        QObject* parent = nullptr
    );

    ~AudioWorker() override;

    void start(const QString& path, u16 startSecond = UINT16_MAX);
    void pause();
    void stop();
    void resume();

    void seekSecond(u16 second);
    void setVolume(f32 volume);

    [[nodiscard]] auto state() const -> ma_device_state;

    void changeAudioDevice();

    void changeGain(u8 band);

    void toggleSpectrumVisualizer(const bool enabled) {
        spectrumVisualizerEnabled = enabled;
    }

    void toggleVisualizer(const bool enabled) { visualizerEnabled = enabled; }

    [[nodiscard]] auto channels() -> AudioChannels;

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

    inline void startDevice();
    inline void prepareBuffer();
    inline void endStream();

    ma_device device;
    ma_device_config deviceConfig;

    shared_ptr<Settings> settings;

    AudioStreamer* audioStreamer = nullptr;
    usize buffersIndex = 0;
    usize bufferOffset = 0;

    f32* spectrumVisualizerBuffer;
    f32* visualizerBuffer;

    f32 volume_ = 1.0F;

    u16 lastPlaybackSecond = 0;
    u16 seekTargetSecond = 0;

    bool spectrumVisualizerEnabled = false;
    bool visualizerEnabled = false;

    bool deviceInitialized = false;
    bool seeked = false;
};
