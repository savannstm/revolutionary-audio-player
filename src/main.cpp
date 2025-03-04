#include <QApplication>
#include <QGuiApplication>
#include <QPalette>

#include "mainwindow.hpp"

auto main(int argc, char* argv[]) -> int {
    QApplication const app(argc, argv);

    QApplication::setStyle("breeze");

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
