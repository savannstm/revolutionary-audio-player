#pragma once

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem() { setEditable(false); };

    explicit MusicItem(const QString& text) : QStandardItem(text) {
        setEditable(false);
    };

    constexpr void setPath(const QString& path) { path_ = path; };

    [[nodiscard]] constexpr auto path() const -> const QString& {
        return path_;
    };

   private:
    QString path_;
};