#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"
#include "Settings.hpp"

#include <QScreen>
#include <QTimer>
#include <QWidget>

// TODO: Make different presets (18, 30 peaks)
class PeakVisualizer : public QWidget {
    Q_OBJECT

   public:
    explicit PeakVisualizer(const f32* bufferData, QWidget* parent = nullptr);

    constexpr void setMode(const PeakVisualizerMode mode_) { mode = mode_; }

    constexpr void setGradient(const QGradient& gradient_) {
        gradient = gradient_;
    }

    void reset();

    void loadSettings(const PeakVisualizerSettings& settings) {
        setHidden(settings.hidden);
        gradientPreset = settings.preset;
        gradient = QGradient(gradientPreset);
        mode = settings.mode;
    };

    void saveSettings(PeakVisualizerSettings& settings) {
        settings.hidden = isHidden();
        settings.preset = gradientPreset;
        settings.mode = mode;
    }

    void setAudioProperties(const u32 rate, const AudioChannels chn) {
        sampleRate = rate;
        channels = chn;

        if (channels == AudioChannels::Surround51) {
            bufferSize = MIN_BUFFER_SIZE_3BYTES;
        } else {
            bufferSize = MIN_BUFFER_SIZE;
        }
    }

    void start() { timer.start(SECOND_MS / u16(screen()->refreshRate())); }

    void stop() { timer.stop(); }

    const f32* buffer;

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    inline void buildPeaks();
    [[nodiscard]] inline auto getBandIndices(u16 fftSize) const
        -> array<u16, TEN_BANDS>;

    FFmpeg::TXContext fftCtx;
    av_tx_fn fft = nullptr;

    u32 sampleRate = 0;
    AudioChannels channels;
    u16 bufferSize;

    f32 fftScale = 0;
    u16 lastFFTSampleCount = 0;
    u16 fftOutputSampleCount = 0;

    array<f32, MIN_BUFFER_SIZE / F32_SAMPLE_SIZE> fftSamples;
    u16 fftSampleCount = 0;

    array<f32, TEN_BANDS> peaks;
    PeakVisualizerMode mode = PeakVisualizerMode::DBFS;
    QGradient gradient = QGradient(QGradient::MorpheusDen);
    QGradient::Preset gradientPreset = QGradient::MorpheusDen;
    QTimer timer;
};
