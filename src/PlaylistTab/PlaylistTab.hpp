#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QPushButton>

class PlaylistTab : public QPushButton {
    Q_OBJECT

   public:
    explicit PlaylistTab(
        const QString& text,
        bool closable,
        QWidget* parent = nullptr
    );

    [[nodiscard]] auto labelText() const -> QString;

    void setLabelText(const QString& label);

    [[nodiscard]] constexpr auto label() const -> PlaylistTabLabel* {
        return label_;
    };

    [[nodiscard]] constexpr auto color() const -> const QString& {
        return color_;
    }

    void setColor(const QString& color);

   signals:
    void clicked();
    void removeTabRequested();
    void addButtonClicked();
    void removeTabsRequested(TabRemoveMode mode);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    auto eventFilter(QObject* obj, QEvent* event) -> bool override;

   private:
    inline auto labelTextWidth() -> i32;
    inline void selectTab();
    inline void createContextMenu();
    inline void deselectLabel();
    inline void grab();

    PlaylistTabLabel* const label_ = nullptr;

    QToolButton* const tabButton;
    QHBoxLayout* const layout_;

    QPoint dragStartPos;
    QString color_;

    bool dragging = false;
    bool addTab = false;
};