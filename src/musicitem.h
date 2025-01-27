#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    MusicItem(const char *title) : QStandardItem(title) {}

    void setPath(const char *pathStr) { path = pathStr; }
    auto getPath() -> const char * { return path.c_str(); }

   private:
    std::string path;
};