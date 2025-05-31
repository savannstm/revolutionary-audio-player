#include "peakvisualizer.hpp"

#include "enums.hpp"
#include "ffmpeg.hpp"

#include <QMenu>
#include <QPainter>

constexpr u8 VISUALIZER_WIDTH = 128;
constexpr u8 PEAK_PADDING = 4;

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

        for (const u16 preset : range(1, QGradient::Preset::NumPresets)) {
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
    if (isHidden()) {
        return;
    }

    // Measure the sample count and copy samples into f32 vector
    const u16 sampleCount = byteSamples.size() / F32_SAMPLE_SIZE;
    vector<f32> samples = vector<f32>(sampleCount);
    memcpy(samples.data(), byteSamples.constData(), byteSamples.size());

    // Compute size that is any power of 2
    const i32 fftSize = 1 << as<i32>(log2f(sampleCount));

    // Compute the scale
    const f32 fftScale = 1.0F / as<f32>(fftSize);

    FFmpeg::TXContext fftCtx;
    AVTXContext* fftCtxPtr = nullptr;
    av_tx_fn fft = nullptr;

    av_tx_init(&fftCtxPtr, &fft, AV_TX_FLOAT_RDFT, 0, fftSize, &fftScale, 0);
    fftCtx.reset(fftCtxPtr);

    vector<AVComplexFloat> output = vector<AVComplexFloat>((fftSize / 2) + 1);
    fft(fftCtx.get(), output.data(), samples.data(), F32_SAMPLE_SIZE);

    vector<u16> bandIndices = getBandIndices(sampleRate, fftSize);
    ranges::fill(peaks, 0);

    for (u8 band = 0; band < TEN_BANDS; band++) {
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
    -> vector<u16> {
    vector<u16> indices;
    indices.reserve(TEN_BANDS);

    for (const f32 frequency : TEN_BAND_FREQUENCIES) {
        const i32 index =
            as<i32>((frequency / as<f32>(sampleRate)) * as<f32>(fftSize));
        indices.emplace_back(min(index, fftSize / 2));
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
    ranges::fill(peaks, 0);
    update();
}