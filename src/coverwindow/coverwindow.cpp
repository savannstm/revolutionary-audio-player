#include "coverwindow.hpp"

#include "extractmetadata.hpp"

#include <QPixmap>

CoverWindow::CoverWindow(
    const QString& coverPath,
    const QString& title,
    QWidget* parent
) {
    layout.addWidget(&image);
    updateCover(coverPath, title);
}

void CoverWindow::resizeEvent(QResizeEvent* event) {
    const QSize newSize = event->size();
    u16 newWidth = newSize.width();
    u16 newHeight = static_cast<u16>(newWidth / aspectRatio);

    if (newHeight > newSize.height()) {
        newHeight = newSize.height();
        newWidth = static_cast<u16>(newHeight * aspectRatio);
    }

    if (size() != QSize(newWidth, newHeight)) {
        resize(newWidth, newHeight);
    }

    QDialog::resizeEvent(event);
}

void CoverWindow::updateCover(const QString& coverPath, const QString& title) {
    auto pixmap = QPixmap();

    const auto coverBytes = extractCover(coverPath.toUtf8().constData());

    pixmap.loadFromData(coverBytes.data(), coverBytes.size());
    image.setPixmap(pixmap);
    image.setScaledContents(true);

    const u16 width = pixmap.width();
    const u16 height = pixmap.height();

    setMinimumSize(MIN_SIZE, MIN_SIZE);
    setMaximumSize(width, height);
    resize(width, height);
    aspectRatio = static_cast<f64>(width) / height;

    setWindowTitle(u"%1: Cover"_s.arg(title));
}