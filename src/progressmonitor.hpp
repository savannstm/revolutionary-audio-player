#ifndef PROGRESSMONITOR_HPP
#define PROGRESSMONITOR_HPP

#include <QBuffer>
#include <QPlainTextEdit>
#include <QTimer>

#include "customslider.hpp"
#include "functions.hpp"
#include "mainwindow.hpp"
#include "type_aliases.hpp"

constexpr u8 interval = 100;

class AudioMonitor : public QObject {
    Q_OBJECT
   public:
    AudioMonitor(QBuffer* buf, QPlainTextEdit* progressTimer,
                 CustomSlider* progressSlider, QObject* parent = nullptr)
        : QObject(parent),
          buffer(buf),
          progressTimer(progressTimer),
          progressSlider(progressSlider) {
        connect(&timer, &QTimer::timeout, this, &AudioMonitor::checkPosition);
        timer.start(interval);
    }

    void updateDuration(const string& dur) { duration = dur; }
    void updateBuffer(QBuffer* buf) { buffer = buf; }

   signals:
    void playbackFinished();

   private:
    QBuffer* buffer;
    QTimer timer;
    i64 lastPos;
    QPlainTextEdit* progressTimer;
    CustomSlider* progressSlider;
    string duration;
    i64 previousPosition;
    i64 previousSecond;

   private slots:
    void checkPosition() {
        if (buffer != nullptr) {
            const i64 bytePosition = buffer->pos();
            const i64 bufferSize = buffer->size();

            if (bufferSize > 0) {
                if (previousPosition != bytePosition) {
                    const i64 playbackSecond =
                        bytePosition /
                        (static_cast<i64>(sampleSize * 2 * 44100));

                    if (previousSecond != playbackSecond) {
                        progressTimer->setPlainText(
                            (format("{}/{}",
                                    formatSecondsToMinutes(playbackSecond),
                                    duration))
                                .c_str());
                        progressSlider->setSliderPosition(
                            static_cast<u16>(playbackSecond));
                        previousSecond = playbackSecond;
                    }

                    previousPosition = bytePosition;
                }

                if (buffer->atEnd()) {
                    emit playbackFinished();
                }
            }
        }
    }
};

#endif  // PROGRESSMONITOR_HPP
