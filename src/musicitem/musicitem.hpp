#pragma once

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem(const QString& text = QString()) : QStandardItem(text) {
        setEditable(false);
    };
};