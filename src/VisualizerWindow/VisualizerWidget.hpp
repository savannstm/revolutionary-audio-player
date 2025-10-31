#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"

#ifdef Q_OS_WINDOWS
#include <GL/glew.h>
#elifdef Q_OS_LINUX
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <projectM-4/projectM.h>

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLWindow>
#include <QScreen>
#include <QTimer>

constexpr u16 MIN_MESH_SIZE = 10;
constexpr u16 MAX_MESH_SIZE = 200;

constexpr u16 DEFAULT_MESH_WIDTH = 80;
constexpr u16 DEFAULT_MESH_HEIGHT = 40;

constexpr u16 MIN_FPS = 24;
constexpr u16 MAX_FPS = 360;

constexpr QSize MIN_VISUALIZER_SIZE = QSize(800, 600);

//! Because QOpenGLWidget uses off-screen framebuffer, and QOpenGLWindow
//! maintains current context, we use the latter. projectM (as of version 4.1.4)
//! is not aware about off-screen buffer of OpenGLWidget, and requires explicit
//! current context.
class VisualizerWidget : public QOpenGLWindow {
    Q_OBJECT

   public:
    explicit VisualizerWidget(
        const f32* visualizerBuffer,
        QWidget* parent = nullptr
    );

    ~VisualizerWidget() override;

    void setChannels(const AudioChannels chn) {
        channels = projectm_channels(chn);

        if (chn == AudioChannels::Surround51) {
            bufferSize = MIN_BUFFER_SIZE_3BYTES;
        } else {
            bufferSize = MIN_BUFFER_SIZE;
        }
    }

    void addSamples() {
        projectm_pcm_add_float(
            projectM,
            buffer,
            bufferSize / (F32_SAMPLE_SIZE * channels),
            channels
        );
    }

    void clear() { projectm_pcm_add_float(projectM, nullptr, 0, channels); }

    void setMeshSize(const u32 width, const u32 height) {
        projectm_set_mesh_size(projectM, width, height);
    }

    void setFPS(const u16 fps) {
        updateTimer.start(SECOND_MS / fps);
        projectm_set_fps(projectM, fps);
    }

    void loadPreset(const QString& presetPath) {
        QFile preset = QFile(presetPath);

        if (!preset.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(
                parent,
                tr("Failed to open preset"),
                preset.errorString()
            );
            return;
        }

        const QByteArray data = preset.readAll();
        projectm_load_preset_data(projectM, data.data(), true);
    }

   protected:
    void initializeGL() override;
    void resizeGL(i32 width, i32 height) override;
    void paintGL() override;

    void mouseMoveEvent(QMouseEvent* event) override {
        QApplication::sendEvent(parent, event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        QApplication::sendEvent(parent, event);
    }

   private:
    QWidget* parent = nullptr;
    QTimer updateTimer;

    projectm* projectM = nullptr;

    const f32* buffer = nullptr;
    projectm_channels channels = projectm_channels::PROJECTM_MONO;
    u16 bufferSize = 0;
};