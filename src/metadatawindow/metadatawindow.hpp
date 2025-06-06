#pragma once

#include "enums.hpp"
#include "rapidhasher.hpp"

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>

class MetadataWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MetadataWindow(
        const HashMap<TrackProperty, QString>& metadata,
        QWidget* parent = nullptr
    );

   private:
    QTreeWidget* treeWidget = new QTreeWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
};