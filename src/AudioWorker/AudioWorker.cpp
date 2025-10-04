#include "AudioWorker.hpp"

#include "AudioStreamer.hpp"

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

    connect(
        audioStreamer,
        &AudioStreamer::samples,
        this,
        &AudioWorker::samples
    );
}

AudioWorker::~AudioWorker() {
    audioSink->stop();
    delete audioSink;

    workerThread->quit();
    workerThread->wait();
    delete workerThread;
}

void AudioWorker::start(const QString& path, const u32 startSecond) {
    if (audioSink != nullptr) {
        audioSink->stop();
        delete audioSink;
    }

    audioStreamer->start(path);
    audioSink = new QAudioSink(device, audioStreamer->format(), this);

    audioSink->setVolume(volumeGain);
    audioSink->start(audioStreamer);

    if (startSecond != UINT32_MAX) {
        audioStreamer->seekSecond(startSecond);
    }
}

void AudioWorker::rebuildAudioSink(const bool playing) {
    if (audioSink == nullptr) {
        return;
    }

    u16 progress;

    if (playing) {
        progress = audioStreamer->progress();
    }

    audioSink->stop();
    delete audioSink;

    audioSink = new QAudioSink(device, audioStreamer->format(), this);
    audioSink->setVolume(volumeGain);

    if (playing) {
        audioSink->start(audioStreamer);
        audioStreamer->seekSecond(progress);
    }
}
