#include "AudioWorker.hpp"

#include "AudioStreamer.hpp"
#include "Constants.hpp"
#include "Settings.hpp"

#include <QMessageBox>
#include <QScreen>
#include <cmath>

AudioWorker::AudioWorker(shared_ptr<Settings> settings_) :
    settings(std::move(settings_)),
    streamer(new AudioStreamer(settings)) {
    timer.setInterval(
        u16(f32(SECOND_MS) / f32(qApp->primaryScreen()->refreshRate()))
    );

    connect(&timer, &QTimer::timeout, this, [this] -> void {
        // TODO: This shit probably needs to be refactored
        if (samplesAccumulated.load(std::memory_order_acquire)) {
            if (!flag) {
                const auto fftWindow = span(buffer.data(), requiredSamples);
                emit fftSamples(fftWindow, sampleRate());

                QTimer::singleShot(20, this, [this] -> void {
                    samplesAccumulated.store(false, std::memory_order_release);
                    flag = false;
                });

                flag = true;
            }
        }

        const u32 second = playbackSecond.load(std::memory_order_acquire);

        if (second != lastPlaybackSecond) {
            lastPlaybackSecond = second;
            emit progressUpdated(second);
        }

        if (streamEndedFlag.exchange(false, std::memory_order_acq_rel)) {
            stopPlayback();
            emit streamEnded();
        }
    });

    timer.start();
}

AudioWorker::~AudioWorker() {
    timer.stop();
    stopPlayback();

    if (deviceInitialized) {
        ma_device_uninit(&device);
        deviceInitialized = false;
    }

    delete streamer;
}

