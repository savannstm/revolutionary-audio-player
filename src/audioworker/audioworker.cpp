#include "audioworker.hpp"

#include "audiostreamer.hpp"

#include <QAudio>

AudioWorker::AudioWorker(QObject* parent) : QObject(parent) {
    moveToThread(workerThread);
    workerThread->start();

    connect(
        audioStreamer,
        &AudioStreamer::progressUpdate,
        this,
        &AudioWorker::progressUpdated
    );

    connect(
        audioStreamer,
        &AudioStreamer::streamEnded,
        this,
        &AudioWorker::streamEnded
    );
}

AudioWorker::~AudioWorker() {
    audioSink->stop();
    delete audioSink;

    workerThread->quit();
    workerThread->wait();
    delete workerThread;
}

void AudioWorker::start(const QString& path) {
    if (audioSink != nullptr) {
        audioSink->stop();
        delete audioSink;
    }

    audioStreamer->start(path);
    audioSink = new QAudioSink(audioStreamer->format());

    connect(
        audioSink,
        &QAudioSink::stateChanged,
        this,
        [&](const QAudio::State state) {
        // TODO: No idea if that works
        if (audioSink->error() != QtAudio::NoError) {
            const u16 duration = audioStreamer->duration();
            audioSink = new QAudioSink(audioStreamer->format());
            audioSink->setVolume(volumeGain);
            audioSink->start(audioStreamer);
            audioStreamer->seekSecond(duration);
        }
    }
    );

    audioSink->setVolume(volumeGain);
    audioSink->start(audioStreamer);
    emit duration(audioStreamer->duration());
}
