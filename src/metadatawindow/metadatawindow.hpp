#pragma once

#include "aliases.hpp"

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>

// TODO: Resize to correct size

class MetadataWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MetadataWindow(
        const metadata_array& metadata,
        QWidget* parent = nullptr
    );

   private:
    QTreeWidget treeWidget = QTreeWidget(this);
    QVBoxLayout layout = QVBoxLayout(this);
};