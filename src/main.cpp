#include "Constants.hpp"
#include "MainWindow.hpp"

#include <QApplication>
#include <QLocalSocket>
#include <QSharedMemory>

auto main(i32 argCount, char* args[]) -> i32 {
    const auto app = QApplication(argCount, args);

    QSharedMemory sharedMemory;
    sharedMemory.setKey("revolutionary-audio-player");

    const QStringList paths = QApplication::arguments().mid(1);

    if (!sharedMemory.create(1)) {
        QLocalSocket socket;
        socket.connectToServer(u"revolutionary-audio-player-server"_s);

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

    QApplication::setOrganizationName(u"savannstm"_s);
    QApplication::setApplicationName(u"revolutionary-audio-player"_s);
    QApplication::setWindowIcon(
        QIcon(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );

    MainWindow window(paths);
    window.showMaximized();

    return QApplication::exec();
}
