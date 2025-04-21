#include "mainwindow.hpp"

#include <QApplication>
#include <QSharedMemory>

auto main(int argc, char* argv[]) -> int {
    std::locale::global(std::locale(".UTF-8"));

    const auto app = QApplication(argc, argv);
    QApplication::setOrganizationName("savannstm");
    QApplication::setApplicationName("revolutionary-audio-player");

    QSharedMemory sharedMemory;
    sharedMemory.setKey("com.savannstm.rap");

    if (!sharedMemory.create(1)) {
        return 1;
    }

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
