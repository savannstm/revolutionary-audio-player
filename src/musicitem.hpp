// local
#include "type_aliases.hpp"

// qt
#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    using QStandardItem::QStandardItem;

    void setPath(cstr pathStr) { path = pathStr; }
    void setPath(const path &fsPath) { path = fsPath; }

    [[nodiscard]] auto getPath() const -> auto { return path; }

   private:
    path path;
};