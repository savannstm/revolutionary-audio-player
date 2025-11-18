#pragma once

#include "Enums.hpp"
#include "RapidHasher.hpp"

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
    QTreeWidget* const treeWidget = new QTreeWidget(this);
    QVBoxLayout* const layout = new QVBoxLayout(this);
};