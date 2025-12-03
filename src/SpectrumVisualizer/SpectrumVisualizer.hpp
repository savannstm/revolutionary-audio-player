#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"
#include "FWD.hpp"

#include <QFrame>
#include <QTimer>

// TODO: Allow creating custom presets

class SpectrumVisualizer : public QFrame {
    Q_OBJECT

   public:
    explicit SpectrumVisualizer(
        const f32* bufferData,
        const shared_ptr<Settings>& settings_,
        QWidget* parent = nullptr
    );

    constexpr void setMode(SpectrumVisualizerMode mode_);

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

    constexpr void setBandCount(Bands bands);

    void reset();
    void start();
    void stop();

    const f32* buffer;

   signals:
    void closed();

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

   private:
    inline void buildPeaks();
    inline auto isDetached() -> bool;
    inline void showCustomContextMenu();
    inline void toggleFullscreen(bool isFullscreen);

    array<f32, MIN_BUFFER_SIZE> fftSamples;
    alignas(
        sizeof(i32) * CHAR_BIT
    ) array<AVComplexFloat, FFT_OUTPUT_SAMPLE_COUNT> fftOutput;
    array<f32, FFT_OUTPUT_SAMPLE_COUNT> fftMagnitudes;
    array<f32, THIRTY_BANDS + 1> fftBandBins;
    array<f32, THIRTY_BANDS> peaks;

    QGradient gradient;
    QTimer timer;

    span<const f32> frequencies = span<const f32>(
        getFrequenciesForBands(Bands::Eighteen),
        EIGHTEEN_BANDS
    );
    FFmpeg::TXContext fftCtx;
    av_tx_fn fft;

    SpectrumVisualizerSettings& settings;

    u32 sampleRate = 0;

    u16 bufferSize;
    u16 fftSampleCount = 0;

    AudioChannels channels;
};
