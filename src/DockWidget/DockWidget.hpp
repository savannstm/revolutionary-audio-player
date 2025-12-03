#pragma once

#include "Enums.hpp"
#include "FWD.hpp"

#include <QSplitter>

class DockWidget : public QSplitter {
    Q_OBJECT

   public:
    explicit DockWidget(QWidget* parent = nullptr);

    void resizeEvent(QResizeEvent* const event) override {
        QSplitter::resizeEvent(event);
        emit resized();
    }

    QTreeWidget* dockMetadataTree;
    QLabel* dockCoverLabel;

   signals:
    void resized();
    void repositioned(DockWidgetPosition);
};