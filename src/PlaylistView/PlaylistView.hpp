#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "PlaylistTabBar.hpp"
#include "Settings.hpp"
#include "TrackTree.hpp"

#include <QDir>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistView(QWidget* parent = nullptr);

    void setSettings(shared_ptr<Settings> settings_) {
        settings = std::move(settings_);
    }

    void setBackgroundImage(
        u8 index,
        const QImage& image,
        const QString& path
    ) const;

    void setCurrentIndex(const i8 index) { tabBar_->setCurrentIndex(index); };

    void setTabColor(const u8 index, const QString& color) {
        tabBar_->setTabColor(index, color);
    };

    void setTabLabel(const u8 index, const QString& label) {
        tabBar_->setTabLabel(index, label);
    };

    [[nodiscard]] constexpr auto tabBar() const -> PlaylistTabBar* {
        return tabBar_;
    }

    [[nodiscard]] auto tabCount() const -> u8 { return tabBar_->tabCount(); };

    [[nodiscard]] auto currentIndex() const -> i8 {
        return tabBar_->currentIndex();
    };

    [[nodiscard]] auto backgroundImage(const u8 index) const -> QLabel* {
        return stackedWidget->widget(index)->findChild<QLabel*>(
            u"centerLabel"_qsv
        );
    };

    [[nodiscard]] auto currentBackgroundImage() const -> QLabel* {
        return stackedWidget->currentWidget()->findChild<QLabel*>(
            u"centerLabel"_qsv
        );
    };

    [[nodiscard]] auto tree(const u8 index) const -> TrackTree* {
        const QWidget* const widget = stackedWidget->widget(index);

        if (widget == nullptr) {
            return nullptr;
        }

        return widget->findChild<TrackTree*>(u"tree"_qsv);
    };

    [[nodiscard]] auto currentTree() const -> TrackTree* {
        return stackedWidget->currentWidget()->findChild<TrackTree*>(
            u"tree"_qsv
        );
    };

    [[nodiscard]] auto tabLabel(const u8 index) const -> QString {
        return tabBar_->tabLabel(index);
    };

    [[nodiscard]] auto tabColor(const u8 index) const -> QString {
        return tabBar_->tabColor(index);
    };

    [[nodiscard]] auto currentPage() const -> QWidget* {
        return stackedWidget->currentWidget();
    };

    [[nodiscard]] auto page(const u8 index) const -> QWidget* {
        return stackedWidget->widget(index);
    };

    [[nodiscard]] auto backgroundImagePath(const u8 index) const -> QString {
        const QLabel* const centerLabel = backgroundImage(index);
        return centerLabel->property("path").toString();
    };

    [[nodiscard]] auto createPage(
        optional<array<TrackProperty, TRACK_PROPERTY_COUNT>> defaultColumns =
            nullopt
    ) -> QWidget*;

    [[nodiscard]] auto addTab(
        const QString& label = QString(),
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& defaultColumns =
            DEFAULT_COLUMN_PROPERTIES
    ) -> u8 {
        const u8 index =
            u8(stackedWidget->addWidget(createPage(defaultColumns)));
        tabBar_->addTab(label);
        return index;
    };

    void removeBackgroundImage(u8 index) const;
    void removePages(TabRemoveMode mode, u8 index);

    void createTabPage(const u8 index) {
        if (stackedWidget->widget(index) != nullptr) {
            return;
        }

        stackedWidget->insertWidget(index, createPage());
    };

    [[nodiscard]] auto hasBackgroundImage(const u8 index) const -> bool {
        const QWidget* const widget = stackedWidget->widget(index);

        if (widget == nullptr) {
            return false;
        }

        const QLabel* const centerLabel = backgroundImage(index);
        return centerLabel->property("path").isValid();
    };

    void removePage(const u8 index) {
        QWidget* const widget = stackedWidget->widget(index);
        stackedWidget->removeWidget(widget);
        delete widget;
    };

   signals:
    void renameTabRequested(u8 index);
    void closeTabRequested(u8 index);
    void indexChanged(i8 index);
    void tabsRemoved(TabRemoveMode mode, u8 startIndex, u8 count);

   private:
    void setTreeOpacity(u8 index, f32 opacity) const;

    [[nodiscard]] auto treeOpacity(u8 index) const -> f32;

    void changePage(const i8 index) {
        stackedWidget->setCurrentIndex(index);
        emit indexChanged(index);
    };

    shared_ptr<Settings> settings;
    PlaylistTabBar* const tabBar_ = new PlaylistTabBar(this);
    QStackedWidget* const stackedWidget = new QStackedWidget(this);
    QVBoxLayout* const layout = new QVBoxLayout(this);
};