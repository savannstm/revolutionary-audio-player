#pragma once

#include "aliases.hpp"

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>

class MetadataWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MetadataWindow(
        const QMap<u8, QString>& metadata,
        QWidget* parent = nullptr
    );

   private:
    QTreeWidget treeWidget = QTreeWidget(this);
    QVBoxLayout layout = QVBoxLayout(this);
};