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

    audioSink = new QAudioSink(audioStreamer.format());
    audioSink->setVolume(volumeGain);
    audioSink->start(&audioStreamer);

    emit duration(audioStreamer.duration());
}
