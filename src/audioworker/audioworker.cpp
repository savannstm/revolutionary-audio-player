#include "audioworker.hpp"

#include "audiostreamer.hpp"

#include <QAudio>
#include <qaudio.h>

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

    // TODO: Listen for audio device failures, try to reconnect if any
    connect(
        audioSink,
        &QAudioSink::stateChanged,
        this,
        [&](const QAudio::State state) {
        if (state == QAudio::StoppedState) {}
    }
    );

    audioSink->setVolume(volumeGain);
    audioSink->start(audioStreamer);

    emit duration(audioStreamer->duration());
}
