#pragma once

#include "constants.hpp"

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>

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
    void populateTree(const metadata_array& metadata);
    static auto propertyToLabel(TrackProperty prop) -> QString;
};