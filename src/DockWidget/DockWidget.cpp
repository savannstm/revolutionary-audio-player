#include "DockWidget.hpp"

#include <QMenu>

DockWidget::DockWidget(QWidget* parent) : QSplitter(parent) {
    connect(
        this,
        &DockWidget::customContextMenuRequested,
        this,
        [=, this] -> void {
        auto* menu = new QMenu(this);

        const QAction* moveToLeftAction = menu->addAction(tr("Move To Left"));
        const QAction* moveToRightAction = menu->addAction(tr("Move To Right"));
        const QAction* moveToTopAction = menu->addAction(tr("Move To Top"));
        const QAction* moveToBottomAction =
            menu->addAction(tr("Move To Bottom"));

        const bool dockCoverLabelHidden = dockCoverLabel->isHidden();
        QAction* showCoverAction = menu->addAction(tr("Show Cover"));
        showCoverAction->setCheckable(true);
        showCoverAction->setChecked(!dockCoverLabelHidden);

        const bool dockMetadataTreeHidden = dockMetadataTree->isHidden();
        QAction* showMetadataAction = menu->addAction(tr("Show Metadata"));
        showMetadataAction->setCheckable(true);
        showMetadataAction->setChecked(!dockMetadataTreeHidden);

        const QAction* selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == moveToLeftAction) {
            emit repositioned(Left);
        } else if (selectedAction == moveToRightAction) {
            emit repositioned(Right);
        } else if (selectedAction == moveToTopAction) {
            emit repositioned(Top);
        } else if (selectedAction == moveToBottomAction) {
            emit repositioned(Bottom);
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