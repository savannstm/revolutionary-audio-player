#include "AudioWorker.hpp"

#include "AudioStreamer.hpp"
#include "Constants.hpp"
#include "Settings.hpp"

AudioWorker::AudioWorker(
    shared_ptr<Settings> settings_,
    f32* const spectrumVisualizerBuffer,
    f32* const visualizerBuffer,
    QObject* const parent
) :
    QObject(parent),
    spectrumVisualizerBuffer(spectrumVisualizerBuffer),
    visualizerBuffer(visualizerBuffer),
    settings(std::move(settings_)),
    audioStreamer(new AudioStreamer(settings, this)) {}

AudioWorker::~AudioWorker() {
    ma_device_uninit(&device);
};

void AudioWorker::start(const QString& path, const u16 startSecond) {
    if (deviceInitialized) {
        ma_device_uninit(&device);
        deviceInitialized = false;
    }

    audioStreamer->start(path, startSecond);
    startDevice();
}

void AudioWorker::dataCallback(
    ma_device* const device,
    void* const output,
    const void* const /* input */,
    const u32 sampleCount
) {
    auto* const self = as<AudioWorker*>(device->pUserData);

    if (self->seeked) {
        self->buffersIndex = 0;
        self->audioStreamer->flushBuffers();

        for (const u8 idx : range(0, BUFFERS_QUEUE_SIZE)) {
            QMetaObject::invokeMethod(
                self,
                &AudioWorker::prepareBuffer,
                Qt::QueuedConnection
            );
        }

        self->seeked = false;
        return;
    }

    AudioStreamer* const streamer = self->audioStreamer;
    auto* buf = &streamer->buffers[self->buffersIndex];

    if (buf->buf.empty()) {
        QMetaObject::invokeMethod(
            self,
            &AudioWorker::endStream,
            Qt::QueuedConnection
        );
        return;
    }

    const u8* ptr = buf->buf.data();
    usize size = buf->buf.size();

    if (self->bufferOffset >= size) {
        // Buffer drained
        QMetaObject::invokeMethod(
            self,
            &AudioWorker::prepareBuffer,
            Qt::QueuedConnection
        );

        self->bufferOffset = 0;
        self->buffersIndex++;

        if (self->buffersIndex == BUFFERS_QUEUE_SIZE) {
            self->buffersIndex = 0;
        }

        buf = &streamer->buffers[self->buffersIndex];

        if (buf->buf.empty()) {
            QMetaObject::invokeMethod(
                self,
                &AudioWorker::endStream,
                Qt::QueuedConnection
            );
            return;
        }

        ptr = buf->buf.data();
        size = buf->buf.size();
    }

    const u64 bytesToCopy =
        min<u64>(streamer->minBufferSize, size - self->bufferOffset);
    const u8* src = ptr + self->bufferOffset;

    // First we copy full-volume samples into the visualizers
    if (self->spectrumVisualizerEnabled) {
        memcpy(self->spectrumVisualizerBuffer, src, bytesToCopy);
    }

    if (self->visualizerEnabled) {
        memcpy(self->visualizerBuffer, src, bytesToCopy);
    }

    // Then we feed this to the output
    memcpy(output, src, bytesToCopy);

    // Clean up dirt
    if (self->audioStreamer->minBufferSize > bytesToCopy) {
        if (self->spectrumVisualizerEnabled) {
            memset(
                ras<u8*>(self->spectrumVisualizerBuffer) + bytesToCopy,
                0,
                streamer->minBufferSize - bytesToCopy
            );
        }

        if (self->visualizerEnabled) {
            memset(
                ras<u8*>(self->visualizerBuffer) + bytesToCopy,
                0,
                streamer->minBufferSize - bytesToCopy
            );
        }

        memset(
            as<u8*>(output) + bytesToCopy,
            0,
            streamer->minBufferSize - bytesToCopy
        );
    }

    self->bufferOffset += bytesToCopy;
    emit self->processedSamples();

    if (buf->second != self->lastPlaybackSecond) {
        emit self->progressUpdated(buf->second);
        self->lastPlaybackSecond = buf->second;
    }
}

void AudioWorker::startDevice() {
    deviceConfig = ma_device_config_init(ma_device_type_playback);

    if (settings->core.outputDeviceID) {
        deviceConfig.playback.pDeviceID =
            &settings->core.outputDeviceID.value();
    }

    const u32 sampleRate = audioStreamer->sampleRate();
    const AudioChannels channels = audioStreamer->channels();

    emit audioProperties(sampleRate, channels);

    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = u8(channels);
    deviceConfig.sampleRate = sampleRate;
    deviceConfig.periodSizeInFrames =
        (channels == AudioChannels::Surround51 ? MIN_BUFFER_SIZE_3BYTES
                                               : MIN_BUFFER_SIZE) /
        (F32_SAMPLE_SIZE * u8(channels));
    deviceConfig.periods = 2;
    deviceConfig.dataCallback = &AudioWorker::dataCallback;
    deviceConfig.pUserData = this;
    deviceConfig.noClip = 1;
    deviceConfig.noPreSilencedOutputBuffer = 1;

    const ma_result result = ma_device_init(nullptr, &deviceConfig, &device);

    if (result != MA_SUCCESS) {
        LOG_ERROR(ma_result_description(result));
        return;
    }

    buffersIndex = 0;

    deviceInitialized = true;
    ma_device_set_master_volume(&device, volume_);
    ma_device_start(&device);
}

void AudioWorker::pause() {
    if (deviceInitialized) {
        ma_device_stop(&device);
    }
}

void AudioWorker::stop() {
    if (deviceInitialized) {
        ma_device_stop(&device);
    }

    audioStreamer->reset();
}

void AudioWorker::resume() {
    if (deviceInitialized) {
        ma_device_start(&device);
    }
}

void AudioWorker::seekSecond(const u16 second) {
    seeked = true;

    audioStreamer->seekSecond(second);
    ma_device_start(&device);
}

void AudioWorker::setVolume(const f32 volume) {
    volume_ = volume;

    if (deviceInitialized) {
        ma_device_set_master_volume(&device, volume);
    }
}

void AudioWorker::changeAudioDevice() {
    if (deviceInitialized) {
        const bool started = state() == ma_device_state_started;

        ma_device_uninit(&device);

        if (started) {
            startDevice();
        }
    }
}

[[nodiscard]] auto AudioWorker::state() const -> ma_device_state {
    if (!deviceInitialized) {
        return ma_device_state_uninitialized;
    }

    return ma_device_get_state(&device);
}

void AudioWorker::endStream() {
    ma_device_stop(&device);
    emit streamEnded();
}

void AudioWorker::changeGain(const u8 band) {
    audioStreamer->changeGain(band);
}

[[nodiscard]] auto AudioWorker::channels() -> AudioChannels {
    return audioStreamer->channels();
}

void AudioWorker::prepareBuffer() {
    audioStreamer->prepareBuffer();
}
