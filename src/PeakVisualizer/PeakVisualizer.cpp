#include "PeakVisualizer.hpp"

#include "Constants.hpp"
#include "Enums.hpp"

#include <QMenu>
#include <QPainter>
#include <numbers>

constexpr u8 VISUALIZER_WIDTH = 128;
constexpr u8 PEAK_PADDING = 4;

PeakVisualizer::PeakVisualizer(
    const f32* const bufferData,
    QWidget* const parent
) :
    QFrame(parent) {
    setFixedWidth(VISUALIZER_WIDTH);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::Panel);
    setLineWidth(1);
    setUpdatesEnabled(false);

    buffer = bufferData;

    AVTXContext* fftCtxPtr = nullptr;

    av_tx_init(
        &fftCtxPtr,
        &fft,
        AV_TX_FLOAT_RDFT,
        0,
        FFT_SAMPLE_COUNT,
        // We need no scaling - we only care about relative amplitude, and
        // if everything's small, then relative amplitude is small too
        nullptr,
        0
    );

    fftCtx.reset(fftCtxPtr);

    connect(this, &QWidget::customContextMenuRequested, this, [&] -> void {
        auto* const menu = new QMenu(this);

        auto* const modeMenu = new QMenu(menu);
        modeMenu->setTitle(tr("Mode"));

        QAction* const relativeAction = modeMenu->addAction(tr("Relative"));
        relativeAction->setCheckable(true);

        QAction* const dBFSAction = modeMenu->addAction(tr("dBFS"));
        dBFSAction->setCheckable(true);

        mode == PeakVisualizerMode::Relative ? relativeAction->setChecked(true)
                                             : dBFSAction->setChecked(true);

        menu->addMenu(modeMenu);

        auto* const bandsMenu = new QMenu(menu);
        bandsMenu->setTitle(tr("Band Count"));

        QAction* const eighteenBandsAction = bandsMenu->addAction(u"18"_s);
        eighteenBandsAction->setCheckable(true);

        QAction* const thirtyBandsAction = bandsMenu->addAction(u"30"_s);
        thirtyBandsAction->setCheckable(true);

        switch (bandCount) {
            case Bands::Eighteen:
                eighteenBandsAction->setChecked(true);
                break;
            case Bands::Thirty:
                thirtyBandsAction->setChecked(true);
                break;
            default:
                break;
        }

        menu->addMenu(bandsMenu);

        auto* const presetMenu = new QMenu(menu);
        presetMenu->setTitle(tr("Presets"));

        constexpr auto unfilteredPresetRange =
            range(1, QGradient::Preset::NumPresets);

        constexpr array<u16, 10> missingPresets = { 39,  40,  74, 141, 130,
                                                    135, 119, 71, 27,  105 };

        // For some idiotic reasons, those indices aren't in Preset enum
        auto filteredPresetRange =
            views::filter(unfilteredPresetRange, [&](const u16 preset) -> bool {
            return !ranges::contains(missingPresets, preset);
        });

        for (const u16 preset : filteredPresetRange) {
            QAction* const action =
                presetMenu->addAction(QString::number(preset));
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

        const QAction* const selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == nullptr) {
            return;
        }

        if (selectedAction == relativeAction) {
            mode = PeakVisualizerMode::Relative;
        } else if (selectedAction == dBFSAction) {
            mode = PeakVisualizerMode::DBFS;
        } else if (selectedAction == eighteenBandsAction) {
            setBandCount(Bands::Eighteen);
        } else if (selectedAction == thirtyBandsAction) {
            setBandCount(Bands::Thirty);
        }
    });

    connect(&timer, &QTimer::timeout, this, [&] -> void {
        buildPeaks();
        update();
    });
}

