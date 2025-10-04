#include "CoverWindow.hpp"

#include "Constants.hpp"
#include "ExtractMetadata.hpp"
#include "Log.hpp"

#include <QMenu>
#include <QPixmap>

CoverWindow::CoverWindow(
    const QString& coverPath,  // NOLINT
    const QString& title,
    QWidget* parent
) :
    QDialog(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setWindowTitle(tr("%1: Cover").arg(title));
    setMinimumSize(MINIMUM_COVER_SIZE);

    setStyleSheet(u"CoverWindow { background-color: black; }"_s);

    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins({ 0, 0, 0, 0 });
    layout->addWidget(coverLabel);

    connect(
        this,
        &QDialog::customContextMenuRequested,
        this,
        &CoverWindow::showContextMenu
    );

    updateCover(coverPath);
}

void CoverWindow::updateCover(const QString& coverPath) {
    const auto extracted = extractCover(coverPath.toUtf8().constData());

    if (!extracted) {
        LOG_WARN(extracted.error());
        return;
    }

    const vector<u8>& coverBytes = extracted.value();
    originalPixmap.loadFromData(coverBytes.data(), coverBytes.size());
    coverLabel->setPixmap(originalPixmap);
    resize(originalPixmap.size());
}

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
            setWindowFlag(Qt::FramelessWindowHint, false);
            showNormal();
            resize(originalPixmap.size());

            const QScreen* currentScreen = screen();

            if (currentScreen == nullptr) {
                currentScreen = QGuiApplication::primaryScreen();
            }

            const QRect screenGeometry = currentScreen->availableGeometry();
            const QSize windowSize = size();

            const i32 xPos =
                screenGeometry.x() +
                ((screenGeometry.width() - windowSize.width()) / 2);
            const i32 yPos =
                screenGeometry.y() +
                ((screenGeometry.height() - windowSize.height()) / 2);

            move(xPos, yPos);
        } else {
            setWindowFlag(Qt::FramelessWindowHint, true);
            showFullScreen();
        }
    } else if (selectedAction == alwaysOnTopAction) {
        setWindowFlag(Qt::WindowStaysOnTopHint, !isOnTop);
        show();
    }
}
