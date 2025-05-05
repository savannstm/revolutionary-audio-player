#include "coverwindow.hpp"

#include "constants.hpp"
#include "extractmetadata.hpp"

#include <QMenu>
#include <QPixmap>

// TODO: Fix maximizing to fullscreen
void CoverWindow::showContextMenu(const QPoint& pos) {
    auto* menu = new QMenu(this);

    const bool isFullscreen = isFullScreen();
    const bool isOnTop = (windowFlags() & Qt::WindowStaysOnTopHint) != 0;

    const QAction* maximizeAction;

    if (isFullscreen) {
        maximizeAction = menu->addAction(tr("Minimize"));
    } else {
        maximizeAction = menu->addAction(tr("Maximize To Fullscreen"));
    };

    const QAction* alwaysOnTopAction;

    if (isOnTop) {
        alwaysOnTopAction = menu->addAction(tr("Unset Always On Top"));
    } else {
        alwaysOnTopAction = menu->addAction(tr("Set Always On Top"));
    }

    const QAction* selectedAction = menu->exec(mapToGlobal(pos));
    delete menu;

    if (selectedAction == maximizeAction) {
        if (isFullscreen) {
            showNormal();
            resize(image->pixmap().size());
            setMaximumSize(image->pixmap().size());
            image->setScaledContents(true);
        } else {
            setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            image->setScaledContents(false);
            image->resize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            showFullScreen();
        }
    } else if (selectedAction == alwaysOnTopAction) {
        setWindowFlag(Qt::WindowStaysOnTopHint, !isOnTop);
        show();
    }
}

CoverWindow::CoverWindow(
    const QString& coverPath,
    const QString& title,
    QWidget* parent
) :
    QDialog(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setWindowTitle(tr("%1: Cover").arg(title));
    setMinimumSize(MINIMUM_COVER_SIZE);
    layout->addWidget(image);

    connect(
        this,
        &QDialog::customContextMenuRequested,
        this,
        &CoverWindow::showContextMenu
    );

    updateCover(coverPath);
}

void CoverWindow::updateCover(const QString& coverPath) {
    const vector<u8> coverBytes = extractCover(coverPath.toUtf8().constData());

    QPixmap pixmap;
    pixmap.loadFromData(coverBytes.data(), coverBytes.size());

    image->setPixmap(pixmap);
    image->setScaledContents(true);
    image->setAlignment(Qt::AlignCenter);

    setMaximumSize(image->pixmap().size());
    resize(image->pixmap().size());
}