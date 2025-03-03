#include "aboutwindow.h"
#include <qmainwindow.h>

AboutWindow::AboutWindow(QWidget *parent) : QMainWindow(parent) {
    ui->setupUi(this);
}
AboutWindow::~AboutWindow() { delete ui; }