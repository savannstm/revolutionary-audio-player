#include "Logger.hpp"

#include <QApplication>
#include <QDebug>
#include <QIODevice>
#include <QMessageBox>
#include <QTime>

void Logger::init() {
    if (!logFile.isOpen()) {
        logFile.setFileName(
            QApplication::applicationDirPath() + u"/rap.log"_qssv
        );

        if (!logFile.open(
                QIODevice::Append | QIODevice::Text | QIODevice::Truncate
            )) {
            QMessageBox::critical(
                nullptr,
                QObject::tr("Failed to initialize logger"),
                QObject::tr(
                    "Logs won't be written to `rap.log` file: %1. Good luck out there."
                )
                    .arg(logFile.errorString())
            );
        };

        stream.setDevice(&logFile);
        stream.setEncoding(QStringConverter::Utf8);
    }
}

void Logger::log(
    const LogLevel level,
    const cstr file,
    const i32 line,
    const cstr func,
    const QString& msg
) {
#ifdef DEBUG_BUILD
    qDebug() << file << line << func << msg;
#elifdef RELEASE_BUILD
    QMutexLocker locker(&mutex);
    init();

    QString loggingLevelString;
    switch (level) {
        case LogLevel::Info:
            loggingLevelString = u"INFO"_s;
            break;
        case LogLevel::Warning:
            loggingLevelString = u"WARNING"_s;
            break;
        case LogLevel::Error:
            loggingLevelString = u"ERROR"_s;
            break;
    }

    const QString timestamp =
        QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm:ss"_s);
    stream << loggingLevelString << ' ' << timestamp << ' ' << file << ' '
           << line << ' ' << func << ' ' << '"' << msg << '"' << '\n';
    stream.flush();
#endif
}
