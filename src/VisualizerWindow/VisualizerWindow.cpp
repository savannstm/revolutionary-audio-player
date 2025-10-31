#include "VisualizerWindow.hpp"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMenu>

VisualizerWindow::VisualizerWindow(
    const f32* visualizerBuffer,
    QWidget* parent
) :
    QDialog(parent, Qt::Window) {
    setWindowTitle(tr("Visualizer"));
    setMouseTracking(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins({ 0, 0, 0, 0 });

    dockWidget = new QWidget(this);
    auto* dockWidgetLayout = new QFormLayout(dockWidget);

    meshWidthSpin = new QSpinBox(dockWidget);
    meshWidthSpin->setRange(MIN_MESH_SIZE, MAX_MESH_SIZE);
    meshWidthSpin->setValue(DEFAULT_MESH_WIDTH);

    meshHeightSpin = new QSpinBox(dockWidget);
    meshHeightSpin->setRange(MIN_MESH_SIZE, MAX_MESH_SIZE);
    meshHeightSpin->setValue(DEFAULT_MESH_HEIGHT);

    fpsSpin = new QSpinBox(dockWidget);
    fpsSpin->setRange(MIN_FPS, MAX_FPS);
    fpsSpin->setValue(u16(screen()->refreshRate()));

    dockWidgetLayout->addRow(tr("Mesh Width:"), meshWidthSpin);
    dockWidgetLayout->addRow(tr("Mesh Height:"), meshHeightSpin);
    dockWidgetLayout->addRow("FPS:", fpsSpin);

    dockWidget->setMinimumWidth(0);
    dockWidget->setMaximumWidth(dockWidget->sizeHint().width());

    dockWidget->hide();

    visualizerWidget = new VisualizerWidget(visualizerBuffer, this);

    //! VisualizerWidget inherits from QOpenGLWindow, so we need to put it into
    //! a container.
    QWidget* const visualizerContainer =
        QWidget::createWindowContainer(visualizerWidget, this);

    layout->addWidget(visualizerContainer);
    layout->addWidget(dockWidget);

    connect(
        meshWidthSpin,
        &QSpinBox::valueChanged,
        this,
        &VisualizerWindow::updateMeshSize
    );

    connect(
        meshHeightSpin,
        &QSpinBox::valueChanged,
        this,
        &VisualizerWindow::updateMeshSize
    );

    connect(
        fpsSpin,
        &QSpinBox::valueChanged,
        this,
        &VisualizerWindow::updateFPS
    );

    connect(visualizerWidget, &VisualizerWidget::destroyed, this, [&] -> void {
        destroy();
    });
}

void VisualizerWindow::toggleFullscreen(const bool isFullscreen) {
    if (isFullscreen) {
        setWindowFlag(Qt::FramelessWindowHint, false);
        showNormal();

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
        setWindowFlag(Qt::FramelessWindowHint, true);
        showFullScreen();
    }
}

void VisualizerWindow::mouseMoveEvent(QMouseEvent* event) {
    const i32 xPos = event->pos().x();
    const i32 yPos = event->pos().y();

    const i32 windowWidth = width();
    const i32 dockHeight = dockWidget->height();

    const bool nearRightEdge =
        (xPos > windowWidth - dockWidget->maximumWidth());

    if (nearRightEdge) {
        if (!dockWidget->isVisible()) {
            dockWidget->show();
        }
    } else {
        if (dockWidget->isVisible()) {
            dockWidget->hide();
        }
    }

    QDialog::mouseMoveEvent(event);
}

void VisualizerWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        auto* menu = new QMenu();

        const QAction* loadPresetAction = menu->addAction(tr("Load Preset"));

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

        const QAction* selectedAction = menu->exec(QCursor::pos());

        if (selectedAction == loadPresetAction) {
            const QString presetPath = QFileDialog::getOpenFileName(
                nullptr,
                tr("Select Preset"),
                nullptr,
                tr("Milkdrop Presets (*.milk)")
            );

            if (!presetPath.isEmpty()) {
                visualizerWidget->loadPreset(presetPath);
            }
        } else if (selectedAction == maximizeAction) {
            toggleFullscreen(isFullscreen);
        } else if (selectedAction == alwaysOnTopAction) {
            setWindowFlag(Qt::WindowStaysOnTopHint, !isOnTop);
            show();
        }

        delete menu;
    } else {
        const auto position = event->position();
        const f32 xPos = f32(position.x());
        const f32 yPos = f32(position.y());

        // TODO: I don't quite get how touch works
        // projectm_touch(projectM, xPos, yPos, 1,
        // PROJECTM_TOUCH_TYPE_RANDOM);
    }
}