void PeakVisualizer::buildPeaks() {
    const u16 frameCount = (bufferSize / F32_SAMPLE_SIZE) / u8(channels);

    if (channels == AudioChannels::Mono) {
        memcpy(fftSamples.data() + fftSampleCount, buffer, bufferSize);
    } else {
        for (const u16 frame : range(0, frameCount)) {
            f32 mixedSample = 0.0F;

            for (const u8 channel : range(0, u8(channels))) {
                const u32 sample = (frame * u8(channels)) + channel;
                mixedSample += buffer[sample];
            }

            fftSamples[frame + fftSampleCount] = mixedSample / f32(channels);
        }
    }

    if (channels == AudioChannels::Surround51 ||
        bufferSize == MIN_BUFFER_SIZE_3BYTES) {
        fftSampleCount += (1 << av_ceil_log2_c(frameCount));
    } else {
        fftSampleCount += frameCount;
    }

    // Inners of FFmpeg's FFT don't allow less than N samples
    if (fftSampleCount < FFT_SAMPLE_COUNT) {
        return;
    }

    fftSampleCount = 0;

    for (const u16 idx : range(0, FFT_SAMPLE_COUNT)) {
        constexpr f32 TWO_PI = std::numbers::pi_v<f32> * 2.0F;
        const f32 hammingWindow =
            0.54F -
            (0.46F * cosf(TWO_PI * f32(idx) / f32(FFT_SAMPLE_COUNT - 1)));
        fftSamples[idx] *= hammingWindow;
    }

    fft(fftCtx.get(), fftOutput.data(), fftSamples.data(), F32_SAMPLE_SIZE);

    // Skip DC component
    const auto fftOutputNoDC =
        span<AVComplexFloat>(fftOutput.data() + 1, FFT_OUTPUT_SAMPLE_COUNT - 1);

    /* CODE STOLEN FROM AUDACIOUS AUDIO PLAYER STARTS */
    for (const u16 sample : range(0, FFT_OUTPUT_SAMPLE_COUNT - 1)) {
        const AVComplexFloat complex = fftOutputNoDC[sample];

        fftMagnitudes[sample] =
            (sqrtf((complex.re * complex.re) + (complex.im * complex.im)) *
             2.0F) /
            f32(FFT_SAMPLE_COUNT);
    }

    // Special treatment for Nyquist component
    const AVComplexFloat nyquistComponent =
        fftOutput[FFT_OUTPUT_SAMPLE_COUNT - 1];

    fftMagnitudes[FFT_OUTPUT_SAMPLE_COUNT - 1] =
        sqrtf(
            (nyquistComponent.re * nyquistComponent.re) +
            (nyquistComponent.im * nyquistComponent.im)
        ) /
        f32(FFT_SAMPLE_COUNT);

    // Compute bins ranging from 0 to 22 kHz
    for (const u8 band : range(0, u8(bandCount) + 1)) {
        fftBandBins[band] =
            powf(FFT_OUTPUT_SAMPLE_COUNT, f32(band) / f32(bandCount)) - 0.5F;
    }

    // Clear the peaks
    ranges::fill_n(peaks.begin(), u8(bandCount), 0.0F);

    // Calculate peak magnitudes
    for (const u8 band : range(0, u8(bandCount))) {
        const u16 start = u16(ceilf(fftBandBins[band]));
        const u16 end = u16(floorf(fftBandBins[band + 1]));
        f32 magnitude = 0;

        if (end < start) {
            magnitude += fftMagnitudes[end] *
                         (fftBandBins[band + 1] - fftBandBins[band]);
        } else {
            if (start > 0) {
                magnitude +=
                    fftMagnitudes[start - 1] * (f32(start) - fftBandBins[band]);
            }

            for (const u16 idx : range(start, end)) {
                magnitude += fftMagnitudes[idx];
            }

            if (end < FFT_OUTPUT_SAMPLE_COUNT) {
                magnitude +=
                    fftMagnitudes[end] * (fftBandBins[band + 1] - f32(end));
            }
        }

        peaks[band] = max<f32>(magnitude, peaks[band]);
    }
    /* CODE STOLEN FROM AUDACIOUS AUDIO PLAYER ENDS */
}

void PeakVisualizer::paintEvent(QPaintEvent* const event) {
    auto painter = QPainter(this);
    painter.fillRect(rect(), Qt::transparent);

    const f32 peakWidth = f32(this->width()) / f32(bandCount);
    const u16 height = this->height();

    f32 maxPeak = 1;

    if (mode == PeakVisualizerMode::Relative) {
        maxPeak = *ranges::max_element(span(peaks.begin(), usize(bandCount)));

        if (maxPeak == 0) {
            maxPeak = 1;
        }
    }

    for (const u8 band : range(0, u8(bandCount))) {
        const f32 peak = peaks[band];
        f32 normalizedPeak;

        // TODO: Mode where all peaks have approximately equal size
        if (mode == PeakVisualizerMode::Relative) {
            normalizedPeak = peak / maxPeak;
        } else if (mode == PeakVisualizerMode::DBFS) {
            constexpr f32 MIN_DBFS = -40;
            constexpr f32 MAX_DBFS = 0;

            const f32 dBFSPeak = 20.0F * log10f(peak);
            normalizedPeak = (dBFSPeak - MIN_DBFS) / (MAX_DBFS - MIN_DBFS);
            normalizedPeak = clamp(normalizedPeak, 0.0F, 1.0F);
        }

        const u16 peakHeight = u16(normalizedPeak * f32(height));

        const QRectF bar = QRectF(
            peakWidth * f32(band),
            height - peakHeight,
            peakWidth,
            peakHeight
        );

        painter.fillRect(bar, gradient);
    }

    QFrame::paintEvent(event);
}

void PeakVisualizer::reset() {
    ranges::fill_n(peaks.data(), u8(bandCount), 0);
    update();
    stop();
}