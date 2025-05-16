#include "audioworker.hpp"

#include "audiostreamer.hpp"

#include <QtAudio>

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
    audioSink = new QAudioSink(device, audioStreamer->format(), this);

    audioSink->setVolume(volumeGain);
    audioSink->start(audioStreamer);
    emit duration(audioStreamer->duration());
}

void AudioWorker::rebuildAudioSink(const QtAudio::State state) {
    if (audioSink == nullptr) {
        return;
    }

    const bool activeState =
        state != QtAudio::IdleState && state != QtAudio::StoppedState;
    u16 progress;

    if (activeState) {
        progress = audioStreamer->progress();
    }

    audioSink->stop();
    delete audioSink;

    audioSink = new QAudioSink(device, audioStreamer->format(), this);
    audioSink->setVolume(volumeGain);

    if (activeState) {
        audioSink->start(audioStreamer);
        audioStreamer->seekSecond(progress);
    }
}
