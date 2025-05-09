#pragma once

#include "constants.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QDirIterator>
#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    explicit TrackTree(QWidget* parent = nullptr);

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

    [[nodiscard]] auto rowMetadata(u16 row) const -> MetadataMap;
    void sortByPath();
    void fillTable(const QStringList& paths);
    void fillTable(QDirIterator& iterator);

   signals:
    void trackSelected(u32 oldIndex, u32 newIndex);

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

   private:
    inline void addFile(const QString& filePath);

    QModelIndex index;
    MusicHeader* musicHeader =
        new MusicHeader(Qt::Orientation::Horizontal, this);
    MusicModel* musicModel = new MusicModel(this);
};