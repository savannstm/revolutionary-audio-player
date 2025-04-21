#include "audioworker.hpp"

#include "audiostreamer.hpp"

#include <QAudio>

AudioWorker::AudioWorker(QObject* parent) : QObject(parent) {
    moveToThread(&workerThread);
    workerThread.start();

    connect(
        &audioStreamer,
        &AudioStreamer::progressUpdate,
        this,
        [&](const u16 second) { emit progressUpdated(second); }
    );

    connect(&audioStreamer, &AudioStreamer::endOfFile, this, [&] {
        emit endOfFile();
    });
}

AudioWorker::~AudioWorker() {
    audioSink->stop();
    delete audioSink;

    workerThread.quit();
    workerThread.wait();
}

void AudioWorker::start(const QString& path) {
    if (audioSink != nullptr) {
        audioSink->stop();
        delete audioSink;
    }

    audioStreamer.start(path);

    audioSink = new QAudioSink(audioStreamer.getFormat());
    audioSink->setVolume(volumeGain);
    audioSink->start(&audioStreamer);

    emit duration(audioStreamer.duration());
}

void AudioWorker::stop() {
    if (audioSink != nullptr) {
        audioSink->stop();
    }
}

void AudioWorker::suspend() {
    if (audioSink != nullptr) {
        audioSink->suspend();
    }
}

void AudioWorker::resume() {
    if (audioSink != nullptr) {
        audioSink->resume();
    }
}

void AudioWorker::seekSecond(const u16 second) {
    audioStreamer.seekSecond(second);
}

void AudioWorker::setVolume(const f64 gain) {
    volumeGain = gain;

    if (audioSink != nullptr) {
        audioSink->setVolume(gain);
    }
}

void AudioWorker::setGain(const i8 dbGain, const u8 band) {
    audioStreamer.setGain(dbGain, band);
}

auto AudioWorker::isEqEnabled() const -> bool {
    return audioStreamer.isEqEnabled();
}

void AudioWorker::setEqEnabled(const bool enabled) {
    audioStreamer.setEqEnabled(enabled);
}

auto AudioWorker::getGain(const u8 band) -> i8 {
    return audioStreamer.getGain(band);
}

auto AudioWorker::playing() const -> bool {
    if (audioSink != nullptr) {
        return audioSink->state() == QAudio::ActiveState;
    }

    return false;
}

void AudioWorker::setBands(const u8 count) {
    audioStreamer.setBands(count);
};

auto AudioWorker::bands() -> const frequencies_array& {
    return audioStreamer.bands();
};

auto AudioWorker::gains() -> const db_gains_array& {
    return audioStreamer.gains();
}