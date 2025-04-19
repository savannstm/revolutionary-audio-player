#pragma once

#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    using QTreeView::QTreeView;

    [[nodiscard]] auto currentIndex() const -> QModelIndex;
    void setCurrentIndex(const QModelIndex& newIndex);

   signals:
    void pressed(const QModelIndex& oldIndex, const QModelIndex& newIndex);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   private:
    QModelIndex index;
};