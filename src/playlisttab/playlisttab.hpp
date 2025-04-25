#pragma once

#include "aliases.hpp"
#include "playlisttablabel.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>

constexpr QSize CLOSE_BUTTON_SIZE = QSize(16, 16);

class PlaylistTab : public QPushButton {
    Q_OBJECT

   public:
    explicit PlaylistTab(
        const QString& text,
        bool closable,
        QWidget* parent = nullptr
    );

    [[nodiscard]] auto label() const -> QString { return label_->text(); }

    [[nodiscard]] constexpr auto button() const -> QToolButton* {
        return tabButton;
    }

   signals:
    void clicked();
    void closeButtonClicked();
    void addButtonClicked();

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    inline auto labelTextWidth() -> i32;
    void handleMousePress();

    bool addTab = false;
    PlaylistTabLabel* label_;
    QToolButton* tabButton = new QToolButton(this);
    QHBoxLayout* layout_ = new QHBoxLayout(this);
};