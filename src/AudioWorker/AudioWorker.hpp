#pragma once

#include "Aliases.hpp"
#include "AudioStreamer.hpp"
#include "FWD.hpp"

#include <QTimer>
#include <miniaudio.h>
#include <thread>

class AudioWorker : public QObject {
    Q_OBJECT

   public:
    explicit AudioWorker(shared_ptr<Settings>);
    ~AudioWorker() override;

    void startPlayback(const QString& path, i32 startSecond = -1);
    void pause();
    void stopPlayback();
    void resume();
    void seekSecond(i32 second);
    void setVolume(f32 volume);
    void changeAudioDevice();
    void changeGain();

    [[nodiscard]] auto state() const -> ma_device_state;
    [[nodiscard]] auto channels() const -> u8;
    [[nodiscard]] auto sampleRate() const -> u32;

   signals:
    void progressUpdated(u32 second);
    void streamEnded();
    void audioProperties(u32 sampleRate, u8 channels);
    void fftSamples(span<const f32> samples, u32 sampleRate);

   private:
    static void dataCallback(
        ma_device* device,
        void* output,
        const void* input,
        u32 sampleCount
    );

    void startDevice();

    // Maximum possible sample rate in most cases is 768000. Since we required
    // (sample_rate / 100) samples, this produces 7680. Since we want a power of
    // two, this produces 8192.
    array<f32, 8192> buffer;

    ma_device device;
    ma_device_config deviceConfig;

    QTimer timer;

    shared_ptr<Settings> settings;
    AudioStreamer* streamer;
    std::jthread* streamerThread = nullptr;

    u16 currentSamples = 0;
    u16 requiredSamples;

    f32 volume_ = 1.0F;
    u32 lastPlaybackSecond = 0;

    atomicI32 secondToSeek{ -1 };
    atomicU32 playbackSecond{ 0 };
    atomicBool streamEndedFlag{ false };
    atomicBool paused{ false };
    atomicBool readScheduled{ false };
    atomicBool samplesAccumulated{ false };

    bool deviceInitialized = false;
    bool flag = false;
};
