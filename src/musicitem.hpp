#include <QStandardItem>
#include <filesystem>

class MusicItem : public QStandardItem {
   public:
    using QStandardItem::QStandardItem;

    void setPath(const char *pathStr) { path = pathStr; }
    void setPath(const std::filesystem::path &fsPath) {
        path = fsPath.string();
    }

    [[nodiscard]] auto getPath() const -> const char * { return path.c_str(); }

   private:
    std::string path;
};