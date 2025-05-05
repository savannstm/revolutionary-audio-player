#pragma once

#include "constants.hpp"
#include "extractmetadata.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QDirIterator>
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

    [[nodiscard]] auto rowMetadata(u16 row) const -> MetadataMap;

    void processFile(const QString& filePath) {
        bool valid = false;

        for (QStringView extension : ALLOWED_FILE_EXTENSIONS) {
            if (filePath.endsWith(extension)) {
                valid = true;
                break;
            }
        }

        if (!valid) {
            return;
        }

        const MetadataMap metadata =
            extractMetadata(filePath.toStdString().c_str());

        const u16 row = musicModel->rowCount();

        for (u8 column = 0; column < TRACK_PROPERTY_COUNT; column++) {
            const u8 headerProperty = musicModel->trackProperty(column);
            auto* item = new MusicItem();

            if (headerProperty == TrackNumber) {
                QString number;

                for (const auto& [idx, chr] :
                     views::enumerate(metadata[headerProperty])) {
                    if (idx == 0 && chr == '0') {
                        continue;
                    }

                    if (!chr.isDigit()) {
                        break;
                    }

                    number.append(chr);
                }

                item->setData(number.toInt(), Qt::EditRole);
            } else if (headerProperty == Play) {
                item->setText(QString());
            } else {
                item->setText(metadata[headerProperty]);
            }

            musicModel->setItem(row, column, item);
        }
    }

    void fillTable(const QStringList& filePaths) {
        for (const QString& filePath : filePaths) {
            if (musicModel->contains(filePath)) {
                continue;
            }

            processFile(filePath);
        }

        for (u8 column = 0; column < TRACK_PROPERTY_COUNT; column++) {
            resizeColumnToContents(column);
        }
    }

    void fillTable(QDirIterator& iterator) {
        while (iterator.hasNext()) {
            iterator.next();
            const QFileInfo entry = iterator.fileInfo();
            const QString path = entry.filePath();

            if (musicModel->contains(path)) {
                continue;
            }

            processFile(path);
        }

        for (u8 column = 0; column < TRACK_PROPERTY_COUNT; column++) {
            resizeColumnToContents(column);
        }
    };

   signals:
    void trackSelected(u32 oldIndex, u32 newIndex);

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

   private:
    QModelIndex index;
    MusicHeader* musicHeader =
        new MusicHeader(Qt::Orientation::Horizontal, this);
    MusicModel* musicModel = new MusicModel(this);
};