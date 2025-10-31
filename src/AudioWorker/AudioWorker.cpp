#include "AudioWorker.hpp"

#include "AudioStreamer.hpp"
#include "Constants.hpp"
#include "Settings.hpp"

AudioWorker::AudioWorker(
    shared_ptr<Settings> settings_,
    f32* peakVisualizerBuffer,
    f32* visualizerBuffer,
    QObject* parent
) :
    QObject(parent),
    settings(std::move(settings_)) {
    audioStreamer = new AudioStreamer(settings, this);

    audioStreamer->peakVisualizerBuffer = peakVisualizerBuffer;
    audioStreamer->visualizerBuffer = visualizerBuffer;

    connect(
        audioStreamer,
        &AudioStreamer::progressUpdate,
        this,
        &AudioWorker::progressUpdated
    );

    connect(audioStreamer, &AudioStreamer::streamEnded, this, [this] -> void {
        ma_device_stop(&device);
        emit streamEnded();
    });

    connect(
        audioStreamer,
        &AudioStreamer::processedSamples,
        this,
        &AudioWorker::processedSamples
    );
}

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
    ma_device* device,
    void* output,
    const void* /* input */,
    const u32 sampleCount
) {
    auto* self = as<AudioWorker*>(device->pUserData);

    const PlaybackAction action = self->playbackAction.exchange(
        PlaybackAction::None,
        std::memory_order_acquire
    );

    if (action == PlaybackAction::Seek) {
        self->audioStreamer->seekSecond(self->seekTargetSecond);
    }

    self->audioStreamer->readData(ras<u8*>(output));
}

void AudioWorker::startDevice() {
    deviceConfig = ma_device_config_init(ma_device_type_playback);

    if (settings->outputDeviceID) {
        deviceConfig.playback.pDeviceID = &settings->outputDeviceID.value();
    }

    const u32 sampleRate = audioStreamer->sampleRate();
    const AudioChannels channels = audioStreamer->channels();

    emit audioProperties(sampleRate, channels);

    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = channels;
    deviceConfig.sampleRate = sampleRate;
    deviceConfig.periodSizeInFrames =
        (channels == AudioChannels::Surround51 ? MIN_BUFFER_SIZE_3BYTES
                                               : MIN_BUFFER_SIZE) /
        (F32_SAMPLE_SIZE * channels);
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

    deviceInitialized = true;
    ma_device_set_master_volume(&device, volume_);
    ma_device_start(&device);
}