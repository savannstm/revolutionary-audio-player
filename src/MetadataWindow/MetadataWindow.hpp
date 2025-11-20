#pragma once

#include "Aliases.hpp"

#include <QDialog>
#include <QTreeWidget>
#include <QVBoxLayout>

class MetadataWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MetadataWindow(
        const TrackMetadata& metadata,
        QWidget* parent = nullptr
    );

   private:
    QTreeWidget* const treeWidget = new QTreeWidget(this);
    QVBoxLayout* const layout = new QVBoxLayout(this);
};