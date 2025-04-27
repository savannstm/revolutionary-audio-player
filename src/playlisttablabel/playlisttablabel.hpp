#pragma once

#include <QLineEdit>

class PlaylistTabLabel : public QLineEdit {
    Q_OBJECT

   public:
    explicit PlaylistTabLabel(const QString& text, QWidget* parent = nullptr);

   signals:
    void doubleClicked();
    void clicked();
    void rightClicked();
    void unfocused();

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
};