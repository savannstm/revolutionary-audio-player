#pragma once

#include "aliases.hpp"
#include "constants.hpp"

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

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    [[nodiscard]] static inline auto getBandIndices(u16 sampleRate, u16 fftSize)
        -> vector<u16>;

    vector<f32> peaks = vector<f32>(TEN_BANDS);
    PeakVisualizerMode mode = DBFS;
    QGradient gradient = QGradient(QGradient::MorpheusDen);
    QGradient::Preset gradientPreset = QGradient::MorpheusDen;
};
