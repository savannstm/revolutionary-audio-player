#define MINIAUDIO_IMPLEMENTATION

#include "Constants.hpp"
#include "MainWindow.hpp"

#include <QApplication>
#include <QLocalSocket>
#include <QLockFile>

auto main(i32 argCount, char* args[]) -> i32 {
    const auto app = QApplication(argCount, args);

    const QString lockFilePath =
        QDir::tempPath() + u"/revolutionary-audio-player.lock"_qssv;

    auto lockFile = QLockFile(lockFilePath);
    const QStringList paths = QApplication::arguments().mid(1);

    if (!lockFile.tryLock()) {
        if (!paths.empty()) {
            QLocalSocket socket;
            socket.connectToServer(u"revolutionary-audio-player-server"_s);

            if (socket.waitForConnected(SECOND_MS)) {
                QByteArray data = paths.join('\n').toUtf8();

                socket.write(data);
                socket.flush();
                socket.waitForBytesWritten(SECOND_MS);
                socket.disconnectFromServer();
            }
        }

        return 0;
    }

    // Use system default locale
    std::locale::global(std::locale(""));

    QApplication::setOrganizationName(u"savannstm"_s);
    QApplication::setApplicationName(u"revolutionary-audio-player"_s);
    QApplication::setWindowIcon(QIcon(
        QApplication::applicationDirPath() + '/' +
        PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    ));

    QApplication::connect(&app, &QApplication::aboutToQuit, [&] -> void {
        lockFile.unlock();
        QFile::remove(lockFilePath);
    });

    auto window = MainWindow(paths);
    window.showMaximized();

    return QApplication::exec();
}
