#pragma once

#include "Aliases.hpp"

#include <QFile>
#include <QMutex>
#include <QTextStream>

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

    static void init();

   public:
    static void
    log(LogLevel level, cstr file, i32 line, cstr func, const QString& msg);
};

#define LOG_INFO(msg) \
    Logger::log(LogLevel::Info, __FILE__, __LINE__, __func__, msg)
#define LOG_WARN(msg) \
    Logger::log(LogLevel::Warning, __FILE__, __LINE__, __func__, msg)
#define LOG_ERROR(msg) \
    Logger::log(LogLevel::Error, __FILE__, __LINE__, __func__, msg)
