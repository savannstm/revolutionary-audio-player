#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"
#include "Settings.hpp"

#include <QWidget>

class PeakVisualizer : public QWidget {
    Q_OBJECT

   public:
    explicit PeakVisualizer(QWidget* parent = nullptr);

    void processSamples(const QByteArray& byteSamples, u16 sampleRate);

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

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    [[nodiscard]] static inline auto getBandIndices(u16 sampleRate, u16 fftSize)
        -> array<u16, TEN_BANDS>;

    FFmpeg::TXContext fftCtx;
    av_tx_fn fft = nullptr;

    f32 fftScale = 0;
    u16 lastFFTSampleCount = 0;
    u16 fftOutputSampleCount = 0;

    array<f32, TEN_BANDS> peaks;
    PeakVisualizerMode mode = DBFS;
    QGradient gradient = QGradient(QGradient::MorpheusDen);
    QGradient::Preset gradientPreset = QGradient::MorpheusDen;
};
