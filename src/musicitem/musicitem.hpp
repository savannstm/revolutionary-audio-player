#pragma once

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem() { setEditable(false); };

    explicit MusicItem(const QString& text) : QStandardItem(text) {
        setEditable(false);
    };
};