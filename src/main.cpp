#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;

    window.showMaximized();

    window.show();
    return QApplication::exec();
}
