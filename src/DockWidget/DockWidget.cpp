#include "DockWidget.hpp"

#include "Enums.hpp"

#include <QMenu>

DockWidget::DockWidget(QWidget* const parent) : QSplitter(parent) {
    connect(
        this,
        &DockWidget::customContextMenuRequested,
        this,
        [=, this] -> void {
        auto* const menu = new QMenu(this);

        const QAction* const moveToLeftAction =
            menu->addAction(tr("Move To Left"));
        const QAction* const moveToRightAction =
            menu->addAction(tr("Move To Right"));
        const QAction* const moveToTopAction =
            menu->addAction(tr("Move To Top"));
        const QAction* const moveToBottomAction =
            menu->addAction(tr("Move To Bottom"));

        const bool dockCoverLabelHidden = dockCoverLabel->isHidden();
        QAction* const showCoverAction = menu->addAction(tr("Show Cover"));
        showCoverAction->setCheckable(true);
        showCoverAction->setChecked(!dockCoverLabelHidden);

        const bool dockMetadataTreeHidden = dockMetadataTree->isHidden();
        QAction* const showMetadataAction =
            menu->addAction(tr("Show Metadata"));
        showMetadataAction->setCheckable(true);
        showMetadataAction->setChecked(!dockMetadataTreeHidden);

        const QAction* const selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == moveToLeftAction) {
            emit repositioned(DockWidgetPosition::Left);
        } else if (selectedAction == moveToRightAction) {
            emit repositioned(DockWidgetPosition::Right);
        } else if (selectedAction == moveToTopAction) {
            emit repositioned(DockWidgetPosition::Top);
        } else if (selectedAction == moveToBottomAction) {
            emit repositioned(DockWidgetPosition::Bottom);
        } else if (selectedAction == showCoverAction) {
            if (dockCoverLabelHidden) {
                dockCoverLabel->show();
            } else if (!dockMetadataTreeHidden) {
                dockCoverLabel->hide();
            }
        } else if (selectedAction == showMetadataAction) {
            if (dockMetadataTreeHidden) {
                dockMetadataTree->show();
            } else if (!dockCoverLabelHidden) {
                dockMetadataTree->hide();
            }
        }
    }
    );
}