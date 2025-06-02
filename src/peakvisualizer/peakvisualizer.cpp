#include "peakvisualizer.hpp"

#include "enums.hpp"

#include <QMenu>
#include <QPainter>

constexpr u8 VISUALIZER_WIDTH = 128;
constexpr u8 PEAK_PADDING = 4;
constexpr u16 MIN_SAMPLE_COUNT = 512;

PeakVisualizer::PeakVisualizer(QWidget* parent) : QWidget(parent) {
    setFixedWidth(VISUALIZER_WIDTH);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QWidget::customContextMenuRequested, this, [&] {
        auto* menu = new QMenu(this);
        auto* hideAction = menu->addAction(tr("Hide"));

        auto* modeMenu = new QMenu(menu);
        modeMenu->setTitle(tr("Mode"));

        auto* relativeAction = modeMenu->addAction(tr("Relative"));
        relativeAction->setCheckable(true);

        auto* dBFSAction = modeMenu->addAction(tr("dBFS"));
        dBFSAction->setCheckable(true);

        mode == Relative ? relativeAction->setChecked(true)
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
            views::filter(unfilteredPresetRange, [&](const u16 preset) {
            return !ranges::contains(missingPresets, preset);
        });

        for (const u16 preset : filteredPresetRange) {
            QAction* action = presetMenu->addAction(QString::number(preset));
            action->setCheckable(true);

            if (gradientPreset == preset) {
                action->setChecked(true);
            }

            connect(action, &QAction::triggered, this, [this, preset] {
                gradient = QGradient(as<QGradient::Preset>(preset));
                gradientPreset = as<QGradient::Preset>(preset);
            });
        }

        menu->addMenu(presetMenu);

        auto* selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == nullptr) {
            return;
        }

        if (selectedAction == hideAction) {
            hide();
        } else if (selectedAction == relativeAction) {
            mode = Relative;
        } else {
            mode = DBFS;
        }
    });
}

void PeakVisualizer::processSamples(
    const QByteArray& byteSamples,
    const u16 sampleRate
) {
    if (isHidden() || byteSamples.isEmpty()) {
        return;
    }

    // Measure the sample count
    const u16 sampleCount = byteSamples.size() / F32_SAMPLE_SIZE;

    // Inners of FFmpeg's FFT don't allow sizes less than 512
    if (sampleCount < MIN_SAMPLE_COUNT) {
        return;
    }

    // Compute size that is any power of 2
    const u16 fftSampleCount = as<u16>(1 << as<u16>(log2f(sampleCount)));

    // Copy only those samples, that will be used in FFT
    vector<f32> samples = vector<f32>(fftSampleCount);
    memcpy(
        samples.data(),
        byteSamples.constData(),
        as<i32>(fftSampleCount * F32_SAMPLE_SIZE)
    );

    AVTXContext* fftCtxPtr = nullptr;

    if (fftSampleCount != lastFFTSampleCount) {
        fftScale = 1.0F / as<f32>(fftSampleCount);

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

    fft(fftCtx.get(), output.data(), samples.data(), F32_SAMPLE_SIZE);

    const array<u16, TEN_BANDS> bandIndices =
        getBandIndices(sampleRate, fftSampleCount);
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
            maxMagnitude = max(maxMagnitude, magnitude);
        }

        peaks[band] = maxMagnitude;
    }

    update();
}

auto PeakVisualizer::getBandIndices(const u16 sampleRate, const u16 fftSize)
    -> array<u16, TEN_BANDS> {
    array<u16, TEN_BANDS> indices;

    for (const auto [idx, frequency] : views::enumerate(TEN_BAND_FREQUENCIES)) {
        const i32 index =
            as<i32>((frequency / as<f32>(sampleRate)) * as<f32>(fftSize));
        indices[idx] = min(index, fftSize / 2);
    }

    return indices;
}

void PeakVisualizer::paintEvent(QPaintEvent* /* event */) {
    QPainter painter = QPainter(this);
    painter.fillRect(rect(), Qt::transparent);

    const u16 peakWidth = this->width() / TEN_BANDS;
    const u16 height = this->height();

    f32 maxPeak = 1;

    if (mode == Relative) {
        maxPeak = *ranges::max_element(peaks);

        if (maxPeak == 0) {
            maxPeak = 1;
        }
    }

    for (const auto [band, peak] : views::enumerate(peaks)) {
        f32 normalizedPeak;

        if (mode == Relative) {
            normalizedPeak = peak / maxPeak;
        } else {
            constexpr f32 MIN_DBFS = -60;
            constexpr f32 MAX_DBFS = 0;

            const f32 dBFSPeak = 20.0F * log10f(max(peak, 0.000001F));
            normalizedPeak = (dBFSPeak - MIN_DBFS) / (MAX_DBFS - MIN_DBFS);
            normalizedPeak = clamp(normalizedPeak, 0.0F, 1.0F);
        }

        const u16 peakHeight = as<u16>(normalizedPeak * as<f32>(height));

        const QRectF bar = QRectF(
            as<u8>(band) * peakWidth,
            height - peakHeight,
            peakWidth - PEAK_PADDING,
            peakHeight
        );

        painter.fillRect(bar, gradient);
    }
}

void PeakVisualizer::reset() {
    peaks.fill(0);
    update();
}