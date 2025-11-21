#include "CoverWindow.hpp"

#include "Constants.hpp"
#include "ExtractMetadata.hpp"
#include "Logger.hpp"

#include <QMenu>
#include <QPixmap>

CoverWindow::CoverWindow(
    const QString& coverPath,  // NOLINT
    const QString& title,
    QWidget* const parent
) :
    QDialog(parent, Qt::Window) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setWindowTitle(title + tr(": Cover"));
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
    auto* const menu = new QMenu(this);

    const bool isFullscreen = isFullScreen();
    const bool isOnTop = (windowFlags() & Qt::WindowStaysOnTopHint) != 0;

    const QAction* maximizeAction = menu->addAction(
        isFullscreen ? tr("Minimize") : tr("Maximize To Fullscreen")
    );

    const QAction* alwaysOnTopAction = menu->addAction(
        isOnTop ? tr("Unset Always On Top") : tr("Set Always On Top")
    );

    const QAction* const selectedAction = menu->exec(mapToGlobal(pos));
    delete menu;

    if (selectedAction == maximizeAction) {
        toggleFullscreen(isFullscreen);
    } else if (selectedAction == alwaysOnTopAction) {
        setWindowFlag(Qt::WindowStaysOnTopHint, !isOnTop);
        show();
    }
}

void CoverWindow::toggleFullscreen(const bool isFullscreen) {
    if (isFullscreen) {
        showNormal();
        resize(originalPixmap.size());

        const QScreen* currentScreen = screen();

        if (currentScreen == nullptr) {
            currentScreen = QApplication::primaryScreen();
        }

        const QRect screenGeometry = currentScreen->availableGeometry();
        const QSize windowSize = size();

        const i32 xPos = screenGeometry.x() +
                         ((screenGeometry.width() - windowSize.width()) / 2);
        const i32 yPos = screenGeometry.y() +
                         ((screenGeometry.height() - windowSize.height()) / 2);

        move(xPos, yPos);
    } else {
        showFullScreen();
    }
}

void CoverWindow::resizeEvent(QResizeEvent* const event) {
    QDialog::resizeEvent(event);

    if (originalPixmap.isNull()) {
        return;
    }

    const QSize availableSize = size();

    const QPixmap scaled = originalPixmap.scaled(
        availableSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    coverLabel->setPixmap(scaled);
    coverLabel->resize(scaled.size());
}
