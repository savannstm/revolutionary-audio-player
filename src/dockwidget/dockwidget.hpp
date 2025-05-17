#pragma once

#include "enums.hpp"

#include <QLabel>
#include <QResizeEvent>
#include <QSplitter>
#include <QTreeWidget>

class DockWidget : public QSplitter {
    Q_OBJECT

   public:
    explicit DockWidget(QWidget* parent = nullptr);
    QTreeWidget* dockMetadataTree;
    QLabel* dockCoverLabel;

    void resizeEvent(QResizeEvent* event) override {
        QSplitter::resizeEvent(event);
        emit resized();
    }

   signals:
    void resized();
    void repositioned(DockWidgetPosition);
};