void AudioWorker::startPlayback(const QString& path, const i32 startSecond) {
    if (deviceInitialized) {
        stopPlayback();
        ma_device_uninit(&device);
        deviceInitialized = false;
    }

    const auto result = streamer->start(path.toStdString(), startSecond);

    if (!result) {
        QMessageBox::information(
            nullptr,
            tr("Unable to play the track"),
            result.error()
        );
        return;
    }

    startDevice();

    streamerThread =
        new std::jthread([this](const std::stop_token& stopToken) -> void {
        while (!stopToken.stop_requested()) {
            if (!paused.load(std::memory_order_acquire)) {
                const i32 seekSecond =
                    secondToSeek.exchange(-1, std::memory_order_acq_rel);

                if (seekSecond != -1) {
                    streamer->seekSecond(seekSecond);
                }

                if (readScheduled.exchange(false, std::memory_order_acq_rel)) {
                    streamer->readData();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    paused.store(false, std::memory_order_release);
}

void AudioWorker::pause() {
    if (deviceInitialized) {
        paused.store(true, std::memory_order_release);
        ma_device_stop(&device);
    }
}

void AudioWorker::stopPlayback() {
    if (deviceInitialized) {
        ma_device_stop(&device);
    }

    if (streamerThread != nullptr) {
        streamerThread->request_stop();
        delete streamerThread;
        streamerThread = nullptr;

        streamer->reset();
    }

    playbackSecond.store(0, std::memory_order_release);
    secondToSeek.store(-1, std::memory_order_release);
    readScheduled.store(false, std::memory_order_release);
    paused.store(false, std::memory_order_release);
    streamEndedFlag.store(false, std::memory_order_release);
    samplesAccumulated.store(false, std::memory_order_release);
    flag = false;

    currentSamples = 0;
}

void AudioWorker::resume() {
    if (deviceInitialized) {
        paused.store(false, std::memory_order_release);
        ma_device_start(&device);
    }
}

void AudioWorker::seekSecond(const i32 second) {
    secondToSeek.store(second, std::memory_order_release);
}

void AudioWorker::changeAudioDevice() {
    if (!deviceInitialized) {
        return;
    }

    const bool wasRunning = state() == ma_device_state_started;
    ma_device_uninit(&device);
    deviceInitialized = false;

    if (wasRunning) {
        startDevice();
    }
}

void AudioWorker::changeGain() {
    streamer->changeGain();
}

void AudioWorker::setVolume(const f32 volume) {
    volume_ = volume;

    if (deviceInitialized) {
        ma_device_set_master_volume(&device, volume);
    }
}

void AudioWorker::startDevice() {
    deviceConfig = ma_device_config_init(ma_device_type_playback);

    if (settings->core.outputDeviceID) {
        deviceConfig.playback.pDeviceID =
            &settings->core.outputDeviceID.value();
    }

    const u32 sampleRate = streamer->sampleRate();
    const u8 channels = streamer->channels();

    emit audioProperties(sampleRate, channels);

    requiredSamples =
        max<u16>(std::bit_ceil(sampleRate / 100), FFT_SAMPLE_COUNT);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = channels;
    deviceConfig.sampleRate = sampleRate;
    deviceConfig.periodSizeInFrames =
        streamer->copySize() / (F32_SIZE * channels);
    deviceConfig.periods = 2;
    deviceConfig.dataCallback = &AudioWorker::dataCallback;
    deviceConfig.pUserData = this;
    deviceConfig.noClip = 1;
    deviceConfig.noPreSilencedOutputBuffer = 1;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        return;
    }

    deviceInitialized = true;
    ma_device_set_master_volume(&device, volume_);
    ma_device_start(&device);
}

void AudioWorker::dataCallback(
    ma_device* const device,
    void* const output,
    const void* const /* input */,
    const u32 /* sampleCount */
) {
    auto* const self = as<AudioWorker*>(device->pUserData);

    const optional<AudioStreamer::Block> block = self->streamer->consumeBlock();

    if (!block) {
        memset(output, 0, self->streamer->copySize());
        self->streamEndedFlag.store(true, std::memory_order_release);
        return;
    }

    self->readScheduled.store(true, std::memory_order_release);
    memcpy(output, block->firstPart.data(), block->firstPart.size());

    if (!block->secondPart.empty()) {
        memcpy(
            as<u8*>(output) + block->firstPart.size(),
            block->secondPart.data(),
            block->secondPart.size()
        );
    }

    const u16 written = block->firstPart.size() + block->secondPart.size();

    const u16 firstSampleCount = block->firstPart.size() / F32_SIZE;
    const u16 frameCount = written / F32_SIZE;
    const u8 channels = self->streamer->channels();

    if (channels == 0) {
        memset(output, 0, self->streamer->copySize());
        return;
    }

    for (u32 frame = 0; frame + channels <= frameCount; frame += channels) {
        if (self->samplesAccumulated.load(std::memory_order_acquire)) {
            break;
        }

        f32 mixedSample = 0.0F;

        for (const u8 channel : range<u8>(0, channels)) {
            const u32 sampleIdx = frame + channel;

            if (sampleIdx < firstSampleCount) {
                mixedSample +=
                    ras<const f32*>(block->firstPart.data())[sampleIdx];
            } else {
                mixedSample += ras<const f32*>(
                    block->secondPart.data()
                )[sampleIdx - firstSampleCount];
            }
        }

        const f32 mixedFrameSample = mixedSample / f32(channels);

        self->buffer[self->currentSamples++] = mixedFrameSample;

        if (self->currentSamples == self->requiredSamples) {
            self->samplesAccumulated.store(true, std::memory_order_release);
            self->currentSamples = 0;
        }
    }

    if (self->streamer->copySize() > written) {
        memset(
            as<u8*>(output) + written,
            0,
            self->streamer->copySize() - written
        );
    }

    self->playbackSecond.store(block->second, std::memory_order_release);
}

auto AudioWorker::state() const -> ma_device_state {
    return deviceInitialized ? ma_device_get_state(&device)
                             : ma_device_state_uninitialized;
}

auto AudioWorker::channels() const -> u8 {
    return streamer->channels();
}

auto AudioWorker::sampleRate() const -> u32 {
    return streamer->sampleRate();
}
