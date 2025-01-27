#include <QApplication>
#include <QGuiApplication>
#include <QPalette>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication const app(argc, argv);
    QApplication::setStyle("fusion");

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
