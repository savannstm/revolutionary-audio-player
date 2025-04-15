#pragma once

#include "aliases.hpp"

#include <QHeaderView>
#include <QMouseEvent>

class MusicHeader : public QHeaderView {
    Q_OBJECT

   public:
    explicit MusicHeader(
        Qt::Orientation orientation,
        QWidget* parent = nullptr
    );

   signals:
    void headerPressed(i32 sectionIndex, Qt::MouseButton button);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
};