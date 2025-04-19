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
    workerThread.quit();
    workerThread.wait();
}

void AudioWorker::start(const string& path) {
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
    audioSink->stop();
}

void AudioWorker::suspend() {
    audioSink->suspend();
}

void AudioWorker::resume() {
    audioSink->resume();
}

void AudioWorker::seekSecond(const u16 second) {
    audioStreamer.seekSecond(second);
}

void AudioWorker::setVolume(const f64 gain) {
    volumeGain = gain;
    audioSink->setVolume(gain);
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
    return audioSink->state() == QAudio::ActiveState;
}
