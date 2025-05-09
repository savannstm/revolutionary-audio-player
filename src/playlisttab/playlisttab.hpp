#pragma once

#include "aliases.hpp"
#include "enums.hpp"
#include "playlisttablabel.hpp"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>

class PlaylistTab : public QPushButton {
    Q_OBJECT

   public:
    explicit PlaylistTab(
        const QString& text,
        bool closable,
        QWidget* parent = nullptr
    );

    [[nodiscard]] auto label() const -> QString { return label_->text(); }

    void setLabel(const QString& label) { label_->setText(label); };

    constexpr auto labelPtr() -> PlaylistTabLabel* { return label_; };

   signals:
    void clicked();
    void rightClicked();
    void removeTabRequested();
    void addButtonClicked();
    void removeTabsRequested(TabRemoveMode mode);
    void exportPlaylistRequested(PlaylistFileType playlistType);
    void importPlaylistRequested(PlaylistFileType playlistType);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    inline auto labelTextWidth() -> i32;
    void handleMousePress();
    void createContextMenu();

    bool addTab = false;
    PlaylistTabLabel* label_;
    QToolButton* tabButton = new QToolButton(this);
    QHBoxLayout* layout_ = new QHBoxLayout(this);
};