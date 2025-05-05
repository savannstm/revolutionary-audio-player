#include "mainwindow.hpp"

#include <QApplication>
#include <QPixmapCache>
#include <QSharedMemory>

auto main(int argc, char* argv[]) -> int {
    const auto app = QApplication(argc, argv);

    // TODO: Opening files from system menu with player
    // TODO: Context menu option of player

    QSharedMemory sharedMemory;
    sharedMemory.setKey("revolutionary-audio-player");

    if (!sharedMemory.create(1)) {
        return 1;
    }

    std::locale::global(std::locale(".UTF-8"));

    QApplication::setOrganizationName("savannstm");
    QApplication::setApplicationName("revolutionary-audio-player");
    QApplication::setWindowIcon(
        QIcon(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
