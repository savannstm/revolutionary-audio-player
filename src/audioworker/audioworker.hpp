#pragma once

#include "audiostreamer.hpp"

#include <QAudioSink>
#include <QObject>
#include <QThread>

class AudioWorker : public QObject {
    Q_OBJECT

   public:
    explicit AudioWorker(QObject* parent = nullptr);
    ~AudioWorker() override;

    void start(const QString& path);
    void stop();
    void suspend();
    void resume();
    void seekSecond(u16 second);

    void setVolume(f64 gain);
    void setGain(i8 gain, u8 band);
    auto getGain(u8 band) -> i8;
    auto gains() -> const db_gains_array&;

    [[nodiscard]] auto isEqEnabled() const -> bool;
    void setEqEnabled(bool enabled);

    [[nodiscard]] auto playing() const -> bool;

    void setBands(u8 count);
    auto bands() -> const frequencies_array&;

   signals:
    void finished();
    void duration(u16 seconds);
    void progressUpdated(u16 second);
    void endOfFile();
    void volumeChanged(f64 gain);

   private:
    QThread workerThread;
    AudioStreamer audioStreamer;
    QAudioSink* audioSink = nullptr;
    f64 volumeGain = 1.0;
};
