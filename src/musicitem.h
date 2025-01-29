#include <qstandarditemmodel.h>

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    MusicItem(const char *title) : QStandardItem(title) {}
    MusicItem() : QStandardItem() {};

    void setPath(const char *pathStr) { path = pathStr; }
    [[nodiscard]] auto getPath() const -> const char * { return path.c_str(); }

   private:
    std::string path;
};