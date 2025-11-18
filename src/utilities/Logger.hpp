#pragma once

#include "Aliases.hpp"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QMessageBox>
#include <QMutex>
#include <QTextStream>
#include <QTime>

enum LogLevel : u8 {
    Info,
    Warning,
    Error
};

class Logger {
   private:
    static inline QFile logFile;
    static inline QTextStream stream;
    static inline QMutex mutex;

    static void init() {
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

   public:
    static void log(
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
};

#define LOG_INFO(msg) \
    Logger::log(LogLevel::Info, __FILE__, __LINE__, __func__, msg)
#define LOG_WARN(msg) \
    Logger::log(LogLevel::Warning, __FILE__, __LINE__, __func__, msg)
#define LOG_ERROR(msg) \
    Logger::log(LogLevel::Error, __FILE__, __LINE__, __func__, msg)
