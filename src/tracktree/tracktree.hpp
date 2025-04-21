#pragma once

#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    explicit TrackTree();
    explicit TrackTree(QWidget* parent);

    [[nodiscard]] auto currentIndex() const -> QModelIndex;
    void setCurrentIndex(const QModelIndex& newIndex);

    auto model() -> MusicModel*;
    auto header() -> MusicHeader*;

   signals:
    void pressed(const QModelIndex& oldIndex, const QModelIndex& newIndex);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    QModelIndex index;
    MusicHeader* musicHeader;
    MusicModel* musicModel;
};