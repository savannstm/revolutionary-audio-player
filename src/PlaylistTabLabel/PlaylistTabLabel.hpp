#pragma once

#include <QLineEdit>

class PlaylistTabLabel : public QLineEdit {
    Q_OBJECT

   public:
    explicit PlaylistTabLabel(const QString& text, QWidget* parent = nullptr);

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};