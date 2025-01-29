#include <QApplication>
#include <QGuiApplication>
#include <QPalette>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication const app(argc, argv);

#ifdef _WIN32
    QApplication::setStyle("fusion");
#endif

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
