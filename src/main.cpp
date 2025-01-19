#include "mainwindow.h"

#include <QApplication>

int sum(int a, int b) { return a + b; }

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow main_window;

    main_window.setWindowTitle("Qt6 MainWindow Example");
    main_window.resize(800, 600);
    main_window.show();

    main_window.show();
    return a.exec();
}
