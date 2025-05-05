#pragma once

#include "audiostreamer.hpp"
#include "enums.hpp"

#include <QAudioSink>
#include <QObject>
#include <QThread>

class AudioWorker : public QObject {
    Q_OBJECT

   public:
    explicit AudioWorker(QObject* parent = nullptr);

    ~AudioWorker() override;

    void start(const QString& path);

    void stop() {
        if (audioSink != nullptr) {
            audioSink->stop();
        }
    };

    void suspend() {
        if (audioSink != nullptr) {
            audioSink->suspend();
        }
    };

    void resume() {
        if (audioSink != nullptr) {
            audioSink->resume();
        }
    };

    void seekSecond(const u16 second) { audioStreamer->seekSecond(second); };

    void setVolume(const f64 gain) {
        volumeGain = gain;

        if (audioSink != nullptr) {
            audioSink->setVolume(gain);
        }
    };

    constexpr void setGain(const i8 dbGain, const u8 band) {
        audioStreamer->setGain(dbGain, band);
    };

    [[nodiscard]] constexpr auto gain(const u8 band) const -> i8 {
        return audioStreamer->gain(band);
    };

    [[nodiscard]] constexpr auto gains() const
        -> const array<i8, THIRTY_BANDS>& {
        return audioStreamer->gains();
    };

    [[nodiscard]] constexpr auto equalizerEnabled() const -> bool {
        return audioStreamer->equalizerEnabled();
    };

    constexpr void toggleEqualizer(const bool enabled) {
        audioStreamer->toggleEqualizer(enabled);
    };

    [[nodiscard]] auto state() const -> QAudio::State {
        if (audioSink != nullptr) {
            return audioSink->state();
        }

        return QAudio::IdleState;
    };

    void setBandCount(const u8 count) { audioStreamer->setBandCount(count); };

    [[nodiscard]] constexpr auto bands() const
        -> const array<f32, THIRTY_BANDS>& {
        return audioStreamer->bands();
    };

   signals:
    void finished();
    void duration(u16 seconds);
    void progressUpdated(u16 second);
    void streamEnded();
    void volumeChanged(f64 gain);

   private:
    QThread* workerThread = new QThread();
    AudioStreamer* audioStreamer = new AudioStreamer(this);
    QAudioSink* audioSink = nullptr;
    f64 volumeGain = 1.0;
};
