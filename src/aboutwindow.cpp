#include "aboutwindow.hpp"

#include <QMainWindow>

AboutWindow::AboutWindow(QWidget *parent) : QMainWindow(parent) {
    ui->setupUi(this);
}
AboutWindow::~AboutWindow() { delete ui; }