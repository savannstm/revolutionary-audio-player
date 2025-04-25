#pragma once

#include "aliases.hpp"
#include "playlisttab.hpp"

#include <QHBoxLayout>
#include <QWidget>

// TODO: Implement tab dragging

class PlaylistTabBar : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistTabBar(QWidget* parent = nullptr);
    ~PlaylistTabBar() override;

    void addTab(const QString& label);
    void insertTab(i8 index, const QString& label, bool closable);
    void removeTab(i8 index);
    void setCurrentIndex(i8 index);
    [[nodiscard]] auto currentIndex() const -> i8;
    [[nodiscard]] auto count() const -> i8;
    [[nodiscard]] auto tabText(i8 index) const -> QString;

   signals:
    void indexChanged(i8 index);
    void tabAdded(i8 index);
    void closeButtonClicked(i8 index);
    void addButtonClicked(i8 index);

   private:
    void handleTabClicked();

    QHBoxLayout* layout = new QHBoxLayout(this);
    QVector<PlaylistTab*> tabs;
    i8 currentIndex_ = -1;
};
