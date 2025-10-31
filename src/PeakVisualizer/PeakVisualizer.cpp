#include "PeakVisualizer.hpp"

#include "Constants.hpp"
#include "Enums.hpp"

#include <QMenu>
#include <QPainter>

constexpr u8 VISUALIZER_WIDTH = 128;
constexpr u8 PEAK_PADDING = 4;
constexpr u16 MIN_FFT_SAMPLE_COUNT = 1024;

PeakVisualizer::PeakVisualizer(const f32* bufferData, QWidget* parent) :
    QWidget(parent) {
    setFixedWidth(VISUALIZER_WIDTH);
    setContextMenuPolicy(Qt::CustomContextMenu);

    buffer = bufferData;

    connect(this, &QWidget::customContextMenuRequested, this, [&] -> void {
        auto* menu = new QMenu(this);

        auto* modeMenu = new QMenu(menu);
        modeMenu->setTitle(tr("Mode"));

        auto* relativeAction = modeMenu->addAction(tr("Relative"));
        relativeAction->setCheckable(true);

        auto* dBFSAction = modeMenu->addAction(tr("dBFS"));
        dBFSAction->setCheckable(true);

        mode == PeakVisualizerMode::Relative ? relativeAction->setChecked(true)
                                             : dBFSAction->setChecked(true);

        menu->addMenu(modeMenu);

        auto* presetMenu = new QMenu(menu);
        presetMenu->setTitle(tr("Presets"));

        constexpr auto unfilteredPresetRange =
            range(1, QGradient::Preset::NumPresets);

        constexpr array<u16, 9> missingPresets = { 39,  40,  74, 141, 130,
                                                   135, 119, 71, 27 };

        // For some idiotic reasons, those indices aren't in Preset enum
        auto filteredPresetRange =
            views::filter(unfilteredPresetRange, [&](const u16 preset) -> bool {
            return !ranges::contains(missingPresets, preset);
        });

        for (const u16 preset : filteredPresetRange) {
            QAction* action = presetMenu->addAction(QString::number(preset));
            action->setCheckable(true);

            if (gradientPreset == preset) {
                action->setChecked(true);
            }

            connect(action, &QAction::triggered, this, [this, preset] -> void {
                gradient = QGradient(QGradient::Preset(preset));
                gradientPreset = QGradient::Preset(preset);
            });
        }

        menu->addMenu(presetMenu);

        auto* selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == nullptr) {
            return;
        }

        if (selectedAction == relativeAction) {
            mode = PeakVisualizerMode::Relative;
        } else {
            mode = PeakVisualizerMode::DBFS;
        }
    });

    connect(&timer, &QTimer::timeout, this, [&] -> void {
        buildPeaks();
        update();
    });
}

void PeakVisualizer::buildPeaks() {
    // Measure the sample count
    const u16 sampleCount = (bufferSize / F32_SAMPLE_SIZE) / channels;

    // This shit screws up the FFT by mixing the samples from all channels
    for (u16 sample = 0; sample < sampleCount; sample++) {
        f32 mixedSample = 0.0F;

        for (u8 channel = 0; channel < channels; channel++) {
            const u32 index = (sample * channels) + channel;
            mixedSample += buffer[index];
        }

        fftSamples[sample + fftSampleCount] = mixedSample / f32(channels);
    }

    // sampleCount is always of the power of two
    fftSampleCount += (1 << std::countr_zero(sampleCount));

    // Inners of FFmpeg's FFT don't allow less than N samples
    if (fftSampleCount < MIN_FFT_SAMPLE_COUNT) {
        return;
    }

    if (fftSampleCount != lastFFTSampleCount) {
        fftScale = 1.0F / f32(fftSampleCount);

        AVTXContext* fftCtxPtr = nullptr;

        av_tx_init(
            &fftCtxPtr,
            &fft,
            AV_TX_FLOAT_RDFT,
            0,
            fftSampleCount,
            &fftScale,
            0
        );

        fftCtx.reset(fftCtxPtr);
        fftOutputSampleCount = (fftSampleCount / 2) + 1;
        lastFFTSampleCount = fftSampleCount;
    }

    vector<AVComplexFloat> output =
        vector<AVComplexFloat>(fftOutputSampleCount);

    fft(fftCtx.get(), output.data(), fftSamples.data(), F32_SAMPLE_SIZE);

    const array<u16, TEN_BANDS> bandIndices = getBandIndices(fftSampleCount);
    peaks.fill(0);

    for (const u8 band : range(0, TEN_BANDS)) {
        const u16 start = band == 0 ? 0 : bandIndices[band - 1];
        const u16 end = bandIndices[band];
        f32 maxMagnitude = 0;

        for (u16 i = start; i < end && i < output.size(); i++) {
            const f32 real = output[i].re;
            const f32 imaginary = output[i].im;

            // Compute the loudness magnitude
            const f32 magnitude = sqrtf(powf(real, 2) + powf(imaginary, 2));
            maxMagnitude = std::max<f32>(maxMagnitude, magnitude);
        }

        peaks[band] = maxMagnitude;
    }

    fftSampleCount = 0;
}

auto PeakVisualizer::getBandIndices(const u16 fftSize) const
    -> array<u16, TEN_BANDS> {
    array<u16, TEN_BANDS> indices;

    for (const auto [idx, frequency] : views::enumerate(TEN_BAND_FREQUENCIES)) {
        const i32 index = i32((frequency / f32(sampleRate)) * f32(fftSize));
        indices[idx] = std::min<u16>(index, fftSize / 2);
    }

    return indices;
}

void PeakVisualizer::paintEvent(QPaintEvent* /* event */) {
    QPainter painter = QPainter(this);
    painter.fillRect(rect(), Qt::transparent);

    const u16 peakWidth = this->width() / TEN_BANDS;
    const u16 height = this->height();

    f32 maxPeak = 1;

    if (mode == PeakVisualizerMode::Relative) {
        maxPeak = *ranges::max_element(peaks);

        if (maxPeak == 0) {
            maxPeak = 1;
        }
    }

    for (const auto [band, peak] : views::enumerate(peaks)) {
        f32 normalizedPeak;

        if (mode == PeakVisualizerMode::Relative) {
            normalizedPeak = peak / maxPeak;
        } else {
            constexpr f32 MIN_DBFS = -60;
            constexpr f32 MAX_DBFS = 0;

            const f32 dBFSPeak = 20.0F * log10f(std::max<f32>(peak, 0.000001F));
            normalizedPeak = (dBFSPeak - MIN_DBFS) / (MAX_DBFS - MIN_DBFS);
            normalizedPeak = clamp(normalizedPeak, 0.0F, 1.0F);
        }

        const u16 peakHeight = u16(normalizedPeak * f32(height));

        const QRect bar = QRect(
            u8(band) * peakWidth,
            height - peakHeight,
            peakWidth - PEAK_PADDING,
            peakHeight
        );

        painter.fillRect(bar, gradient);
    }
}

void PeakVisualizer::reset() {
    timer.stop();
    peaks.fill(0);
    update();
}