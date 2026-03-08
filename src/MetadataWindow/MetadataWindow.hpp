#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

class MetadataWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MetadataWindow(
        const TrackMetadata& metadata,
        QWidget* parent = nullptr
    );

   private:
    static constexpr QSize METADATA_WINDOW_SIZE = QSize(800, 600);

    QTreeWidget* const treeWidget;
    QVBoxLayout* const layout;
};