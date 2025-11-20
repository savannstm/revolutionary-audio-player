#pragma once

#include <QStandardItem>

class TrackTreeItem : public QStandardItem {
   public:
    explicit TrackTreeItem(const QString& text = QString()) :
        QStandardItem(text) {
        setEditable(false);
    };
};