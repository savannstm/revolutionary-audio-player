#include "constants.hpp"
#include "mainwindow.hpp"

#include <QApplication>
#include <QLocalSocket>
#include <QSharedMemory>

inline auto parseArgs(const i32 argCount, std::span<char*> args)
    -> QStringList {
    QStringList paths;

    if (argCount > 1) {
        paths.reserve(argCount);

        for (const u32 idx : range(1, argCount)) {
            paths.append(args[idx]);
        }
    }

    return paths;
}

auto main(i32 argCount, char* args[]) -> i32 {
    const auto app = QApplication(argCount, args);

    QSharedMemory sharedMemory;
    sharedMemory.setKey("revolutionary-audio-player");

    QStringList paths = parseArgs(argCount, std::span(args, argCount));

    if (!sharedMemory.create(1)) {
        QLocalSocket socket;
        socket.connectToServer("revolutionary-audio-player-server");

        if (socket.waitForConnected(SECOND_MS)) {
            if (argCount > 1) {
                QByteArray data = paths.join('\n').toUtf8();
                socket.write(data);
                socket.flush();
                socket.waitForBytesWritten(SECOND_MS);
            }

            socket.disconnectFromServer();
        }

        return 0;
    }

    std::locale::global(std::locale(".UTF-8"));

    QApplication::setOrganizationName("savannstm");
    QApplication::setApplicationName("revolutionary-audio-player");
    QApplication::setWindowIcon(
        QIcon(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );

    MainWindow window(paths);
    window.showMaximized();

    return QApplication::exec();
}
