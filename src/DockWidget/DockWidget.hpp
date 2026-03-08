#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QSplitter>

class DockWidget : public QSplitter {
    Q_OBJECT

   public:
    enum class Position : u8 {
        Left,
        Top,
        Bottom,
        Right
    };

    explicit DockWidget(QWidget* parent = nullptr);

    void resizeEvent(QResizeEvent* const event) override {
        QSplitter::resizeEvent(event);
        emit resized();
    }

    QTreeWidget* dockMetadataTree;
    QLabel* dockCoverLabel;

   signals:
    void resized();
    void repositioned(Position newPosition);
};