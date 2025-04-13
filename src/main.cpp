#include "mainwindow.hpp"

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QSharedMemory>
#include <QStyleFactory>

auto main(int argc, char* argv[]) -> int {
    std::locale::global(std::locale(".UTF-8"));

    const auto app = QApplication(argc, argv);

    QSharedMemory sharedMemory;
    sharedMemory.setKey("com.savannstm.rap");

    if (!sharedMemory.create(1)) {
        return 1;
    }

    qDebug() << QStyleFactory::keys();
    if (QStyleFactory::keys().contains("Fusion")) {
        QApplication::setStyle("fusion");
    }

    auto* palette = new QPalette();
    palette->setColor(
        QPalette::ColorGroup::All,
        QPalette::ColorRole::Dark,
        "dark"
    );

    QApplication::setPalette(*palette);

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
