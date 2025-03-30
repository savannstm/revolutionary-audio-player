#include <qapplication.h>
#include <qpalette.h>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>

#include "mainwindow.hpp"

auto main(int argc, char* argv[]) -> int {
    QApplication const app(argc, argv);
    QApplication::setStyle("fusion");

    auto* palette = new QPalette();
    palette->setColor(QPalette::ColorGroup::All, QPalette::ColorRole::Dark,
                      "dark");

    QApplication::setPalette(*palette);

    MainWindow window;
    window.showMaximized();

    return QApplication::exec();
}
