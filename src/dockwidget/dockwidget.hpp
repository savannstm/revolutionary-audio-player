#pragma once

#include "enums.hpp"

#include <QLabel>
#include <QSplitter>
#include <QTreeWidget>

class DockWidget : public QSplitter {
    Q_OBJECT

   public:
    explicit DockWidget(QWidget* parent = nullptr);
    QTreeWidget* dockMetadataTree;
    QLabel* dockCoverLabel;

   signals:
    void repositioned(DockWidgetPosition);
};