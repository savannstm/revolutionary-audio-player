#pragma once

#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    explicit TrackTree();
    explicit TrackTree(QWidget* parent);

    void setCurrentIndex(const QModelIndex& newIndex);

    [[nodiscard]] constexpr auto currentIndex() const -> QModelIndex {
        return index;
    };

    [[nodiscard]] constexpr auto model() const -> MusicModel* {
        return musicModel;
    };

    [[nodiscard]] constexpr auto header() const -> MusicHeader* {
        return musicHeader;
    };

   signals:
    void
    trackSelected(const QModelIndex& oldIndex, const QModelIndex& newIndex);

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

   private:
    QModelIndex index;
    MusicHeader* musicHeader =
        new MusicHeader(Qt::Orientation::Horizontal, this);
    MusicModel* musicModel = new MusicModel(this);
};