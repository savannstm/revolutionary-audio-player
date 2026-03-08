#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"
#include "FWD.hpp"

#include <QFrame>

// TODO: Allow creating custom presets

class SpectrumVisualizer : public QFrame {
    Q_OBJECT

   public:
    enum class Mode : u8 {
        Relative,
        DBFS,
        Equal,
        Waveform
    };

    explicit SpectrumVisualizer(
        const shared_ptr<Settings>& settings_,
        QWidget* parent = nullptr
    );

    constexpr void setMode(Mode mode_);

    constexpr void setGradient(const QGradient& gradient_) {
        gradient = gradient_;
    }

    void visualize(f32* const samples) {
        memcpy(
            this->samples.data(),
            samples,
            usize(FFT_SAMPLE_COUNT * F32_SIZE)
        );
        buildPeaks();
        update();
    };

    constexpr void setBandCount(Bands bands);

    void reset();

   signals:
    void closed();
    void samplesRequested();

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

   private:
    void buildPeaks();
    inline auto isDetached() -> bool;
    inline void showCustomContextMenu();
    inline void toggleFullscreen(bool isFullscreen);

    static constexpr QSize MINIMUM_VISUALIZER_SIZE = QSize(128, 32);
    static constexpr u16 FFT_OUTPUT_COUNT = 257;

    array<f32, FFT_SAMPLE_COUNT> samples;
    alignas(
        sizeof(i32) * CHAR_BIT
    ) array<AVComplexFloat, FFT_OUTPUT_COUNT> fftOutput;
    array<f32, FFT_OUTPUT_COUNT> fftMagnitudes;
    array<f32, usize(MAX_BANDS) + 1> fftBandBins;
    array<f32, usize(MAX_BANDS)> peaks;

    u32 fftCollectedCount;

    FFmpeg::TXContext fftCtx;
    av_tx_fn fft;

    QGradient gradient;

    span<const f32> frequencies = span<const f32>(
        getFrequenciesForBands(Bands::Eighteen),
        EIGHTEEN_BANDS
    );

    SpectrumVisualizerSettings& settings;
};
