#pragma once

#include "Aliases.hpp"
#include "Duration.hpp"
#include "Enums.hpp"

#include <QLabel>

class ProgressLabel : public QLabel {
    Q_OBJECT

   public:
    explicit ProgressLabel(QWidget* const parent = nullptr) : QLabel(parent) {
        setCursor(QCursor(Qt::PointingHandCursor));
    }

    [[nodiscard]] auto duration() const -> QString {
        const QString text = this->text();
        return text.sliced(text.indexOf('/') + 1);
    };

    [[nodiscard]] auto durationSeconds() const -> u32 {
        const QString duration = this->duration();
        return Duration::stringToSeconds(duration).value();
    };

    [[nodiscard]] auto elapsed() const -> QString {
        const QString text = this->text();
        return text.sliced(0, text.indexOf('/'));
    };

    [[nodiscard]] auto elapsedSeconds() const -> u32 {
        const QString elapsed = this->elapsed();
        return Duration::stringToSeconds(elapsed).value();
    };

    void setDuration(const Duration dur) {
        durationSecs = dur.toSeconds();
        updateDisplay();
    }

    void setDuration(const u32 secs) {
        durationSecs = secs;
        updateDisplay();
    }

    void setDuration(const QStringView string) {
        durationSecs = Duration::stringToSeconds(string).value();
        updateDisplay();
    }

    void setElapsed(const Duration dur) {
        elapsedSecs = dur.toSeconds();
        updateDisplay();
    }

    void setElapsed(const u32 secs) {
        elapsedSecs = secs;
        updateDisplay();
    }

    void setElapsed(const QStringView string) {
        elapsedSecs = Duration::stringToSeconds(string).value();
        updateDisplay();
    }

   signals:
    void modeChanged(ProgressDisplayMode mode);

   protected:
    void mousePressEvent(QMouseEvent* const event) override {
        if (mode == ProgressDisplayMode::Elapsed) {
            mode = ProgressDisplayMode::Remaining;
        } else {
            mode = ProgressDisplayMode::Elapsed;
        }

        updateDisplay();
    }

   private:
    void updateDisplay() {
        if (mode == ProgressDisplayMode::Elapsed) {
            setText(
                Duration::secondsToString(elapsedSecs) + '/' +
                Duration::secondsToString(durationSecs)
            );
        } else {
            const u32 remaining =
                durationSecs > elapsedSecs ? durationSecs - elapsedSecs : 0;

            setText(
                '-' + Duration::secondsToString(remaining) + '/' +
                Duration::secondsToString(durationSecs)
            );
        }
    }

    ProgressDisplayMode mode = ProgressDisplayMode::Elapsed;
    u32 elapsedSecs = 0;
    u32 durationSecs = 0;
};