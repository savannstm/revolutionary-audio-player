#pragma once

#include "aliases.hpp"
#include "playlisttablabel.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>

constexpr QSize CLOSE_BUTTON_SIZE = QSize(16, 16);

enum RemoveMode : u8 {
    ToLeft,
    ToRight,
    Other
};

class PlaylistTab : public QPushButton {
    Q_OBJECT

   public:
    explicit PlaylistTab(
        const QString& text,
        bool closable,
        QWidget* parent = nullptr
    );

    [[nodiscard]] auto label() const -> QString { return label_.text(); }

    auto labelPtr() -> PlaylistTabLabel*;

   signals:
    void clicked();
    void removeTabRequested();
    void addButtonClicked();
    void removeTabs(RemoveMode mode);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    inline auto labelTextWidth() -> i32;
    void handleMousePress();

    bool addTab = false;
    PlaylistTabLabel label_;
    QToolButton tabButton = QToolButton(this);
    QHBoxLayout layout_ = QHBoxLayout(this);
};