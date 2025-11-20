#include "SpectrumVisualizer.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "InputPopup.hpp"

#include <QDrag>
#include <QIntValidator>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <numbers>

constexpr QSize MINIMUM_VISUALIZER_SIZE = QSize(128, 32);

SpectrumVisualizer::SpectrumVisualizer(
    const f32* const bufferData,
    const shared_ptr<Settings>& settings_,
    QWidget* const parent
) :
    QFrame(parent),
    settings(settings_->spectrumVisualizer) {
    setWindowTitle(tr("Spectrum Analyzer"));
    setMinimumSize(MINIMUM_VISUALIZER_SIZE);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::Panel);
    setLineWidth(1);

    setHidden(settings.hidden);
    setBandCount(settings.bands);
    gradient = QGradient(settings.preset);

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

    connect(
        this,
        &SpectrumVisualizer::customContextMenuRequested,
        this,
        &SpectrumVisualizer::showCustomContextMenu
    );

    connect(&timer, &QTimer::timeout, this, [this] -> void {
        buildPeaks();
        update();
    });
}

void SpectrumVisualizer::buildPeaks() {
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

    if (settings.mode != SpectrumVisualizerMode::Waveform) {
        fft(fftCtx.get(), fftOutput.data(), fftSamples.data(), F32_SAMPLE_SIZE);

        // Skip DC component
        const auto fftOutputNoDC = span<AVComplexFloat>(
            fftOutput.data() + 1,
            FFT_OUTPUT_SAMPLE_COUNT - 1
        );

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
        for (const u8 band : range(0, u8(settings.bands) + 1)) {
            fftBandBins[band] =
                powf(FFT_OUTPUT_SAMPLE_COUNT, f32(band) / f32(settings.bands)) -
                0.5F;
        }

        // Clear the peaks
        ranges::fill_n(peaks.begin(), u8(settings.bands), 0.0F);

        // Calculate peak magnitudes
        for (const u8 band : range(0, u8(settings.bands))) {
            const u16 start = u16(ceilf(fftBandBins[band]));
            const u16 end = u16(floorf(fftBandBins[band + 1]));
            f32 magnitude = 0;

            if (end < start) {
                magnitude += fftMagnitudes[end] *
                             (fftBandBins[band + 1] - fftBandBins[band]);
            } else {
                if (start > 0) {
                    magnitude += fftMagnitudes[start - 1] *
                                 (f32(start) - fftBandBins[band]);
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
    }
    /* CODE STOLEN FROM AUDACIOUS AUDIO PLAYER ENDS */
}

void SpectrumVisualizer::paintEvent(QPaintEvent* const event) {
    auto painter = QPainter(this);
    painter.fillRect(rect(), Qt::transparent);

    const u16 width = this->width();
    const u16 height = this->height();

    if (settings.mode == SpectrumVisualizerMode::Waveform) {
        painter.setRenderHint(QPainter::Antialiasing);

        const f32 centerY = f32(height) / 2.0F;
        const f32 sampleWidth = f32(width) / f32(FFT_SAMPLE_COUNT);

        f32 maxAmplitude = 0.0F;
        for (const u16 idx : range(0, FFT_SAMPLE_COUNT)) {
            maxAmplitude = max(maxAmplitude, fabsf(fftSamples[idx]));
        }

        if (maxAmplitude == 0.0F) {
            maxAmplitude = 1.0F;
        }

        QPainterPath waveformPath;
        waveformPath.moveTo(0, centerY);

        for (const u16 idx : range(0, FFT_SAMPLE_COUNT)) {
            const f32 xPos = f32(idx) * sampleWidth;
            const f32 normalizedSample = fftSamples[idx] / maxAmplitude;
            const f32 yPos = centerY - (normalizedSample * centerY * 0.9F);

            if (idx == 0) {
                waveformPath.moveTo(xPos, yPos);
            } else {
                waveformPath.lineTo(xPos, yPos);
            }
        }

        QPen pen = QPen(gradient.stops().first().second);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawPath(waveformPath);
    } else {
        const f32 peakWidth = f32(width) / f32(settings.bands);

        f32 maxPeak = 1;

        if (settings.mode == SpectrumVisualizerMode::Relative) {
            maxPeak = *ranges::max_element(
                span(peaks.begin(), usize(settings.bands))
            );

            if (maxPeak == 0) {
                maxPeak = 1;
            }
        }

        for (const u8 band : range(0, u8(settings.bands))) {
            const f32 peak = peaks[band];
            f32 normalizedPeak;

            if (settings.mode == SpectrumVisualizerMode::Relative) {
                normalizedPeak = peak / maxPeak;
            } else {
                constexpr f32 MIN_DBFS = -40;
                constexpr f32 MAX_DBFS = 0;

                const f32 dBFSPeak =
                    (20.0F * log10f(peak)) + settings.gainFactor;
                normalizedPeak = (dBFSPeak - MIN_DBFS) / (MAX_DBFS - MIN_DBFS);

                if (settings.mode == SpectrumVisualizerMode::Equal) {
                    constexpr f32 MIN_NORMALIZED = 0.35F;

                    if (normalizedPeak < MIN_NORMALIZED) {
                        normalizedPeak += MIN_NORMALIZED;
                    }
                }

                normalizedPeak = clamp(normalizedPeak, 0.0F, 1.0F);
            }

            const u16 peakHeight = u16(normalizedPeak * f32(height));

            const QRectF bar = QRectF(
                peakWidth * f32(band),
                height - peakHeight,
                peakWidth - f32(settings.peakPadding),
                peakHeight
            );

            painter.fillRect(bar, gradient);
        }
    }

    QFrame::paintEvent(event);
}

void SpectrumVisualizer::reset() {
    ranges::fill_n(peaks.data(), u8(settings.bands), 0);
    update();
    stop();
}

auto SpectrumVisualizer::isDetached() -> bool {
    return parent() == nullptr;
}

void SpectrumVisualizer::mousePressEvent(QMouseEvent* const event) {
    const bool isDetached = this->isDetached();

    if (!isDetached && event->type() == QEvent::MouseButtonDblClick) {
        setParent(
            nullptr,
            Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint
        );
        show();

        setContextMenuPolicy(Qt::CustomContextMenu);
        move(event->pos());
    } else if (isDetached && (event->buttons() & Qt::LeftButton) != 0) {
        auto* const drag = new QDrag(this);
        auto* const mime = new QMimeData();

        mime->setText("spectrumvisualizer");

        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
    }
}

void SpectrumVisualizer::showCustomContextMenu() {
    const bool isDetached = this->isDetached();

    auto* const menu = new QMenu(this);

    auto* const modeMenu = new QMenu(menu);
    modeMenu->setTitle(tr("Mode"));

    QAction* const relativeAction = modeMenu->addAction(tr("Peak (Relative)"));
    relativeAction->setCheckable(true);

    QAction* const dBFSAction = modeMenu->addAction(tr("Peak (dBFS)"));
    dBFSAction->setCheckable(true);

    QAction* const equalAction = modeMenu->addAction(tr("Peak (Equal)"));
    equalAction->setCheckable(true);

    QAction* const waveformAction = modeMenu->addAction(tr("Waveform"));
    waveformAction->setCheckable(true);

    switch (settings.mode) {
        case SpectrumVisualizerMode::Relative:
            relativeAction->setChecked(true);
            break;
        case SpectrumVisualizerMode::DBFS:
            dBFSAction->setChecked(true);
            break;
        case SpectrumVisualizerMode::Equal:
            equalAction->setChecked(true);
            break;
        case SpectrumVisualizerMode::Waveform:
            waveformAction->setChecked(true);
            break;
    }

    menu->addMenu(modeMenu);

    if (settings.mode != SpectrumVisualizerMode::Waveform) {
        auto* const bandsMenu = new QMenu(menu);
        bandsMenu->setTitle(tr("Band Count"));

        QAction* const eighteenBandsAction =
            bandsMenu->addAction(u"18"_s, this, [this] -> void {
            setBandCount(Bands::Eighteen);
        });
        eighteenBandsAction->setCheckable(true);

        QAction* const thirtyBandsAction =
            bandsMenu->addAction(u"30"_s, this, [this] -> void {
            setBandCount(Bands::Thirty);
        });
        thirtyBandsAction->setCheckable(true);

        switch (settings.bands) {
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

        menu->addAction(tr("Change Peak Padding"), this, [this] -> void {
            auto* const inputPopup = new InputPopup(
                QString::number(settings.peakPadding),
                QCursor::pos(),
                this
            );

            auto* const intValidator = new QIntValidator(0, 64, inputPopup);
            inputPopup->setValidator(intValidator);
            inputPopup->show();

            connect(
                inputPopup,
                &InputPopup::finished,
                this,
                [this](const QString& text) -> void {
                settings.peakPadding = text.toUInt();
            },
                Qt::SingleShotConnection
            );
        });

        menu->addAction(tr("Set Gain Factor"), this, [this] -> void {
            auto* const inputPopup = new InputPopup(
                QString::number(settings.gainFactor),
                QCursor::pos(),
                this
            );

            auto* const intValidator =
                new QDoubleValidator(0, 40, 2, inputPopup);
            inputPopup->setValidator(intValidator);
            inputPopup->show();

            connect(
                inputPopup,
                &InputPopup::finished,
                this,
                [this](const QString& text) -> void {
                settings.gainFactor = QLocale::system().toFloat(text);
            },
                Qt::SingleShotConnection
            );
        });
    }

    auto* const presetMenu = new QMenu(menu);
    presetMenu->setTitle(tr("Presets"));

    constexpr auto unfilteredPresetRange =
        range(1, QGradient::Preset::NumPresets);

    constexpr array<u16, 10> missingPresets = { 39,  40,  74, 141, 130,
                                                135, 119, 71, 27,  105 };

    // For some idiotic reasons, those indices aren't in Preset enum
    auto filteredPresetRange = views::filter(
        unfilteredPresetRange,
        [this, &missingPresets](const u16 preset) -> bool {
        return !ranges::contains(missingPresets, preset);
    }
    );

    for (const u16 preset : filteredPresetRange) {
        QAction* const action = presetMenu->addAction(QString::number(preset));
        action->setCheckable(true);

        if (settings.preset == preset) {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, this, [this, preset] -> void {
            gradient = QGradient(QGradient::Preset(preset));
            settings.preset = QGradient::Preset(preset);
        });
    }

    menu->addMenu(presetMenu);

    if (isDetached) {
        const bool isFullscreen = isFullScreen();
        const bool isOnTop = (windowFlags() & Qt::WindowStaysOnTopHint) != 0;

        const QAction* maximizeAction = menu->addAction(
            tr(isFullscreen ? "Minimize" : "Maximize To Fullscreen"),
            this,
            [this, isFullscreen] -> void { toggleFullscreen(isFullscreen); }
        );

        const QAction* alwaysOnTopAction = menu->addAction(
            tr(isOnTop ? "Unset Always On Top" : "Set Always On Top"),
            this,
            [this, isOnTop] -> void {
            setWindowFlag(Qt::WindowStaysOnTopHint, !isOnTop);
            show();
        }
        );
    }

    const QAction* const selectedAction = menu->exec(QCursor::pos());

    if (selectedAction == nullptr) {
        return;
    }

    if (selectedAction == relativeAction) {
        settings.mode = SpectrumVisualizerMode::Relative;
    } else if (selectedAction == dBFSAction) {
        settings.mode = SpectrumVisualizerMode::DBFS;
    } else if (selectedAction == equalAction) {
        settings.mode = SpectrumVisualizerMode::Equal;
    } else if (selectedAction == waveformAction) {
        settings.mode = SpectrumVisualizerMode::Waveform;
    }

    delete menu;
}

void SpectrumVisualizer::toggleFullscreen(const bool isFullscreen) {
    if (isFullscreen) {
        showNormal();

        const QScreen* currentScreen = screen();

        if (currentScreen == nullptr) {
            currentScreen = QApplication::primaryScreen();
        }

        const QRect screenGeometry = currentScreen->availableGeometry();
        const QSize windowSize = size();

        const i32 xPos = screenGeometry.x() +
                         ((screenGeometry.width() - windowSize.width()) / 2);
        const i32 yPos = screenGeometry.y() +
                         ((screenGeometry.height() - windowSize.height()) / 2);

        move(xPos, yPos);
    } else {
        showFullScreen();
    }
}