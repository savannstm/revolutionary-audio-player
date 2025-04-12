#include "aboutwindow.hpp"

#include <QMainWindow>

AboutWindow::AboutWindow(QWidget* parent) : QDialog(parent) {
    ui->setupUi(this);
}

AboutWindow::~AboutWindow() {
    delete ui;
}