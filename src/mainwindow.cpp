#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    // Set up the central widget
    QTextEdit *textEdit = new QTextEdit(this);
    setCentralWidget(textEdit);

    // Add a menu bar
    QMenu *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Open");
    fileMenu->addAction("Save");
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &MainWindow::close);

    // Add a toolbar
    QToolBar *toolbar = new QToolBar("Main Toolbar", this);
    addToolBar(toolbar);
    toolbar->addAction("Copy");
    toolbar->addAction("Paste");

    // Add a status bar
    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");

    // Add a dockable widget
    QDockWidget *dock = new QDockWidget("Dock Widget", this);
    QLabel *dockLabel = new QLabel("This is a dockable widget", dock);
    dock->setWidget(dockLabel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

MainWindow::~MainWindow() { delete ui; }
