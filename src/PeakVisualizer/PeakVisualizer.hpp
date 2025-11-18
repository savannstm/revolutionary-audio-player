#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"
#include "Settings.hpp"

#include <QFrame>
#include <QScreen>
#include <QTimer>

// TODO: Allow changing width
// TODO: Implement gain factor for visualization
// TODO: Allow setting a gap between peaks
// TODO: Make floatable

class PeakVisualizer : public QFrame {
    Q_OBJECT

   public:
    explicit PeakVisualizer(const f32* bufferData, QWidget* parent = nullptr);

    constexpr void setMode(const PeakVisualizerMode mode_) { mode = mode_; }

    constexpr void setGradient(const QGradient& gradient_) {
        gradient = gradient_;
    }

    constexpr void setAudioProperties(const u32 rate, const AudioChannels chn) {
        sampleRate = rate;
        channels = chn;

        if (channels == AudioChannels::Surround51) {
            bufferSize = MIN_BUFFER_SIZE_3BYTES;
        } else {
            bufferSize = MIN_BUFFER_SIZE;
        }
    }

    constexpr void setBandCount(const Bands bands) {
        bandCount = bands;
        frequencies =
            span<const f32>(getFrequenciesForBands(bands), usize(bandCount));
    }

    void reset();

    void loadSettings(const PeakVisualizerSettings& settings) {
        setHidden(settings.hidden);
        gradientPreset = settings.preset;
        gradient = QGradient(gradientPreset);
        mode = settings.mode;
        setBandCount(settings.bands);
    };

    void saveSettings(PeakVisualizerSettings& settings) {
        settings.hidden = isHidden();
        settings.preset = gradientPreset;
        settings.mode = mode;
        settings.bands = bandCount;
    }

    void start() {
        timer.start(SECOND_MS / u16(screen()->refreshRate()));
        setUpdatesEnabled(true);
    }

    void stop() {
        timer.stop();
        setUpdatesEnabled(false);
    }

    const f32* buffer;

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    inline void buildPeaks();

    array<f32, MIN_BUFFER_SIZE> fftSamples;
    alignas(
        sizeof(i32) * CHAR_BIT
    ) array<AVComplexFloat, FFT_OUTPUT_SAMPLE_COUNT> fftOutput;
    array<f32, FFT_OUTPUT_SAMPLE_COUNT> fftMagnitudes;
    array<f32, usize(Bands::Thirty) + 1> fftBandBins;
    array<f32, usize(Bands::Thirty)> peaks;

    QGradient gradient = QGradient(QGradient::MorpheusDen);

    QTimer timer;

    span<const f32> frequencies = span<const f32>(
        getFrequenciesForBands(Bands::Eighteen),
        usize(Bands::Eighteen)
    );
    FFmpeg::TXContext fftCtx;
    av_tx_fn fft;

    QGradient::Preset gradientPreset = QGradient::MorpheusDen;

    u32 sampleRate = 0;
    u16 bufferSize;

    u16 fftSampleCount = 0;

    AudioChannels channels;

    Bands bandCount = Bands::Eighteen;
    PeakVisualizerMode mode = PeakVisualizerMode::DBFS;
};
