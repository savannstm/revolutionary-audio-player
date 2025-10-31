#pragma once

#include "Aliases.hpp"
#include "VisualizerWidget.hpp"

#include <QDialog>
#include <QSpinBox>

class VisualizerWindow : public QDialog {
    Q_OBJECT

   public:
    explicit VisualizerWindow(
        const f32* visualizerBuffer,
        QWidget* parent = nullptr
    );

    auto visualizer() -> VisualizerWidget* { return visualizerWidget; };

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

   private:
    void updateMeshSize() {
        visualizerWidget->setMeshSize(
            meshWidthSpin->value(),
            meshHeightSpin->value()
        );
    }

    void updateFPS(const i32 fps) { visualizerWidget->setFPS(fps); }

    void toggleFullscreen(bool isFullscreen);

    VisualizerWidget* visualizerWidget = nullptr;
    QWidget* dockWidget = nullptr;
    QSpinBox* meshWidthSpin = nullptr;
    QSpinBox* meshHeightSpin = nullptr;
    QSpinBox* fpsSpin = nullptr;
};